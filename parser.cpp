#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <cmath>

#include "parser.h"
#include "common.h"

int parse_by_whitespace(char *buf, char **argv) {
	int argc = 0;
	for (char *p = strtok(buf, " \t\n"); p; p = strtok(NULL, " \t\n"))
		argv[argc++] = p;
	return argc;
}

void print_int(char *buff, char *topic) {
	int8_t sign = *buff;
	buff += sizeof(sign);

	int64_t nr = ntohl(*(u_int32_t *)buff);

	if (sign)
		nr = -nr;

	printf("%s - INT - %ld\n", topic, nr);
}

void print_short_real(char *buff, char *topic) {
	float nr = ntohs(*(u_int16_t *)buff) / (float)100;

	printf("%s - SHORT_REAL - %.2f\n", topic, nr);
}

void print_float(char *buff, char *topic) {
	int8_t sign = *buff;
	buff += sizeof(sign);

	float nr = ntohl(*(u_int32_t *)buff);
	buff += sizeof(uint32_t);

	if (sign)
		nr = -nr;

	int8_t pow10 = *buff;

	printf("%s - FLOAT - %f\n", topic, nr / (float) pow(10, pow10));
}

void parse_subscription(char *buff) {
	char topic[51] = { 0 };

	memcpy(topic, buff, 50);
	buff += 50;

	u_int8_t type = *buff;
	buff += sizeof(u_int8_t);

	switch (type) {
	case INT:
		print_int(buff, topic);
		break;
	case SHORT_REAL:
		print_short_real(buff, topic);
		break;

	case FLOAT:
		print_float(buff, topic);
		break;

	case STRING:
		printf("%s - STRING - %s\n", topic, buff);
		break;
	default:
		break;
	}
}
