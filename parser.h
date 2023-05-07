#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>

#define BUFSIZE 1024

int parse_by_whitespace(char *buf, char **argv);

void parse_subscription(char *buff);

#endif
