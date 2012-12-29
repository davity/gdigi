/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <string.h>
#include <gdk/gdk.h>
#include "gdigi.h"
#include "gdigi_api.h"
#include "gdigi_api_server.h"
#include "gui.h"

static int api_sock = -1;
static fd_set read_socks;

typedef struct {
    guint               id;
    guint               position;
    struct sockaddr_un  client_address;
    int                 client_address_len;
} request_parameter_t;

static GTree *request_parameter_tree;

/**
 * The q-sort'ish comparison function for an id-position tree.
 *
 * \param a The first id/pos parameter.
 * \param b The second id/pos parameter.
 * \param data Unused.
 */
static gint id_position_tree_compare(gconstpointer a,
                                     gconstpointer b,
                                     gpointer data)
{
    gint id_a = GPOINTER_TO_INT(a) & 0xFF0000;
    gint id_b = GPOINTER_TO_INT(b) & 0xFF0000;

    if (id_a > id_b) {
        return 1;
    } else if (id_a == id_b) {
        gint pos_a = GPOINTER_TO_INT(a) & 0xFFFF;
        gint pos_b = GPOINTER_TO_INT(b) & 0xFFFF;
        return pos_a - pos_b;
    } else {
        return -1;
    }
}

/**
 * Free a client request parameter structure.
 *
 * \param req The request structure to be freed.
 */
static void request_parameter_free_client(request_parameter_t *req)
{
    g_slice_free(request_parameter_t, req);
}

static void request_parameter_free_client_list (GList *client_list)
{
    g_list_free_full(client_list,
                     (GDestroyNotify) request_parameter_free_client);
}

/**
 * Process a request_parameter request.
 *
 * \param id Id of the requested parameter.
 * \param pos Position of the requested parameter.
 * \param client_address Client address (unix socket path) to reply to.
 * \param client_address_len Length of the client address.
 */
static void get_parameter_request(guint id, guint pos,
                                  struct sockaddr_un *client_address,
                                  gint client_address_len)
{
    gpointer *key;
    GList *client_list;
    request_parameter_t *req = g_slice_new(request_parameter_t);

    req->id = id;
    req->position = pos;
    req->client_address = *client_address; /* Struct copy */
    req->client_address_len = client_address_len;

    key = GDIGI_KEY(pos, id);
    client_list = g_tree_lookup(request_parameter_tree, key);
    if (client_list == NULL) {
        client_list = g_list_append(client_list, req);
        g_tree_insert(request_parameter_tree, key, client_list);
    } else {
        client_list = g_list_append(client_list, req);
        g_tree_steal(request_parameter_tree, key);
        g_tree_insert(request_parameter_tree, key, client_list);
    }

    get_option(id, pos);
}

/**
 * Do the initialization required for the API.
 *
 * Create a unix domain socket and bind it to a path in /var/tmp. 
 * Set the socket to non-blocking and init the fd set.
 * Create a tree on which to hang pending client requests.
 */
void gdigi_api_server_init(void)
{
    socklen_t len;
    struct sockaddr_un addr1, addr2;
    int flags;

    if (api_sock != -1) {
        return;
    }

    api_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (api_sock < 0) {
        g_warning("Failed to get server sock: %m");
        return;
    }

    (void) unlink(GDIGI_SOCKET_PATH);

    memset(&addr1, '\0', sizeof(addr1));
    strncpy(addr1.sun_path, GDIGI_SOCKET_PATH, sizeof(addr1.sun_path) - 1);
    addr1.sun_family = AF_UNIX;

    if (bind(api_sock, (struct sockaddr *)&addr1, sizeof(struct sockaddr_un)) < 0) {
        g_warning("bind socket error: %m");
        goto sock_err;
    }

    if (getsockname(api_sock, (struct sockaddr *)&addr2, &len) < 0) {
        g_warning("getsockname socket error: %m");
        goto sock_err;
    }

    if ((flags = fcntl(api_sock, F_GETFL, 0)) < 0) {
        g_warning("fcntl get flags error: %m");
        goto sock_err;
    }

    if (fcntl(api_sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        g_warning("fcntl set flags error: %m");
        goto sock_err;
    }

    FD_ZERO(&read_socks);

    debug_msg(DEBUG_API, "Got server sock %d\n", api_sock);

    request_parameter_tree = g_tree_new_full(id_position_tree_compare,
                                             NULL, 
                                             NULL,
                                             (GDestroyNotify) request_parameter_free_client_list);

    return;

sock_err:
    close(api_sock);
    api_sock = -1;
}

/**
 * Process a request to deliver a 'RECEIVE_PARAMETER_VALUE' message to the
 * device. Since this is intended to simulate knob twiddling, also post
 * a 'REQUEST_PARAMETER_VALUE' to update the gui.
 *
 * \param id    Parameter id.
 * \param pos   Paramter position.
 * \param val   New paramter value.
 */
static void set_parameter_request(guint id, guint pos, guint value)
{
    SettingParam param;
    debug_msg(DEBUG_API, "set_option for id %d pos %d value %d\n",
                          id, pos, value);

    set_option(id, pos, value);

    /* Refresh the gui. */
    param.id = id;
    param.position = pos;
    param.value = value;

     GDK_THREADS_ENTER();
     apply_setting_param_to_gui(&param);
     GDK_THREADS_LEAVE();

}

/**
 * Check if there is a message to process on the client socket.
 */
void gdigi_api_server_select(void)
{
    int rc, n;
    socklen_t len;
    gdigi_api_request_t req;
    struct sockaddr_un cliaddr;
    struct timeval none = {0};

    FD_SET(api_sock, &read_socks);
    rc = select(api_sock + 1, &read_socks, NULL, NULL, &none);
    if (rc < 0) {
        g_warning("Select socket error: %m");
        return;
    } else if (rc == 0) {
        /* Nothing to do */
        return;
    }

    debug_msg(DEBUG_API, "Read socket.\n");

    memset(&req, '\0', sizeof(req));
    memset(&cliaddr, '\0', sizeof(cliaddr));
    len = sizeof(struct sockaddr_un);
    n = recvfrom(api_sock, &req, sizeof(req), 0,
                 (struct sockaddr *)&cliaddr, &len);

#define HEX_WIDTH 26
    if (debug_flag_is_set(DEBUG_API)) {
        printf("%d bytes from client %s:\n", n, cliaddr.sun_path);
        unsigned char *msg = (unsigned char *)&req;
        int x;
        for (x = 0; x < n; x++) {
            if (x && (x % HEX_WIDTH) == 0) {
                printf("\n");
            }
            printf("%02x ", msg[x]);
        }
        if (x % HEX_WIDTH) {
            printf("\n");
        }
    }

    req.op = g_ntohl(req.op);
    req.id = g_ntohl(req.id);
    req.position = g_ntohl(req.position);
    req.value = g_ntohl(req.value);

    switch (req.op) {
    case GDIGI_API_GET_PARAMETER:
        get_parameter_request(req.id, req.position, &cliaddr, len);
        break;

    case GDIGI_API_SET_PARAMETER:
        set_parameter_request(req.id, req.position, req.value);
        break;

    default:
        g_warning("Unknown client operation 0x%x.\n", req.op);
        break;
    }
}

/**
 * Deliver a parameter value to a client that's waiting for it.
 *
 * \param req   The client request structure.
 * \param val   The value returned to the client.
 */
static void get_parameter_response(request_parameter_t *req, guint *value)
{
    gint32 rc;
    gdigi_api_response_t rsp;

    memset(&rsp, '\0', sizeof(gdigi_api_response_t));
    rsp.id = g_htonl(req->id);
    rsp.position = g_htonl(req->position);
    rsp.value = g_htonl(*value);

    debug_msg(DEBUG_API, "Reply to client %s with value %d\n",
                         req->client_address.sun_path, *value);

    rc = sendto(api_sock, &rsp, sizeof(gdigi_api_response_t), 0,
                (struct sockaddr *)&req->client_address,
                req->client_address_len);
    if (rc < 0) {
        g_warning("Failed to respond to client %s: %m",
                   req->client_address.sun_path);
        return;
    }
}

/**
 * Checks if there are pending client requests for the indicated parameter
 * and sends responses.
 *
 * \param id    Paramter id.
 * \param pos   Parameter position.
 * \param val   Parameter value.
 */
void gdigi_api_server_get_parameter_response(guint id, guint pos, guint value)
{
    GList *client_list;
    gpointer key = GDIGI_KEY(pos, id);

    if (api_sock < 0) {
        return;
    }

    debug_msg(DEBUG_API, "Looking for response for id %d pos %d\n", id, pos);

    client_list = g_tree_lookup(request_parameter_tree, key);
    if (!client_list) {
        return;
    }
    g_list_foreach(client_list, (GFunc)get_parameter_response, &value);

    g_tree_remove(request_parameter_tree, key);
}

void gdigi_api_server_fini(void)
{
    if (api_sock >= 0) {
        close(api_sock);
        api_sock = -1;
        unlink(GDIGI_SOCKET_PATH);
    }
}
