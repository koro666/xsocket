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

	value = getenv("XBIND");
	if (!value || !*value)
		return;

	if (!strcmp(value, "*"))
	{
		xb_nport = -1;
		return;
	}

	AUTO_FREE char* buffer = strdup(value);

	size_t count = 0, capacity = 0;
	in_port_t* ports = NULL;

	const char* separator = " ";
	char *token, *dummy;
	for (token = strtok_r(buffer, separator, &dummy); token; token = strtok_r(NULL, separator, &dummy))
	{
		char* endptr;
		unsigned long port = strtoul(token, &endptr, 10);
		if (*endptr)
			continue;
		if (port <= 0 || port >= 0x10000)
			continue;

		if (count >= capacity)
		{
			capacity += 8;
			ports = realloc(ports, capacity * sizeof(in_port_t));
		}

		ports[count++] = port;
	}

	if (count < capacity)
		ports = realloc(ports, count * sizeof(in_port_t));

	xb_ports = ports;
	xb_nport = count;
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
