/*!
 * \file log.h
 * \brief
 *
 * \date Create: 2012/4/6
 * \author hac Ping-Jhih Chen
 *
 * \details
 * \note Please start a local syslog server (e.g. rsyslogd) before sending any log message.
 *
 * \par Example:
 * An example of codes to use this syslog interface.
 * \code
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"

int main(int argc, char **argv)
{
	log_init(argv[0]); // use command name as the log name

	log_info("Print a message on syslog");	
	log_error("This is an error message");

	log_fatal("Print an error message and exit the program");

	printf("never print\n");

	return 0;
}
 * \endcode
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

extern void log_init(const char *name);

extern void log_info(const char *fmt, ...);
extern void log_error(const char *fmt, ...);
extern void log_fatal(const char *fmt, ...);

#endif /* LOG_H_ */
