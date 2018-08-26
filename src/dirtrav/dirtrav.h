
#ifndef DIRTRAV_H_
#define DIRTRAV_H_

#include <stdint.h>

typedef int (*dirtrav_cb_handle_t)(const char *path, void *arg);
typedef int (*dirtrav_cb_filter_t)(const char *path, void *arg);

#define DIRTRAV_FLAG_STOP_AT_ERROR (1 << 1)
#define DIRTRAV_FLAG_FOLLOW_LINK   (1 << 2)
#define DIRTRAV_FLAG_DFL           (0)

#define DIRTRAV_FLAG_DEL(_flag, _del)    do { (_flag) = (_flag) & (~(_del)); } while (0)
#define DIRTRAV_FLAG_ADD(_flag, _add)    do { (_flag) |= (_add); } while (0)
#define DIRTRAV_FLAG_CHECK(_flag, _test) (((_flag) & (_test)) == (_test))

typedef uint8_t dirtrav_flag_t;

int dirtrav(const char *path, const dirtrav_flag_t flag,
	const dirtrav_cb_handle_t cb_handle, const dirtrav_cb_filter_t cb_filter, void *arg);

#endif /* DIRTRAV_H_ */
