
#ifndef SRC_LGU_UNIXSOCK_UNIXSOCK_H_
#define SRC_LGU_UNIXSOCK_UNIXSOCK_H_

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <sys/un.h>

#include <sys/types.h>
#include <sys/socket.h>

static __attribute__((unused))
int unixsock_open(const char *path)
{
	int fd;

	if (access(path, R_OK | W_OK))
	{
		return -1;
	}

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		return fd;
	}

	{
		struct sockaddr_un server;

		server.sun_family = AF_UNIX;
		snprintf(server.sun_path, sizeof(server.sun_path), "%s", path);

		if (strlen(server.sun_path) != strlen(path))
		{
			goto error_exit;
		}

		if (connect(fd, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0)
		{
			goto error_exit;
		}
	}

	return fd;

error_exit:
	if (fd)
	{
		close(fd);
	}

	return -1;
}

static __attribute__((unused))
int unixsock_accept(const int srvfd)
{
	int clifd = accept(srvfd, NULL, NULL);
	return clifd;
}

static __attribute__((unused))
int unixsock_recv(const int fd, uint8_t *buf, const unsigned int buf_sz)
{
	int res = 0;

again:
	res = recv(fd, buf, buf_sz, 0);
	if (res < 0)
	{
		if (errno == EAGAIN || errno == EINTR)
		{
			fprintf(stderr, "again\n");
			goto again;
		}
	}

	return res;
}

static __attribute__((unused))
int unixsock_send(const int fd, uint8_t *buf, const unsigned int buf_sz)
{
	int ret;
	int consumed = 0;

	while (1)
	{
		ret = send(fd, buf, buf_sz, MSG_NOSIGNAL);
		if (ret < 0)
		{
			return ret;
		}

		consumed += ret;

		if (consumed == buf_sz)
		{
			break;
		}
	}

	return consumed;
}

static __attribute__((unused))
void unixsock_close(const int fd)
{
	if (fd < 0)
	{
		return;
	}

	shutdown(fd, SHUT_RDWR);
	close(fd);
}

#endif /* SRC_LGU_UNIXSOCK_UNIXSOCK_H_ */
