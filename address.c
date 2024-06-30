#include "system.h"
#include "address.h"

ssize_t parse_port_list(const char* value, in_port_t** ports)
{
	*ports = NULL;
	if (!value || !*value)
		return 0;

	if (!strcmp(value, "*"))
		return -1;

	AUTO_FREE char* buffer = strdup(value);

	size_t count = 0, capacity = 0;
	in_port_t* array = NULL;

	const char separator[] = " ";
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
			array = realloc(array, capacity * sizeof(in_port_t));
		}

		array[count++] = port;
	}

	if (count < capacity)
		array = realloc(array, count * sizeof(in_port_t));

	*ports = array;
	return count;
}

int check_socket(fd_t sockfd, int* domain, int* type, int* protocol)
{
	socklen_t length;

	*domain = 0;
	*type = 0;
	*protocol = 0;

	length = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_DOMAIN, domain, &length) < 0)
		return -1;

	if (*domain != AF_INET && *domain != AF_INET6)
		return 0;

	length = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_TYPE, type, &length) < 0)
		return -1;

	if (*type != SOCK_STREAM && *type != SOCK_DGRAM)
		return 0;

	length = sizeof(int);
	if (getsockopt(sockfd, SOL_SOCKET, SO_PROTOCOL, protocol, &length) < 0)
		return -1;

	if (*protocol != IPPROTO_TCP && *protocol != IPPROTO_UDP)
		return 0;

	return 1;
}

bool check_address(const struct sockaddr* address, socklen_t addrlen, const in_port_t* ports, ssize_t nports)
{
	in_port_t port;
	if (address->sa_family == AF_INET && addrlen >= sizeof(struct sockaddr_in))
		port = ntohs(((struct sockaddr_in*)address)->sin_port);
	else if (address ->sa_family == AF_INET6 && addrlen >= sizeof(struct sockaddr_in6))
		port = ntohs(((struct sockaddr_in6*)address)->sin6_port);
	else
		return false;

	if (nports < 0)
		return true;

	for (ssize_t i = 0; i < nports; ++i)
	{
		if (ports[i] == port)
			return true;
	}

	return false;
}
