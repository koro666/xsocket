#include "system.h"
#include "bind.h"
#include "cleanup.h"
#include "address.h"
#include "xsocket.h"
#include "switch.h"

char* xsocket_address;
ssize_t xb_nport;
in_port_t* xb_ports;
volatile bind_fn xb_bind;

__attribute__((constructor))
void xbind_initialize(void)
{
	const char* value = getenv("XSOCKET");
	xsocket_address = value && *value ? strdup(value) : NULL;
	xb_nport = parse_port_list(getenv("XBIND"), &xb_ports);
}

__attribute__((visibility("default")))
int bind(fd_t sockfd, const struct sockaddr* address, socklen_t addrlen)
{
	if (check_try_switch(xsocket_address, sockfd, address, addrlen, xb_ports, xb_nport) < 0)
		return -1;

	return xbind_forward(sockfd, address, addrlen);
}

int xbind_forward(fd_t sockfd, const struct sockaddr* address, socklen_t addrlen)
{
	const bind_fn invalid = (bind_fn)(intptr_t)(-1);

	bind_fn ptr = xb_bind;
	if (__builtin_expect(!ptr, 0))
	{
		ptr = (bind_fn)dlsym(RTLD_NEXT, "bind");

		if (!ptr)
			ptr = invalid;

		xb_bind = ptr;
	}

	if (__builtin_expect(ptr != invalid, 1))
		return ptr(sockfd, address, addrlen);

	errno = ENOSYS;
	return -1;
}

__attribute__((destructor))
void xbind_terminate(void)
{
	free(xb_ports);
	xb_ports = NULL;
	xb_nport = 0;

	free(xsocket_address);
	xsocket_address = NULL;
}
