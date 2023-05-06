#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

int send_all(int sockfd, void *buff, size_t len);

int recv_all(int sockfd, void *buff, size_t len);

/* line max size */
#define MSG_MAXSIZE 1024

struct subscribe_t {
  char topic[51];
  bool sf;
} __attribute__ ((__packed__));

struct unsubscribe_t {
  char topic[51];
} __attribute__ ((__packed__));

enum request_type {
    EXIT,
    SUBSCRIBE,
    UNSUBSCRIBE,
    CONNECT
};

struct tcp_request_t {
  char id[11];
  union {
    subscribe_t subscribe;
    unsubscribe_t unsubscribe;
  };
  request_type type;
} __attribute__ ((__packed__));

enum data_type {
  INT = 0,
  SHORT_REAL = 1,
  FLOAT = 2,
  STRING = 3
};

#endif
