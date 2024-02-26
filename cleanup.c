#include "system.h"
#include "cleanup.h"

void close_p(fd_t* pfd)
{
	if (*pfd < 0)
		return;

	int e = errno;
	close(*pfd);
	*pfd = -1;
	errno = e;
}

void free_p(void* pp)
{
	void** xpp = (void**)pp;
	if (!*xpp)
		return;

	int e = errno;
	free(*xpp);
	*xpp = NULL;
	errno = e;
}
