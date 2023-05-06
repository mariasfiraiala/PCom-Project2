#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>

#define BUFSIZE 1024

int parse_by_whitespace(char *buf, char **argv);

char *argv_to_string(char **argv, int start, int end);

void parse_subscription(char *buff);

#endif
