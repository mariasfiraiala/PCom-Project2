#ifndef __PARSER_H__
#define __PARSER_H__

/**
 * Parses by whitespace buffer read from stdin.
 *
 * @param buf
 *	String that needs to be interpreted
 * @param argv
 *  Array of strings with non-whitespace components
 * @return
 *	The number of the tokens in argv
 */
int parse_by_whitespace(char *buf, char **argv);

/**
 * Parses buffer received on the network.
 *
 * @param buf
 *	String storing the subscription message
 * @return
 *	None
 */
void parse_subscription(char *buff);

#endif
