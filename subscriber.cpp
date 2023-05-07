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
#include <netinet/tcp.h>

#include "parser.h"
#include "common.h"
#include "utils.h"

void subscriber(int sockfd, char *id)
{
    /* Signal that a new subscriber is connecting */
    tcp_request_t connect;
    strcpy(connect.id, id);
    connect.type = CONNECT;
    send_all(sockfd, &connect, sizeof(connect));

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

    while (1) {
        /* Wait for data on at least one fd */
        poll(pfds, nfds, -1);

        /* The server response */
        if (pfds[1].revents & POLLIN) {
            int len;
            char buff[2048];

            int rc = recv_all(sockfd, &len, sizeof(len));
            if (!rc)
                return;

            recv_all(sockfd, buff, len);

            /* Print the message received from the UDP client */
            parse_subscription(buff);
        } else if ((pfds[0].revents & POLLIN) != 0) {
            /* Subscriber sends a request */
            fgets(buf, sizeof(buf), stdin);

            /* If the subscriber wishes to disconnect */
            char *argv[BUFSIZE];
            int argc;

            argc = parse_by_whitespace(buf, argv);
            if (!strcmp(argv[0], "exit")) {
                if (argc != 1) {
                    // printf("\nWrong format for exit\n");
                } else {
                    tcp_request_t request;
                    strcpy(request.id, id);
                    request.type = EXIT;

                    send_all(sockfd, &request, sizeof(request));
                    return;
                }
            }

            if (!strcmp(argv[0], "subscribe")) {
                if (argc != 3) {
                    // printf("\nWrong format for subscribe\n");
                } else {
                    tcp_request_t request;
                    strcpy(request.id, id);
                    request.type = SUBSCRIBE;
                    strcpy(request.subscribe.topic, argv[1]);
                    request.subscribe.sf = atoi(argv[2]);

                    send_all(sockfd, &request, sizeof(request));

                    printf("Subscribed to topic.\n");
                }
            }

            if (!strcmp(argv[0], "unsubscribe")) {
                if (argc != 2) {
                    // printf("\nWrong format for unsubscribe\n");
                } else {
                    tcp_request_t request;
                    strcpy(request.id, id);
                    request.type = SUBSCRIBE;
                    strcpy(request.subscribe.topic, argv[1]);

                    send_all(sockfd, &request, sizeof(request));

                    printf("Unsubscribed from topic.\n");
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "\nUsage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    uint16_t server_port;
    int rc = sscanf(argv[3], "%hu", &server_port);
    DIE(rc != 1, "sscanf() failed");

    int sockfd = -1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket() failed");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | TCP_NODELAY, &enable, sizeof(int));
    DIE(sockfd < 0, "setsockopt() failed");

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton() failed");

    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect() failed");

    subscriber(sockfd, argv[1]);

    close(sockfd);

    return 0;
}
