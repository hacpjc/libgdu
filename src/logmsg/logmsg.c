#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include "logmsg.h"

/*!
 * open syslog
 *
 * \param name
 */
void logmsg_init(const char *name)
{
	openlog(name, LOG_NDELAY | LOG_NOWAIT, LOG_DAEMON);
}

void logmsg_exit(void)
{
	closelog();
}

