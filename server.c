#include "system.h"
#include "server.h"
#include "socket.h"
#include "protocol.h"

#ifdef XSOCKET_SYSTEMD
bool xs_journal;
#endif

int main(int argc, char** argv)
{
#ifdef XSOCKET_SYSTEMD
	if ((xs_journal = xs_check_journal()))
		sd_journal_send("PRIORITY=%d", LOG_DEBUG, NULL);
#endif

	sigset_t sigmask;
	__attribute__((cleanup(xs_cleanup_restore)))
	sigset_t* restore = NULL;

	AUTO_CLOSE fd_t sigfd = -1;
	if (!xs_setup_signals(&sigmask, &restore, &sigfd))
		return EXIT_FAILURE;

	struct sockaddr_un address;
	AUTO_CLOSE fd_t srvfd = -1;
	__attribute__((cleanup(xs_cleanup_unlink)))
	char* path = NULL;

	if (!xs_setup_socket(argc, argv, &address, &srvfd, &path))
		return EXIT_FAILURE;

#ifdef XSOCKET_SYSTEMD
	sd_notify(0, "READY=1");
#endif

	bool parent = true;
	uint32_t signo = 0;

	while (!signo)
	{
		bool signaled, received;
		if (!xs_poll_sockets(sigfd, srvfd, &signaled, &received))
			return EXIT_FAILURE;

		if (signaled)
			signo = xs_read_signal(sigfd);

		if (!received)
			continue;

		if (!parent)
		{
			bool eof;
			if (!xs_handle_request(srvfd, &eof))
				return EXIT_FAILURE;
			if (eof)
				break;
			continue;
		}

		int split = xs_split(srvfd, &parent);
		if (parent)
			continue;

		path = NULL;
		restore = NULL;

		if (split < 0)
			return EXIT_FAILURE;
	}

#ifdef XSOCKET_SYSTEMD
	if (parent)
		sd_notify(0, "STOPPING=1");
#endif

	return !signo || signo == SIGTERM ? EXIT_SUCCESS : EXIT_FAILURE;
}

#ifdef XSOCKET_SYSTEMD
bool xs_check_journal(void)
{
	const char* value = getenv("JOURNAL_STREAM");
	if (!value || !*value)
		return false;

#ifdef _DEBUG
	if (!strcmp(value, "*"))
		return true;
#endif

	AUTO_FREE char* buffer = strdup(value);
	char* separator = strchr(buffer, ':');
	if (!separator)
		return false;

	*separator = 0;
	const char *sdev = buffer, *sino = separator + 1;
	if (!*sdev || !*sino)
		return false;

	char* endptr;
	unsigned long long dev = strtoull(sdev, &endptr, 10);
	if (*endptr)
		return false;

	unsigned long long ino = strtoull(sino, &endptr, 10);
	if (*endptr)
		return false;

	struct stat st;
	if (fstat(STDERR_FILENO, &st) < 0)
		return false;

	return st.st_dev == dev && st.st_ino == ino;
}

void xs_perror(const char* message)
{
	if (xs_journal)
		sd_journal_perror(message);
	else
		perror(message);
}

void xs_printf(int priority, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);

	if (xs_journal)
		sd_journal_printv(priority, format, ap);
	else
		vfprintf(stderr, format, ap);

	va_end(ap);
}
#endif

bool xs_setup_signals(sigset_t* omask, sigset_t** prestore, fd_t* pfd)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	if (sigprocmask(SIG_BLOCK, &mask, omask) < 0)
	{
		xs_perror("sigprocmask");
		return false;
	}

	*prestore = omask;

	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
	{
		xs_perror("signal");
		return false;
	}

	*pfd = signalfd(-1, &mask, SFD_CLOEXEC);
	if (*pfd < 0)
	{
		xs_perror("signalfd");
		return false;
	}

	return true;
}

bool xs_setup_socket(int argc, char* const* argv, struct sockaddr_un* address, fd_t* pfd, char** ppath)
{
	to_sockaddr_un(argc > 1 ? argv[1] : xsocket_default_address, address);
	socklen_t addrlen = sockaddr_un_len(address);

	AUTO_CLOSE fd_t fd = socket(AF_UNIX, SOCK_SEQPACKET|SOCK_CLOEXEC, 0);
	if (fd < 0)
	{
		xs_perror("socket");
		return false;
	}

	mode_t mask = umask(0);
	int result = bind(fd, (struct sockaddr*)address, addrlen);
	umask(mask);

	if (result < 0)
	{
		xs_perror("bind");
		return false;
	}

	if (*address->sun_path)
		*ppath = address->sun_path;

	int optval = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) < 0)
	{
		xs_perror("setsockopt");
		return false;
	}

	if (listen(fd, 32) < 0)
	{
		xs_perror("listen");
		return false;
	}

	*pfd = fd;
	fd = -1;
	return true;
}

bool xs_poll_sockets(fd_t sigfd, fd_t srvfd, bool* psig, bool* prcvd)
{
	*psig = false;
	*prcvd = false;

	struct pollfd fds[2] =
	{
		{ .fd = sigfd, .events = POLLIN },
		{ .fd = srvfd, .events = POLLIN }
	};

	int result;
	do { result = poll(fds, 2, -1); }
	while (result < 0 && errno == EINTR);

	if (result < 0)
	{
		xs_perror("poll");
		return false;
	}

	*psig = !!(fds[0].revents & POLLIN);
	*prcvd = !!(fds[1].revents & POLLIN);
	return true;
}

uint32_t xs_read_signal(fd_t sigfd)
{
	struct signalfd_siginfo fdsi;

	ssize_t size;
	do { size = read(sigfd, &fdsi, sizeof(struct signalfd_siginfo)); }
	while (size < 0 && errno == EINTR);

	if (size < 0)
	{
		xs_perror("read");
		return 0;
	}

	if (size < sizeof(struct signalfd_siginfo))
		return 0;

	xs_printf(LOG_INFO, "received signal %u\n", (unsigned int)fdsi.ssi_signo);
	return fdsi.ssi_signo;
}

bool xs_split(fd_t srvfd, bool* pparent)
{
	AUTO_CLOSE fd_t newfd = -1;
	do { newfd = accept4(srvfd, NULL, NULL, SOCK_CLOEXEC); }
	while (newfd < 0 && errno == EINTR);

	if (newfd < 0)
	{
		xs_perror("accept4");
		return false;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		xs_perror("fork");
		return false;
	}

	if (pid)
		return true;

	*pparent = false;

	if (dup3(newfd, srvfd, O_CLOEXEC) < 0)
	{
		xs_perror("dup3");
		return false;
	}

	return true;
}

bool xs_handle_request(fd_t srvfd, bool* peof)
{
	*peof = false;

	struct xs_protocol_request request;
	struct ucred cred;
	ssize_t received = recv_packet(srvfd, &request, sizeof(request), &cred, NULL, true);

	if (received < 0)
	{
		xs_perror("recv_packet");
		return false;
	}

	if (!received)
	{
		shutdown(srvfd, SHUT_WR);
		*peof = true;
		return true;
	}

	if (received < sizeof(request) || ntohl(request.signature) != XS_PROTOCOL_REQUEST)
	{
		xs_printf(LOG_WARNING, "truncated or invalid request: pid=%d, uid=%d, gid=%d\n", (int)cred.pid, (int)cred.uid, (int)cred.gid);
		return false;
	}

	int domain = ntohl(request.domain);
	int type = ntohl(request.type);
	int protocol = ntohl(request.protocol);

	xs_printf(LOG_DEBUG, "socket request: domain=%d, type=%d, protocol=%d, pid=%d, uid=%d, gid=%d\n",
		domain, type, protocol, (int)cred.pid, (int)cred.uid, (int)cred.gid);

	AUTO_CLOSE fd_t newfd = -1;
	type |= SOCK_CLOEXEC;
	int error = 0;

	newfd = socket(domain, type, protocol);
	if (newfd < 0)
	{
		error = errno;
		xs_perror("socket");
	}

	struct xs_protocol_response response =
	{
		.signature = htonl(XS_PROTOCOL_RESPONSE),
		.error = htonl(error)
	};

	ssize_t written = send_packet(srvfd, &response, sizeof(response), newfd);
	if (written < 0)
	{
		xs_perror("send_packet");
		return false;
	}

	if (written < sizeof(response))
		return false;

	return true;
}

void xs_cleanup_restore(sigset_t** ppmask)
{
	if (!*ppmask)
		return;

	sigprocmask(SIG_SETMASK, *ppmask, NULL);
	*ppmask = NULL;
}

void xs_cleanup_unlink(char** ppath)
{
	if (!*ppath)
		return;

	unlink(*ppath);
	*ppath = NULL;
}
