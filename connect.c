#include "system.h"
#include "connect.h"
#include "cleanup.h"
#include "address.h"
#include "xsocket.h"
#include "switch.h"

char* xsocket_address;
ssize_t xc_nport;
in_port_t* xc_ports;
volatile connect_fn xc_connect;

__attribute__((constructor))
void xconnect_initialize(void)
{
	const char* value = getenv("XSOCKET");
	xsocket_address = value && *value ? strdup(value) : NULL;
	xc_nport = parse_port_list(getenv("XCONNECT"), &xc_ports);
}

__attribute__((visibility("default")))
int connect(fd_t sockfd, const struct sockaddr* address, socklen_t addrlen)
{
	if (check_try_switch(xsocket_address, sockfd, address, addrlen, xc_ports, xc_nport) < 0)
		return -1;

	return xconnect_forward(sockfd, address, addrlen);
}

int xconnect_forward(fd_t sockfd, const struct sockaddr* address, socklen_t addrlen)
{
	const connect_fn invalid = (connect_fn)(intptr_t)(-1);

	connect_fn ptr = xc_connect;
	if (__builtin_expect(!ptr, 0))
	{
		ptr = (connect_fn)dlsym(RTLD_NEXT, "connect");

		if (!ptr)
			ptr = invalid;

		xc_connect = ptr;
	}

	if (__builtin_expect(ptr != invalid, 1))
		return ptr(sockfd, address, addrlen);

	errno = ENOSYS;
	return -1;
}

__attribute__((destructor))
void xconnect_terminate(void)
{
	free(xc_ports);
	xc_ports = NULL;
	xc_nport = 0;

	free(xsocket_address);
	xsocket_address = NULL;
}
