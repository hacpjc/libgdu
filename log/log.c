/*!
 * \file log.c
 * \brief
 *
 * \date Create: 2012/4/6
 * \author hac Ping-Jhih Chen
 * \sa log.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include "log.h"

/*!
 * internal flag to identify if we already open the syslog or not
 */
static int log_on_syslog = 0;

/*!
 * open syslog
 *
 * \param tag
 */
extern void log_init(const char *name)
{
	openlog(name, LOG_NDELAY, LOG_DAEMON);
	++log_on_syslog;
}

/*!
 * write syslog msg
 *
 * \param level syslog level
 * \param fmt format string like printf(3)
 * \param ap  argument list
 */
static void log_printf(int level, const char *fmt, va_list ap)
{
	if (log_on_syslog) {
		vsyslog(level, fmt, ap);
	} else {
		(void) vfprintf(stderr, fmt, ap);

		if (strchr(fmt, '\n') != NULL) {
			fprintf(stderr, "\n");
		}
	}
}

/*!
 * print information syslog (LOG_INFO)
 *
 * \param fmt format string like printf(3)
 */
extern void log_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_printf(LOG_INFO, fmt, ap);
	va_end(ap);
}

/*!
 * print log_error syslog (LOG_ERR)
 *
 * \param fmt format string like printf(3)
 */
extern void log_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_printf(LOG_ERR, fmt, ap);
	va_end(ap);
}

/*!
 * print log_fatal log_error msg and exit the program (LOG_ERR)
 *
 * \param fmt format string like printf(3)
 */
extern void log_fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_printf(LOG_ERR, fmt, ap);
	va_end(ap);

	exit(-1);
}
