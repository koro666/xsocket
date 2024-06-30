#include "system.h"
#include "hook.h"
#include "cleanup.h"
#include "socket.h"
#include "protocol.h"
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
	int domain, type, protocol;
	int check = check_socket(sockfd, &domain, &type, &protocol);
	if (check < 0)
		return -1;

	if (!check || !check_address(address, addrlen, xb_ports, xb_nport))
		return xbind_forward(sockfd, address, addrlen);

	type |= SOCK_CLOEXEC;

	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		return -1;

	if (flags & O_NONBLOCK)
		type |= SOCK_NONBLOCK;

	AUTO_CLOSE fd_t newfd = xsocket(xsocket_address, domain, type, protocol);
	if (newfd < 0)
		return -1;

	if (!switcheroo(sockfd, newfd))
		return -1;

	close_p(&newfd);
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
