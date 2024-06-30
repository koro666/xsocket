#include "system.h"
#include "switch.h"
#include "cleanup.h"
#include "address.h"
#include "option.h"
#include "xsocket.h"

int check_try_switch(const char* server, fd_t sockfd, const struct sockaddr* address, socklen_t addrlen, const in_port_t* ports, ssize_t nports)
{
	int domain, type, protocol;
	int check = check_socket(sockfd, &domain, &type, &protocol);
	if (check <= 0)
		return check;

	if (!check_address(address, addrlen, ports, nports))
		return 0;

	type |= SOCK_CLOEXEC;

	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
		return -1;

	if (flags & O_NONBLOCK)
		type |= SOCK_NONBLOCK;

	AUTO_CLOSE fd_t newfd = xsocket(server, domain, type, protocol);
	if (newfd < 0)
		return -1;

	if (copysockopt(sockfd, newfd) < 0)
		return -1;

	flags = fcntl(sockfd, F_GETFD, 0);
	if (flags < 0)
		return -1;

	if (dup3(newfd, sockfd, flags & FD_CLOEXEC ? O_CLOEXEC : 0) < 0)
		return -1;

	return 1;
}
