#include "system.h"
#include "socket.h"

void to_sockaddr_un(const char* name, struct sockaddr_un* sun)
{
	memset(sun, 0, sizeof(struct sockaddr_un));
	sun->sun_family = AF_UNIX;

	if (*name == '@')
		strncpy(sun->sun_path + 1, name + 1, sizeof(sun->sun_path) - 1);
	else
		strlcpy(sun->sun_path, name, sizeof(sun->sun_path));
}

size_t sockaddr_un_len(const struct sockaddr_un* sun)
{
	if (sun->sun_path[0])
		return offsetof(struct sockaddr_un, sun_path) + strlen(sun->sun_path) + 1;

	if (sun->sun_path[1])
		return offsetof(struct sockaddr_un, sun_path) + 1 + strnlen(sun->sun_path + 1, sizeof(sun->sun_path) - 1);

	return sizeof(sa_family_t);
}

ssize_t send_packet(fd_t sockfd, const void* data, size_t size, fd_t sendfd)
{
	struct iovec iov = { (void*)data, size };

	size_t ctrlsz = sendfd < 0 ? 0 : CMSG_SPACE(sizeof(fd_t));
	alignas(struct cmsghdr) uint8_t control[ctrlsz];

	struct msghdr msg =
	{
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};

	if (ctrlsz)
	{
		memset(control, 0, ctrlsz);
		msg.msg_control = control;
		msg.msg_controllen = ctrlsz;

		struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(fd_t));
		memcpy(CMSG_DATA(cmsg), &sendfd, sizeof(fd_t));
	}

	ssize_t result;
	do { result = sendmsg(sockfd, &msg, MSG_NOSIGNAL); }
	while (result < 0 && errno == EINTR);

	return result;
}

ssize_t recv_packet(fd_t sockfd, void* buffer, size_t size, struct ucred* cred, fd_t* recvfd, bool cloexec)
{
	memset(buffer, 0, size);
	if (cred)
		memset(cred, -1, sizeof(struct ucred));
	if (recvfd)
		*recvfd = -1;

	struct iovec iov = { buffer, size };

	size_t ctrlsz = 0;
	if (cred)
		ctrlsz = CMSG_SPACE(sizeof(struct ucred));
	if (recvfd)
		ctrlsz = CMSG_ALIGN(ctrlsz) + CMSG_SPACE(sizeof(fd_t));

	alignas(struct cmsghdr) uint8_t control[ctrlsz];

	struct msghdr msg =
	{
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = ctrlsz ? control : NULL,
		.msg_controllen = ctrlsz
	};

	ssize_t received;
	do { received = recvmsg(sockfd, &msg, cloexec ? MSG_CMSG_CLOEXEC : 0); }
	while (received < 0 && errno == EINTR);

	if (received < 0)
		return received;

	for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
	{
		if (cmsg->cmsg_level != SOL_SOCKET)
			continue;

		uint8_t* cmsg_data = (uint8_t*)CMSG_DATA(cmsg);
		size_t cmsg_data_len = cmsg->cmsg_len - (cmsg_data - (uint8_t*)cmsg);

		if (cmsg->cmsg_type == SCM_CREDENTIALS)
		{
			if (cred)
				memcpy(cred, cmsg_data, MIN(cmsg_data_len, sizeof(struct ucred)));
		}
		else if (cmsg->cmsg_type == SCM_RIGHTS)
		{
			size_t nfds = cmsg_data_len / sizeof(fd_t);
			fd_t* fds = (fd_t*)cmsg_data;

			if (recvfd)
			{
				for (size_t i = 0; i < nfds; ++i)
				{
					close_p(recvfd);
					*recvfd = fds[i];
				}
			}
			else
			{
				for (size_t i = 0; i < nfds; ++i)
					close(fds[i]);
			}
		}
	}

	return received;
}
