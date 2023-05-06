#include <bits/stdc++.h>

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
#include <netinet/tcp.h>

#include "common.h"
#include "utils.h"

std::map<std::string, tcp_client_t *> ids;
std::map<std::string, std::vector<tcp_client_t *>> topics;

void send_message(stored_message_t *message, int fd)
{
    send_all(fd, &message->len, sizeof(message->len));
    send_all(fd, message->buff, message->len);
}

void server(int listenfd, int udp_cli_fd) {
    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_clients = 2;

    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = udp_cli_fd;
    poll_fds[1].events = POLLIN;

    while (1) {
        int rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");

        for (int i = 0; i < num_clients; ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listenfd) {
                    /**
                     * Accept connection received on inactive socket,
                     * a future TCP client
                     */
                    struct sockaddr_in tcp_cli_addr;
                    socklen_t tcp_cli_len = sizeof(tcp_cli_addr);
                    int tcp_cli_fd = accept(listenfd,
                               (struct sockaddr*)&tcp_cli_addr, &tcp_cli_len);
                    DIE(tcp_cli_fd < 0, "accept() failed");

                    /**
                     * Add the newly created tcp client
                     * to the poll list of fds
                     */
                    poll_fds[num_clients].fd = tcp_cli_fd;
                    poll_fds[num_clients].events = POLLIN;
                    ++num_clients;
                } else if (poll_fds[i].fd == udp_cli_fd) {
                    char buff[2048];

                    struct sockaddr udp_cli_addr;
                    socklen_t udp_cli_len = sizeof(udp_cli_addr);

                    int rc = recvfrom(poll_fds[i].fd, buff, 2048, 0, &udp_cli_addr, &udp_cli_len);

                    stored_message_t *message = new stored_message_t;
                    message->len = rc;
                    message->c = 0;
                    message->buff = new char[message->len];
                    memcpy(message->buff, buff, message->len);

                    char topic[51];
                    memcpy(topic, buff, 50);
                    topic[51] = '\0';

                    /* Send the packet only to subscribed clients */
                    auto &subscribed_clients = topics[topic];

                    for (auto &client : subscribed_clients) {
                        /**
                         * When the client is connected, we send 
                         * the message instantly
                         */
                        if (client->connected)
                            send_message(message, client->fd);

                        /**
                         *  When the client is not connected, but wishes to get
                         *  the message, we store it
                         */
                        if (!client->connected && client->topics[topic]) {
                            ++message->c;
                            client->lost_messages.push_back(message);
                        }
                    }

                    if (!message->c)
                        delete message;
                } else {
                    tcp_request_t request;
                    int rc = recv_all(poll_fds[i].fd, &request, sizeof(request));

                    if (!rc) {
                        ids[request.id]->connected = false;
                        /* TODO: Remove the fd entry */
                    }

                    switch (request.type) {
                    case CONNECT: {
                         /**
                          * if there's already a connected client (with a
                          * different fd), reject the current connection
                          */
                        if (ids.count(request.id) && ids[request.id]->connected) {
                            printf("Client %s already connected.\n", request.id);
                            close(poll_fds[i].fd);
                        } else {
                            struct sockaddr_in sin;
                            socklen_t len = sizeof(sin);

                            int rc = getsockname(poll_fds[i].fd, (struct sockaddr *)&sin, &len);
                            DIE(rc < 0, "getsockname() failed");

                            printf("New client %s connected from %s:%d.\n", request.id, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
                            tcp_client_t *new_client = new tcp_client_t;
                            new_client->fd = poll_fds[i].fd;
                            new_client->id = request.id;
                            new_client->connected = true;

                            ids.insert({request.id, new_client});
                        }
                        break;
                    }

                    case SUBSCRIBE: {
                        /**
                         * if the client is at their first subscription,
                         * create an entry for them in the topics and ids
                         * database
                         */
                        tcp_client_t *client = ids[request.id];
                        if (!client->topics.count(request.subscribe.topic))
                            topics[request.subscribe.topic].push_back(client);

                        client->topics[request.subscribe.topic] = request.subscribe.sf;
                        break;
                    }

                    case UNSUBSCRIBE: {
                        tcp_client_t *client = ids[request.id];
                        auto &subs = topics[request.unsubscribe.topic];
                        subs.erase(std::find(subs.begin(), subs.end(), client));

                        client->topics.erase(request.unsubscribe.topic);
                        break;
                    }

                    case EXIT: {
                        printf("Client %s disconnected.\n", request.id);
                        close(poll_fds[i].fd);
                    }

                    default:
                        break;
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "\nUsage: %s <PORT_SERVER>\n", argv[0]);
        return 1;
    }

    uint16_t server_port;
    int rc = sscanf(argv[1], "%hu", &server_port);
    DIE(rc != 1, "sscanf() failed");

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket() failed");

    int udp_cli_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_cli_fd < 0, "socket() failed");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable,
            sizeof(int))
        < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(listenfd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind() failed");

    rc = bind(udp_cli_fd, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind() failed");

    listen(listenfd, MAX_CONNECTIONS);

    server(listenfd, udp_cli_fd);

    close(listenfd);

    return 0;
}
