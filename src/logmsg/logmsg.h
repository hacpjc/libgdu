
#ifndef SRC_LGU_LOGMSG_LOGMSG_H_
#define SRC_LGU_LOGMSG_LOGMSG_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#define LOGMSG_ENABLE_SYSLOG (1)
#define LOGMSG_ENABLE_CONSOLE (1)
#define LOGMSG_ENABLE_BUGON (1)

#define LOGMSG_MB() do { } while (0)

/*!
 * write syslog msg
 *
 * \param level syslog level
 * \param fmt format string like printf(3)
 * \param ap  argument list
 */
static inline __attribute__((unused))
void logmsg_printf(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

#if LOGMSG_ENABLE_CONSOLE
	(void) vfprintf(stdout, fmt, ap);
#endif

#if LOGMSG_ENABLE_SYSLOG
	vsyslog(level, fmt, ap);
#endif

	va_end(ap);
}

/*!
 * Send logmsg once (or maybe twice...) to avoid annoying msg
 */
#define logmsg_printf_once(_level, _fmt, args...) \
	do { \
		static unsigned int __logmsg_once_stamp = 0; \
		if (__logmsg_once_stamp == 0) \
		{ \
			logmsg_printf(_level, _fmt, ##args); \
			__logmsg_once_stamp++; \
			LOGMSG_MB(); \
		} \
	} while (0)

/*!
 * print information syslog (LOG_INFO)
 *
 * \param fmt format string like printf(3)
 */
#define logmsg_info(_fmt, args...) logmsg_printf(LOG_INFO, _fmt "\n", ##args)

/*!
 * print information syslog (LOG_WARNING)
 *
 * \param fmt format string like printf(3)
 */
#define logmsg_warning(_fmt, args...) logmsg_printf(LOG_WARNING, "<%s> "_fmt "\n", __FUNCTION__, ##args)

/*!
 * print log_error syslog (LOG_ERR)
 *
 * \param fmt format string like printf(3)
 */
#define logmsg_err(_fmt, args...) \
	do { \
		logmsg_printf(LOG_ERR, "<%s> " _fmt "\n", __FUNCTION__, ##args); \
	} while (0)

#define logmsg_crit(_fmt, args...) \
	do { \
		logmsg_printf(LOG_CRIT, "<%s> " _fmt "\n", __FUNCTION__, ##args); \
	} while (0)

#define logmsg_abort(_fmt, args...) \
	do { \
		logmsg_printf(LOG_CRIT, "<%s> " _fmt "\n", __FUNCTION__, ##args); \
		abort(); \
	} while (0)

#if LOGMSG_ENABLE_BUGON
#define BUG() \
	do { \
		logmsg_abort("Oops! <%s:%d>\n", __FUNCTION__, __LINE__); \
	} while (0)

#define BUG_ON(_condition) \
	do { \
		if ((_condition)) { logmsg_abort(#_condition); } \
	} while (0)

#define WARN_ON(_condition) \
	do { \
		if ((_condition)) { \
			logmsg_warning("WARNING! %s:%d, %s\n", __FUNCTION__, __LINE__, #_condition); \
		} \
	} while (0)
#else
// Optimize-out
#define BUG() do { } while (0)
#define BUG_ON(_condition) do { int __useless__ = (_condition); } while (0)
#define WARN_ON(_condition) do { int __useless__ = (_condition); } while (0)
#endif

void logmsg_init(const char *name);
void logmsg_exit(void);

#endif /* SRC_LGU_LOGMSG_LOGMSG_H_ */
