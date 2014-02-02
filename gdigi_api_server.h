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
 * These are the functions that gdigi needs to implement the server side of
 * the public Dbus-based API.
 */
#ifndef GDIGI_API_SERVER_H
#define GDIGI_API_SERVER_H

void gdigi_dbus_init(void);
void gdigi_dbus_fini(void);
void gdigi_dbus_read(void);
void gdigi_api_server_get_dbus_parameter_response(guint pos, guint id, guint value);

#endif /* GDIGI_API_SERVER_H */
