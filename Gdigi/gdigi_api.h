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

/**
 * This module defines the public gdigi API.
 *
 * The gdigi socket accepts various messages. Currently, these include
 * parameter get/set. This allows the gui and the device to be driven via
 * the command line.
 *
 * See Gdigi.pm for a perl implementation of the API.
 *
 * TODO: Build a library implementing the API and build the perl module
 *       from the library.
 */

#ifndef GDIGI_API_H
#define GDIGI_API_H


#include <glib.h>

#define GDIGI_SOCKET_PATH "/var/tmp/gdigi_sock"

typedef enum {
    GDIGI_API_GET_PARAMETER = 1,
    GDIGI_API_SET_PARAMETER = 2,
} gdigi_api_op_t;

/* 
 * Even though we currently use a unix domain socket, messages are exchanged
 * in network byte order.
 */
typedef struct {
    guint32 op;
    guint32 id;
    guint32 position;
    guint32 value;          /* Ignored for SET */
} gdigi_api_request_t;

/* Only sent for GET. */
typedef struct {
    guint32 id;
    guint32 position;
    guint32 value;
} gdigi_api_response_t;

void gdigi_fini(void);
gint gdigi_init(void);

gint gdigi_get_parameter(guint id, guint position, guint *value);
gint gdigi_set_parameter(guint id, guint position, guint value);

void gdigi_set_debug();
void gdigi_clear_debug();

#endif /* GDIGI_API_H */
