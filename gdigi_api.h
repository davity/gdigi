/*
 *  Copyright Tim LaBerge, 2014.
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

/**
 * This module defines the public gdigi DBus API.
 *
 * See Gdigi.pm for a perl implementation of the API.
 */

#ifndef GDIGI_API_H
#define GDIGI_API_H

#include <glib.h>

#define GDIGI_DBUS_SESSION_NAME "gdigi.server"
#define GDIGI_DBUS_CLIENT_NAME "gdigi.client"
#define GDIGI_DBUS_OBJECT_NAME "/gdigi/parameter/Object"
#define GDIGI_DBUS_INTERFACE_NAME "gdigi.parameter.io"
#define GDIGI_DBUS_METHOD_GET "get"
#define GDIGI_DBUS_METHOD_SET "set"

gint gdigi_init(void);
gint gdigi_fini(void);

gint gdigi_get_parameter(guint position, guint id, guint *value);
gint gdigi_set_parameter(guint position, guint id, guint value);

void gdigi_set_debug();
void gdigi_clear_debug();

#endif
