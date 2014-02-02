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

/**
 * This program is a simple gdigi client that uses the gdigi DBus C API to
 * get or set device parameters.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gdigi_api.h"

void usage (void)
{
    fprintf(stderr, "gdigi_client [get|set] position id [value]\n"
                    "'value' is required for set, ignored for set.\n");
    exit(1);
}

int main (int argc, char **argv)
{
    guint id;
    guint pos;
    guint val;

    if (gdigi_init() < 0) {
        fprintf(stderr, "gdigi_init() failed.\n");
        exit(1);
    }

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (!strncmp(argv[1], GDIGI_DBUS_METHOD_GET, 3)) {
        if (argc < 4) {
            usage();
            exit(1);
        }

        pos = atoi(argv[2]);
        id = atoi(argv[3]);

        if (gdigi_get_parameter(pos, id, &val) < 0) {
            fprintf(stderr, "gdigi_get_parameter failed for pos %u id %u.\n",
                            pos, id);
            exit(1);
        }
        printf("Get (pos %d, id %d) --> %d\n", pos, id, val);
    } else if (!strncmp(argv[1], GDIGI_DBUS_METHOD_SET, 3)) {
        if (argc < 5) {
            exit(1);
            usage();
        }

        pos = atoi(argv[2]);
        id = atoi(argv[3]);
        val = atoi(argv[4]);

        if (gdigi_set_parameter(pos, id, val) < 0) {
            fprintf(stderr, "gdigi_set_parameter pos %u id %u val %u failed.\n",
                            pos, id, val);
        }
        printf("Set (pos %d, id %d) --> %d\n", pos, id, val);
    } else {
        usage();
        exit(1);
    }

    return 0;
}
