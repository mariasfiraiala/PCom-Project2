#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <bits/stdc++.h>

#define MAX_PFDS 32
#define MAX_CONNECTIONS 512

#define DIE(assertion, call_description)                                       \
    do {                                                                       \
        if (assertion) {                                                       \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                 \
            perror(call_description);                                          \
            exit(errno);                                                       \
        }                                                                      \
    } while (0)

struct stored_message_t {
    int c;
    int len;
    char *buff;
} __attribute__ ((__packed__));

struct tcp_client_t {
    int fd;
    std::string id;
    bool connected;
    std::map<std::string, bool> topics;
    std::vector<stored_message_t *> lost_messages;
};

#endif
