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

#include <stdlib.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include "gdigi_api.h"

static gboolean gdigi_api_debug = FALSE;

static DBusConnection* conn;
static DBusError err;

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

/**
 * API to initialize the library.
 *
 * Returns 0 on success, -1 with errno set on error.
 */
gint gdigi_init(void)
{
    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
       fprintf(stderr, "Error on connection to session bus: %s.\n",
                        err.message);
       dbus_error_free(&err);
       return -1;
    }
    if (!conn) {
        fprintf(stderr, "Failed to connect to session bus: %s.\n", err.message);
        dbus_error_free(&err);
        return -1;
    }

    int ret;
    ret = dbus_bus_request_name(conn,
                                GDIGI_DBUS_CLIENT_NAME,
                                DBUS_NAME_FLAG_REPLACE_EXISTING,
                                &err);
    if (ret < 0) {
        fprintf(stderr, "failed to request bus session name %s: %s\n", GDIGI_DBUS_CLIENT_NAME, err.message);
        return -1;
    }

    if (dbus_error_is_set(&err)) {
       fprintf(stderr, "Failed to request name on bus: %s.\n", err.message);
       dbus_error_free(&err);
       return -1;
    }

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
       fprintf(stderr, "Failed to obtain ownership on bus: %s.\n", err.message);
       dbus_error_free(&err);
       return -1;
    }

    return 0;
}

static DBusMessage* gdigi_alloc_msg(char *method)
{
    DBusMessage *msg;

    msg = dbus_message_new_method_call(GDIGI_DBUS_SESSION_NAME,
                                       GDIGI_DBUS_OBJECT_NAME,
                                       GDIGI_DBUS_INTERFACE_NAME,
                                       method);
    if (!msg) {
       fprintf(stderr, "Failed to allocate new message\n");
    }

    return msg;
}

gint gdigi_set_parameter(guint position, guint id, guint value)
{
    DBusMessage *msg = gdigi_alloc_msg(GDIGI_DBUS_METHOD_SET);
    DBusMessageIter args;
    DBusPendingCall* pending;

    if (msg == NULL) {
        return -1;
    }

    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &position)) {
        fprintf(stderr, "Failed to append arguments.!\n");
        return -1;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id)) {
        fprintf(stderr, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &value)) {
        fprintf(stderr, "Out Of Memory!\n");
        return -1;
    }

    if (!dbus_connection_send_with_reply (conn, msg, &pending, 1000)) {
        fprintf(stderr, "Failed to send dbus message.\n");
        return -1;
    }
    if (NULL == pending) {
        fprintf(stderr, "Pending Call Null\n");
        return -1;
    }
    dbus_connection_flush(conn);
    dbus_message_unref(msg);

    dbus_pending_call_block(pending);
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
       fprintf(stderr, "Reply Null\n");
       exit(1);
    }
    dbus_pending_call_unref(pending);

    value = 0;
    int ret = -1;
    if (!dbus_message_iter_init(msg, &args)) {
       fprintf(stderr, "Message has no arguments!\n");
       ret = -1;
    } else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
       fprintf(stderr, "Argument is not uint32!\n");
       ret = -1;
    } else {
       dbus_message_iter_get_basic(&args, &value);
       ret = 0;
    }

    if (ret) {
        return ret;
    }

    gdigi_debug("Set: (pos %u id %u) --> %u\n", position, id, value);

    dbus_message_unref(msg);

    return 0;
}
        
gint gdigi_get_parameter (guint position, guint id, guint *value)
{
    DBusMessageIter args;
    DBusPendingCall* pending;

    if (value == NULL) {
        fprintf(stderr, "gdigi_get_parameter() called with NULL value.\n");
        return -1;
    }

    DBusMessage *msg = gdigi_alloc_msg(GDIGI_DBUS_METHOD_GET);
    if (!msg) {
        return -1;
    }

    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &position)) {
       fprintf(stderr, "Failed to append arguments.!\n");
       return -1;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &id)) {
       fprintf(stderr, "Out Of Memory!\n");
       return -1;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, value)) {
       fprintf(stderr, "Out Of Memory!\n");
       return -1;
    }

    if (!dbus_connection_send_with_reply(conn, msg, &pending, 1000)) {
        fprintf(stderr, "Failed to send dbus message.\n");
        return -1;
    }

    if (!pending) {
       fprintf(stderr, "Pending Call Null\n");
       return -1;
    }

    dbus_connection_flush(conn);
    dbus_message_unref(msg);
    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
       fprintf(stderr, "Reply Null\n");
       return -1;
    }

    *value = 0;
    int ret = -1;
    if (!dbus_message_iter_init(msg, &args)) {
       fprintf(stderr, "Message has no arguments!\n");
    } else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args)) {
       fprintf(stderr, "Argument is not uint32!\n");
    } else {
       dbus_message_iter_get_basic(&args, value);
       ret = 0;
    }

    if (ret) {
        return -1;
    }

    gdigi_debug("Get: (pos %u id %u) --> %u\n", position, id, *value);

    dbus_pending_call_unref(pending);
    dbus_message_unref(msg);

    return 0;
}
