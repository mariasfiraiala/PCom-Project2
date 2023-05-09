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
#include "parser.h"

std::map<std::string, tcp_client_t *> ids;
std::map<std::string, std::vector<tcp_client_t *>> topics;
std::map<int, std::pair<in_addr, uint16_t>> ips_ports;

void send_message(stored_message_t *message, int fd) {
    send_all(fd, &message->len, sizeof(message->len));
    send_all(fd, message->buff, message->len);
}

void server(int listenfd, int udp_cli_fd) {
    std::vector<struct pollfd> poll_fds;

    poll_fds.push_back(pollfd{listenfd, POLLIN, 0});
    poll_fds.push_back(pollfd{udp_cli_fd, POLLIN, 0});
    poll_fds.push_back(pollfd{STDIN_FILENO, POLLIN, 0});

    while (1) {
        int rc = poll(&poll_fds[0], poll_fds.size(), -1);
		DIE(rc < 0, "poll");

        for (uint64_t i = 0; i < poll_fds.size(); ++i) {
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

                    int enable = 1;
                    setsockopt(tcp_cli_fd, SOL_SOCKET,
                            SO_REUSEADDR | TCP_NODELAY, &enable, sizeof(int));
                    DIE(tcp_cli_fd < 0, "setsockopt failed");

                    /**
                     * Save the IP address and port for when the client wishes
                     * to connect later on. They are used to print its
                     * "credentials".
                     */
                    ips_ports[tcp_cli_fd] = std::make_pair(tcp_cli_addr.sin_addr,
                                                           tcp_cli_addr.sin_port);

                    /**
                     * Add the newly created tcp client
                     * to the poll list of fds
                     */
                    poll_fds.push_back(pollfd{tcp_cli_fd, POLLIN, 0});
                } else if (poll_fds[i].fd == udp_cli_fd) {
                    char buff[2 * MSG_MAXSIZE];

                    struct sockaddr udp_cli_addr;
                    socklen_t udp_cli_len = sizeof(udp_cli_addr);

                    int rc = recvfrom(poll_fds[i].fd, buff, 2 * MSG_MAXSIZE, 0,
                                      &udp_cli_addr, &udp_cli_len);

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
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    char buff[MSG_MAXSIZE], *argv[MSG_MAXSIZE];

                    fgets(buff, MSG_MAXSIZE, stdin);
                    int argc = parse_by_whitespace(buff, argv);

                    if (argc == 1 && !strcmp(argv[0], "exit"))
                        for (auto & p : poll_fds) {
                            close(p.fd);
                            return;
                        }
                } else {
                    tcp_request_t request;
                    recv_all(poll_fds[i].fd, &request, sizeof(request));

                    switch (request.type) {
                    case CONNECT: {
                        if (ids.count(request.id)) {
                            /**
                             * If there's already a connected client (with the
                             * same id), reject the current connection
                             */
                            if (ids[request.id]->connected) {
                                printf("Client %s already connected.\n", request.id);
                                close(poll_fds[i].fd);
                                poll_fds.erase(poll_fds.begin() + i);
                                ips_ports.erase(poll_fds[i].fd);
                            } else {
                                /**
                                 * If the client is stored in the id database,
                                 * but it's not online, reconnect it
                                 */
                                printf("New client %s connected from %hu:%s.\n",
                                       request.id,
                                       ntohs(ips_ports[poll_fds[i].fd].second),
                                       inet_ntoa(ips_ports[poll_fds[i].fd].first));

                                auto client = ids[request.id];
                                client->connected = true;

                                /**
                                 * Send messages from waiting queue, when the
                                 * client gets connected again 
                                 */
                                for (auto &message : client->lost_messages) {
                                    send_message(message, client->fd);

                                    --message->c;

                                    if (!message->c)
                                        delete message;
                                }
                                client->lost_messages.clear();
                            }
                        } else {
                            printf("New client %s connected from %hu:%s.\n",
                                   request.id,
                                   ntohs(ips_ports[poll_fds[i].fd].second),
                                   inet_ntoa(ips_ports[poll_fds[i].fd].first));

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
                         * If the client is at their first subscription,
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
                        ids[request.id]->connected = false;
                        close(poll_fds[i].fd);
                        poll_fds.erase(poll_fds.begin() + i);
                        break;
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

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    uint16_t server_port;
    int rc = sscanf(argv[1], "%hu", &server_port);
    DIE(rc != 1, "sscanf() failed");

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket() failed");

    int udp_cli_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_cli_fd < 0, "socket() failed");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

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
