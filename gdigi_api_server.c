/*
 *  Copyright (c) 2014 Tim LaBerge <tlaberge@visi.com>
 * 
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

#include <dbus/dbus.h>
#include "gdigi.h"
#include "gdigi_api.h"
#include "gui.h"

typedef struct {
    guint               position;
    guint               id;
    DBusMessage        *msg;
    DBusConnection     *conn;
} request_dbus_parameter_t;

/**
 * GTree that stores pending requests.
 *
 * When client get requests are received, they are stored in this tree, keyed by
 * (Position, ID). When the device returns with the corresponding value,
 * we do a lookup in the tree and reply to waiting clients.
 */
static GTree *request_dbus_parameter_tree;

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
static void request_dbus_parameter_free_client(request_dbus_parameter_t *req)
{
    g_slice_free(request_dbus_parameter_t, req);
}

static void request_dbus_parameter_free_client_list (GList *client_list)
{
    g_list_free_full(client_list,
                     (GDestroyNotify) request_dbus_parameter_free_client);
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
static void set_parameter_request(guint pos, guint id, guint value)
{
    SettingParam param;
    debug_msg(DEBUG_API, "set_option for pos %u id %u value %u\n",
                          pos, id, value);

    set_option(id, pos, value);

    /* Refresh the gui. */
    param.position = pos;
    param.id = id;
    param.value = value;

     GDK_THREADS_ENTER();
     apply_setting_param_to_gui(&param);
     GDK_THREADS_LEAVE();

}

/**
 * Process a request_parameter request.
 *
 * \param id Id of the requested parameter.
 * \param pos Position of the requested parameter.
 * \param msg received dbus message.
 * \param conn Dbus connection.
 */
static void get_dbus_parameter_request(guint pos,
                                       guint id,
                                       DBusMessage *msg,
                                       DBusConnection *conn)
{
    gpointer *key;
    GList *client_list;
    request_dbus_parameter_t *req = g_slice_new(request_dbus_parameter_t);

    req->id = id;
    req->position = pos;
    req->msg = msg;
    req->conn = conn;

    debug_msg(DEBUG_API, "get_dbus_parameter_request() for pos %u id %u\n",
                          pos, id);

    key = GDIGI_KEY(pos, id);
    client_list = g_tree_lookup(request_dbus_parameter_tree, key);
    if (client_list == NULL) {
        client_list = g_list_append(client_list, req);
    } else {
        client_list = g_list_append(client_list, req);
        g_tree_steal(request_dbus_parameter_tree, key);
    }
    g_tree_insert(request_dbus_parameter_tree, key, client_list);

    get_option(id, pos);
}

DBusConnection* gdigi_dbus_conn;

/**
 * Initialize the dbus infrastructure.
 */
void gdigi_dbus_init(void)
{
    DBusError err;
    int ret;

    // initialise the error
    dbus_error_init(&err);

    // connect to the bus and check for errors
    gdigi_dbus_conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) { 
       fprintf(stderr, "Connection Error (%s)\n", err.message); 
       dbus_error_free(&err); 
    }
    if (NULL == gdigi_dbus_conn) {
       fprintf(stderr, "Connection Null\n"); 
       return;
    }

    // request our name on the bus and check for errors
    ret = dbus_bus_request_name(gdigi_dbus_conn,
                                GDIGI_DBUS_SESSION_NAME,
                                DBUS_NAME_FLAG_REPLACE_EXISTING ,
                                &err);
    if (dbus_error_is_set(&err)) { 
        fprintf(stderr, "Name Error (%s)\n", err.message); 
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        fprintf(stderr, "Not Primary Owner (%d)\n", ret);
        return;
    }
    request_dbus_parameter_tree =
        g_tree_new_full(id_position_tree_compare,
                        NULL, 
                        NULL,
                        (GDestroyNotify) request_dbus_parameter_free_client_list);

}

/**
 * Reply to a get message.
 *
 * \param msg   The message to reply to.
 * \param conn  The connection on which to reply. 
 * \param val   The value to return.
 */
void gdigi_dbus_message_reply_get(DBusMessage *msg, DBusConnection *conn, dbus_uint32_t val)
{
    DBusMessage *reply;
    DBusMessageIter args;
    dbus_uint32_t serial; 

    debug_msg(DEBUG_API, "gdigi_dbus_message reply() for val %u\n", val);

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &val);

    if (!dbus_connection_send(conn, reply, &serial)) {
        fprintf(stderr, "dbus_connection_send failed.\n");
    }

    dbus_connection_flush(conn);
    dbus_message_unref(reply);
}

/**
 * Reply to a set message.
 *
 * \param msg   The message to reply to.
 * \param conn  The connection on which to reply.
 */
void gdigi_dbus_message_reply_set(DBusMessage *msg, DBusConnection *conn)
{
    DBusMessage *reply;
    DBusMessageIter args;
    dbus_uint32_t serial; 
    dbus_uint32_t retval = 0;

    debug_msg(DEBUG_API, "gdigi_dbus_message_reply_set()\n.");

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &retval);

    if (!dbus_connection_send(conn, reply, &serial)) {
        fprintf(stderr, "dbus_connection_send failed.\n");
    }

    dbus_connection_flush(conn);
    dbus_message_unref(reply);
}

// This string is used by the introspection library.
char *intro_string =
"        <node name=\"/gdigi/parameter/Object\">\n"
"          <interface name=\"gdigi.parameter.io\">\n"
"            <method name=\"get\">\n"
"              <arg name=\"pos\" type=\"u\" direction=\"in\"/>\n"
"              <arg name=\"id\" type=\"u\" direction=\"in\"/>\n"
"              <arg name=\"val\" type=\"u\" direction=\"out\"/>\n"
"            </method>\n"
"            <method name=\"set\">\n"
"              <arg name=\"pos\" type=\"u\" direction=\"in\"/>\n"
"              <arg name=\"id\" type=\"u\" direction=\"in\"/>\n"
"              <arg name=\"val\" type=\"u\" direction=\"in\"/>\n"
"            </method>\n"
"         </interface>\n"
"       </node>\n";

/**
 * Reply to an introspection request. This lets bindings figure out parameter
 * types automagically.
 *
 * \param msg   The message to reply to.
 * \param conn  The connection on which to reply.
 */
void gdigi_dbus_message_reply_introspection(DBusMessage *msg, DBusConnection *conn)
{
    DBusMessage *reply;
    DBusMessageIter args;
    dbus_uint32_t serial; 

    reply = dbus_message_new_method_return(msg);

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &intro_string);

    if (!dbus_connection_send(conn, reply, &serial)) {
        fprintf(stderr, "dbus_connection_send failed.\n");
    }

    dbus_connection_flush(conn);
    dbus_message_unref(reply);
}

typedef enum gdigi_dbus_msg {
    GDIGI_GET_PARAMETER,
    GDIGI_SET_PARAMETER,
    GDIGI_INTROSPECTION,
} gdigi_dbus_msg_t;

/**
 * Read a message off the dbus.
 */
void gdigi_dbus_read()
{
    DBusMessage* msg;
    gdigi_dbus_msg_t msg_type;
    dbus_uint32_t position;
    dbus_uint32_t id;
    dbus_uint32_t value;
    DBusMessageIter args;
    
    dbus_connection_read_write(gdigi_dbus_conn, 0);
    msg = dbus_connection_pop_message(gdigi_dbus_conn);

    if (NULL == msg) {
        return;
    }

    if (dbus_message_is_method_call(msg, "gdigi.parameter.io", "get")) {
        msg_type = GDIGI_GET_PARAMETER;
    } else if (dbus_message_is_method_call(msg, "gdigi.parameter.io", "set")) {
        msg_type = GDIGI_SET_PARAMETER;
    } else if (dbus_message_is_method_call(msg, "org.freedesktop.DBus.Introspectable", "Introspect")) {

        gdigi_dbus_message_reply_introspection(msg, gdigi_dbus_conn);
        return;
    } else {
        fprintf(stderr, "Unknown message\n");
        return;
    }

    if (!dbus_message_iter_init(msg, &args)) {
        fprintf(stderr, "Message has no args!\n");
        return;
    }
    
    if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
        fprintf(stderr, "First arg is not a uint32!\n");
    }

    dbus_message_iter_get_basic(&args, &position);
    if (!dbus_message_iter_next(&args)) {
        fprintf(stderr, "Message has too few args!\n");
        return;
    }
    if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
        fprintf(stderr, "Second arg is not a uint32!\n");
    }
    dbus_message_iter_get_basic(&args, &id);

    if (msg_type == GDIGI_SET_PARAMETER) {

        if (!dbus_message_iter_next(&args)) {
            fprintf(stderr, "Message has too few args!\n");
            return;
        }

        if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
            fprintf(stderr, "Third arg is not a uint32!\n");
        }
        dbus_message_iter_get_basic(&args, &value);

        set_parameter_request(position, id, value);

        gdigi_dbus_message_reply_set(msg, gdigi_dbus_conn);

        dbus_message_unref(msg);

    } if (msg_type == GDIGI_GET_PARAMETER) {
        get_dbus_parameter_request(position, id, msg, gdigi_dbus_conn);
    }
}

/**
 * Deliver a parameter value to a client that's waiting for it.
 *
 * \param req   The client request structure.
 * \param val   The value returned to the client.
 */
static void get_dbus_parameter_response(request_dbus_parameter_t *req, guint *value)
{
    // Send back the three parameters: position/id/value.
    DBusMessage *msg = req->msg;
    gdigi_dbus_message_reply_get(req->msg, req->conn, *value);
    dbus_message_unref(msg);
}

/**
 * Checks if there are pending client requests for the indicated parameter
 * and sends responses.
 *
 * \param id    Paramter id.
 * \param pos   Parameter position.
 * \param val   Parameter value.
 */
void gdigi_api_server_get_dbus_parameter_response(guint pos, guint id, guint value)
{
    GList *client_list;
    gpointer key = GDIGI_KEY(pos, id);

    debug_msg(DEBUG_API, "Looking for dbus response for pos %u id %u\n", id, pos);

    client_list = g_tree_lookup(request_dbus_parameter_tree, key);
    if (!client_list) {
        return;
    }

    g_list_foreach(client_list, (GFunc)get_dbus_parameter_response, &value);
    g_tree_remove(request_dbus_parameter_tree, key);
}

/**
 * Do any cleanup required on exit.
 */
void gdigi_dbus_fini(void)
{
    // Whatever we need to close down the dbus server.
}
