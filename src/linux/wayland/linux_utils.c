#ifdef PLATFORM_LINUX
#include "linux_utils.h"
#include <time.h>

/*
 * https://wayland-book.com/surfaces/shared-memory.html
 * 
 * #define _POSIX_C_SOURCE 200112L
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>

void
LogErrnoExit(char *pFile, int line)
{
	YFATAL("%s:%d||%s",  pFile, line, strerror(errno));
	exit(1);
}

static void
NameRandom(char *pBuf)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long result = ts.tv_nsec;
	for (int i = 0; i < 6; ++i)
	{
		pBuf[i] = 'A' + (result&15) + (result&16) * 2;
		result >>=5;
	}
}

static int
ShmFileCreate(void)
{
	int retries = 100;
	do 
	{
		char pName[] = "/wl_shm-XXXXXX";
		NameRandom(pName + sizeof(pName) - 7);
		--retries;
		int fd = shm_open(pName, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0)
		{
			shm_unlink(pName);
			return fd;
		}
	} while(retries > 0 && errno == EEXIST);
	return -1;
}

int
ShmFileAllocate(size_t size)
{
	int fd = ShmFileCreate();
	if (fd < 0)
		return -1;
	int ret = 0;
	do 
	{
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);

	if (ret < 0)
	{
		close(fd);
		return -1;
	}
	return fd;
}
#endif // PLATFORM_LINUX
