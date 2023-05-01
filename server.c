#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include "common.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "\nUsage: %s <PORT_SERVER>\n", argv[0]);
        return 1;
    }

    uint16_t server_port;
    int rc = sscanf(argv[1], "%hu", &server_port);
    DIE(rc != 1, "sscanf() failed");

    return 0;
}
