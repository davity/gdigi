#define _BSD_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <string.h>
#include <glib.h>
#include "gdigi_api.h"

static gboolean gdigi_api_debug = FALSE;

/**
 * Turns on library debugging.
 */
void gdigi_set_debug (void)
{
    gdigi_api_debug = TRUE;
}

/*
 * Turns off library debugging.
 */
void gdigi_clear_debug (void)
{
    gdigi_api_debug = FALSE;
}

/**
 * Variadic macro for debugging.
 */
#define gdigi_debug(_format, _args...) \
    if (gdigi_api_debug) { \
        fprintf(stderr, "GDIGI CLIENT: " _format, ## _args); \
    }


static int gdigi_api_socket = -1;
#define GDIGI_CLIENT_NAME "/var/tmp/gdigi_client_XXXXXX"

static struct sockaddr_un gdigi_server_addr;
static fd_set rfds;

#define GDIGI_API_CLIENT_NAME_TEMPLATE "/var/tmp/gdigi_client_XXXXXX"
static char gdigi_api_client_name[64];

/**
 * API to initialize the library.
 *
 * Returns 0 on success, -1 with errno set on error.
 */
gint gdigi_init(void)
{
    struct sockaddr_un client_addr;
    int client_fd;
    
    gdigi_api_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (gdigi_api_socket < 0) {
        return -1;
    }

    
    snprintf(gdigi_api_client_name, 64, GDIGI_API_CLIENT_NAME_TEMPLATE);
    client_fd = mkstemp(gdigi_api_client_name);
    close(client_fd);
    unlink(gdigi_api_client_name);

    memset(&client_addr, '\0', sizeof(client_addr));
    strncpy(client_addr.sun_path, gdigi_api_client_name, sizeof(client_addr.sun_path) - 1);
    client_addr.sun_family = AF_UNIX;
    gdigi_debug("Set client addr to %s\n", client_addr.sun_path);

    if (bind(gdigi_api_socket, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_un)) < 0) {
        fprintf(stderr, "Bind failed: %m\n");
        return -1;
    }

    memset(&gdigi_server_addr, '\0', sizeof(struct sockaddr_un));
    gdigi_server_addr.sun_family = AF_UNIX;
    snprintf(gdigi_server_addr.sun_path,
              sizeof(gdigi_server_addr.sun_path) - 1,
              "%s", GDIGI_SOCKET_PATH);

    FD_ZERO(&rfds);
    
    return 0;
}

/**
 * Send a request to the server.
 *
 * Returns 0 on success, -1 on error.
 * 
 * \param req   The request to send.
 */
static gint gdigi_send(gdigi_api_request_t *req)
{
    gdigi_debug("Sending op %d id %d pos %d val %d\n",
                 req->op, req->id, req->position, req->value);

    req->op = g_htonl(req->op);
    req->id = g_htonl(req->id);
    req->position = g_htonl(req->position);
    req->value = g_htonl(req->value);


    return sendto(gdigi_api_socket, req, sizeof(gdigi_api_request_t), 0,
                  (struct sockaddr *) &gdigi_server_addr, 
                  sizeof(gdigi_server_addr));
}

/**
 * Receive a response from the server. Times out in one second.
 *
 * \param rsp   A pointer to the response structure to populate.
 */
static gint gdigi_receive(gdigi_api_response_t *rsp)
{
    int n;
    socklen_t len;
    int rc;
    struct timeval ts = {5, 0};

    FD_SET(gdigi_api_socket, &rfds);

    rc = select(gdigi_api_socket + 1, &rfds, NULL, NULL, &ts);
    if (rc < 0) {
        perror("select:");
        return -1;
    } else if (rc == 0) {
        fprintf(stderr, "timeout on read.\n");
        return -1;
    }

    n = recvfrom(gdigi_api_socket, rsp, sizeof(gdigi_api_response_t), 0,
                   (struct sockaddr *) &gdigi_server_addr, &len);
    if (n < 0) {
        perror("recvfrom:");
        return -1;
    } else if (n < sizeof(gdigi_api_response_t)) {
        fprintf(stderr, "Short read on recv, got %d expected %d.\n", 
                        n, (int)sizeof(gdigi_api_response_t));
        return -1;
    }

    rsp->id = g_ntohl(rsp->id);
    rsp->position = g_ntohl(rsp->position);
    rsp->value = g_ntohl(rsp->value);

    return 0;
}

gint gdigi_get_parameter (guint id, guint pos, guint *value)
{
    uint32_t op = GDIGI_API_GET_PARAMETER;
    gdigi_api_request_t req;
    gdigi_api_response_t rsp;

    req.op = op;
    req.id = id;
    req.position = pos;
    req.value = 0;

    if (gdigi_send(&req) < 0) {
        perror("gdigi_send");
        return -1;
    }

    memset(&rsp, '\0', sizeof(gdigi_api_response_t));
    if (gdigi_receive(&rsp) < 0) {
        perror("gdigi_receive");
        return -1;
    }

    gdigi_debug("Got rsp id %d pos %d val %d\n",
                 rsp.id, rsp.position, rsp.value);

    *value = rsp.value;

    return 0;
}

gint gdigi_set_parameter (guint id, guint position, guint value)
{
    gdigi_api_request_t req;

    req.op = GDIGI_API_SET_PARAMETER;
    req.id = id;
    req.position = position;
    req.value = value;

    return gdigi_send(&req);
}

void gdigi_fini (void)
{
    close(gdigi_api_socket);
    unlink(gdigi_api_client_name);
}
