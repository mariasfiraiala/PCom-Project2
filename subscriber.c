#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include "common.h"
#include "utils.h"

void subscriber(int sockfd)
{
    struct pollfd pfds[MAX_PFDS];
    int nfds = 0;

    /* stdin fd entry */
    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    ++nfds;

    /* server socket fd entry */
    pfds[nfds].fd = sockfd;
    pfds[nfds].events = POLLIN;
    ++nfds;

    char buf[MSG_MAXSIZE + 1];
    memset(buf, 0, MSG_MAXSIZE + 1);

    struct chat_packet sent_packet;
    struct chat_packet recv_packet;
 
    while (1) {
        /* wait for data on at least one fd */
        poll(pfds, nfds, -1);

        /* the server response */
        if ((pfds[1].revents & POLLIN) != 0) {
            int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
            if (rc <= 0)
                 break;

            /* TODO: Print the message received from the UDP client */
        } else if ((pfds[0].revents & POLLIN) != 0) {
            /* subscriber sends a request */
            fgets(buf, sizeof(buf), stdin);

            /* if the subscriber wishes to disconnect */
            if (buf[0] == 'e')
                break;
    
            sent_packet.len = strlen(buf) + 1;
            strcpy(sent_packet.message, buf);
            send_all(sockfd, &sent_packet, sizeof(sent_packet));

            /* echo the subscriber request */
            if (buf[0] == 's')
                printf("Subscribed to topic.\n");
            else
                printf("Unsubscribed from topic.\n");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "\nUsage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
        return 1;
    }

    uint16_t server_port;
    int rc = sscanf(argv[3], "%hu", &server_port);
    DIE(rc != 1, "sscanf() failed");

    int sockfd = -1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket() failed");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton() failed");

    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect() failed");

    subscriber(sockfd);

    close(sockfd);

    return 0;
}
