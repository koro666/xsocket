#include "system.h"
#include "cleanup.h"

void close_p(fd_t* pfd)
{
	if (*pfd < 0)
		return;
	close(*pfd);
	*pfd = -1;
}

void free_p(void* pp)
{
	void** xpp = (void**)pp;
	if (!*xpp)
		return;

	free(*xpp);
	*xpp = NULL;
}
