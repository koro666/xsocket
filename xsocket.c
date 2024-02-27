#include "system.h"
#include "xsocket.h"
#include "cleanup.h"
#include "socket.h"
#include "protocol.h"

__attribute__((visibility("default")))
int xsocket(const char* server, int domain, int type, int protocol)
{
	if (!server)
		server = xsocket_default_address;

	AUTO_CLOSE fd_t srvfd = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC, 0);
	if (srvfd < 0)
		return -1;

	struct sockaddr_un srvaddr;
	to_sockaddr_un(server, &srvaddr);
	socklen_t srvaddrlen = sockaddr_un_len(&srvaddr);

	if (connect(srvfd, (struct sockaddr*)&srvaddr, srvaddrlen) < 0)
		return -1;

	bool cloexec = !!(type & SOCK_CLOEXEC);
	type &= ~SOCK_CLOEXEC;

	struct xs_protocol_request request =
	{
		.signature = htonl(XS_PROTOCOL_REQUEST),
		.domain = htonl(domain),
		.type = htonl(type),
		.protocol = htonl(protocol)
	};

	ssize_t written = send_packet(srvfd, &request, sizeof(request), -1);
	if (written < 0)
		return -1;

	if (written < sizeof(request))
	{
		errno = EMSGSIZE;
		return -1;
	}

	if (shutdown(srvfd, SHUT_WR) < 0)
		return -1;

	struct xs_protocol_response response;
	AUTO_CLOSE fd_t rcvfd = -1;

	ssize_t received = recv_packet(srvfd, &response, sizeof(response), NULL, &rcvfd, cloexec);
	if (received < 0)
		return -1;

	if (received < sizeof(response) || ntohl(response.signature) != XS_PROTOCOL_RESPONSE)
	{
		errno = EINVAL;
		return -1;
	}

	if (response.error)
	{
		errno = ntohl(response.error);
		return -1;
	}

	if (rcvfd < 0)
	{
		errno = EPERM;
		return -1;
	}

	fd_t resfd = rcvfd;
	rcvfd = -1;

	errno = 0;
	return resfd;
}
