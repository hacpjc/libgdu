#ifndef SRC_LIB_SYSCONF_SYSCONF_H_
#define SRC_LIB_SYSCONF_SYSCONF_H_

#include <unistd.h>

static inline __attribute__((unused))
unsigned int sysconf_get_hz(void)
{
	long int ret = -1;
	static unsigned int hz = 0;

	if (hz == 0)
	{
		/* Get system HZ. */
		ret = sysconf(_SC_CLK_TCK);
		if (ret < 0)
		{
			hz = 100;
		}
		else
		{
			hz = (unsigned int) ret;
		}
	}

	return hz;
}

#endif /* SRC_LIB_SYSCONF_SYSCONF_H_ */
