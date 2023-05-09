#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"
#include "utils.h"

int recv_all(int sockfd, void *buffer, int len) {

	int bytes_received = 0;
	int bytes_remaining = len;
	char *buff = (char *)buffer;

  	while (bytes_remaining > 0) {
		int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
		DIE(rc == -1, "recv() failed");

		if (!rc)
			return 0;

		bytes_received += rc;
		bytes_remaining -= rc;
  	}

	return bytes_received;
}

int send_all(int sockfd, void *buffer, int len) {
	int bytes_sent = 0;
	int bytes_remaining = len;
	char *buff = (char *)buffer;

	while (bytes_remaining > 0) {
		int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
		DIE(rc == -1, "sent() failed");

		bytes_sent += rc;
		bytes_remaining -= rc;
	}

  	return bytes_sent;
}
