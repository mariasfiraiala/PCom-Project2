#ifndef __COMMON_H__
#define __COMMON_H__

#include <inttypes.h>

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

/* Dimensiunea maxima a mesajului */
#define MSG_MAXSIZE 1024

struct chat_packet {
  uint16_t len;
  char message[MSG_MAXSIZE + 1];
};

#endif
