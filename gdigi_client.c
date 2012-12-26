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
#include "gdigi.h"
#include "gdigi_api.h"
#include "gdigi_api_server.h"

void usage (void)
{
    fprintf(stderr, "gdigi_client [get|set] id position [value]\n"
                    "'value' is required for set, ignored for set.\n");
    exit(1);
}

int main (int argc, char **argv)
{
    guint op;
    guint id;
    guint pos;
    guint val;

    if (argc < 2) {
        usage();
        exit(1);
    }

    if (!strncmp(argv[1], "get", 3)) {
        if (argc < 4) {
            usage();
            exit(1);
        }
        op = GDIGI_API_GET_PARAMETER;
        id = atoi(argv[2]);
        pos = atoi(argv[3]);
    } else if (!strncmp(argv[1], "set", 3)) {
        if (argc < 5) {
            exit(1);
            usage();
        }
        op = GDIGI_API_SET_PARAMETER;
        id = atoi(argv[2]);
        pos = atoi(argv[3]);
        val = atoi(argv[4]);
    } else {
        usage();
        exit(1);
    }

    if (gdigi_init() < 0) {
        exit(1);
    }

    gdigi_set_debug();

    switch (op) {
    case GDIGI_API_GET_PARAMETER:
        if (gdigi_get_parameter(id, pos, &val) < 0) {;
            goto fini;
        }
        printf("(id %d, pos %d) --> %d\n", id, pos, val);
        break;

    case GDIGI_API_SET_PARAMETER:
        if (gdigi_set_parameter(id, pos, val) < 0) {
            goto fini;
        }
        printf("Set id %d position %d to value %d.\n", id, pos, val);

        break;

    default:
        fprintf(stderr, "Unknown op 0x%x.\n", op);
    }

fini:
    gdigi_fini();

    exit(0);
}
