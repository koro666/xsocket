#include "system.h"
#include "switch.h"
#include "option.h"

bool switcheroo(fd_t oldfd, fd_t newfd)
{
	if (copysockopt(oldfd, newfd) < 0)
		return false;

	int flags = fcntl(oldfd, F_GETFD, 0);
	if (flags < 0)
		return false;

	if (dup3(newfd, oldfd, flags & FD_CLOEXEC ? O_CLOEXEC : 0) < 0)
		return false;

	return true;
}
