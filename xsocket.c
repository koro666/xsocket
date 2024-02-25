#include "system.h"
#include "xsocket.h"

__attribute__((visibility("default")))
int xsocket(const char* server, int domain, int type, int protocol)
{
	errno = ENOSYS;
	return -1;
}
