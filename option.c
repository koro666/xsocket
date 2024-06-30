#include "system.h"
#include "option.h"

volatile xs_option_bitarray xo_socket[XS_OPTION_ARRSIZE];
volatile xs_option_bitarray xo_ip[XS_OPTION_ARRSIZE];
volatile xs_option_bitarray xo_ip6[XS_OPTION_ARRSIZE];
volatile xs_option_bitarray xo_tcp[XS_OPTION_ARRSIZE];
volatile xs_option_bitarray xo_udp[XS_OPTION_ARRSIZE];
volatile socklen_t xo_optlen_max;

const struct xs_option_map xo_optmap[] =
{
	{ SOL_SOCKET, xo_socket },
	{ SOL_IP, xo_ip },
	{ SOL_IPV6, xo_ip6 },
	{ IPPROTO_TCP, xo_tcp },
	{ IPPROTO_UDP, xo_udp }
};

volatile setsockopt_fn xo_setsockopt;

__attribute__((visibility("default")))
int setsockopt(fd_t fd, int level, int optname, const void* optval, socklen_t optlen)
{
	int result = setsockopt_forward(fd, level, optname, optval, optlen);
	if (result < 0)
		return result;

	if (optname >= XS_OPTION_MAXNAME)
		return result;

	const size_t count = sizeof(xo_optmap) / sizeof(struct xs_option_map);
	const struct xs_option_map* optmap = NULL;

	for (int i = 0; i < count; ++i)
	{
		const struct xs_option_map* tmp = xo_optmap + i;
		if (tmp->level == level)
		{
			optmap = tmp;
			break;
		}
	}

	if (!optmap)
		return result;

	size_t index = optname / XS_OPTION_ARRBITS;
	xs_option_bitarray mask = ((xs_option_bitarray)1) << (optname % XS_OPTION_ARRBITS);

	__atomic_or_fetch(optmap->array + index, mask, __ATOMIC_RELEASE);

	do
	{
		socklen_t curmax = xo_optlen_max;
		if (curmax >= optlen)
			break;

		socklen_t newmax = optlen;
		if (__atomic_compare_exchange(&xo_optlen_max, &curmax, &newmax, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
			break;
	} while(true);

	return result;
}

int setsockopt_forward(fd_t fd, int level, int optname, const void* optval, socklen_t optlen)
{
	const setsockopt_fn invalid = (setsockopt_fn)(intptr_t)(-1);

	setsockopt_fn ptr = xo_setsockopt;
	if (__builtin_expect(!ptr, 0))
	{
		ptr = (setsockopt_fn)dlsym(RTLD_NEXT, "setsockopt");

		if (!ptr)
			ptr = invalid;

		xo_setsockopt = ptr;
	}

	if (__builtin_expect(ptr != invalid, 1))
		return ptr(fd, level, optname, optval, optlen);

	errno = ENOSYS;
	return -1;
}

int copysockopt(fd_t oldfd, fd_t newfd)
{
	const size_t count = sizeof(xo_optmap) / sizeof(struct xs_option_map);
	socklen_t optlen_max = xo_optlen_max;

	for (size_t i = 0; i < count; ++i)
	{
		const struct xs_option_map* optmap = xo_optmap + i;

		for (size_t j = 0; j < XS_OPTION_ARRSIZE; ++j)
		{
			uintptr_t bits = optmap->array[j];
			for (int k = 0; k < XS_OPTION_ARRBITS; ++k)
			{
				uintptr_t mask = ((xs_option_bitarray)1) << k;
				if (!(bits & mask))
					continue;

				int optname = (j * XS_OPTION_ARRBITS) + k;
				uint8_t buffer[optlen_max];
				socklen_t size = optlen_max;

				if (getsockopt(oldfd, optmap->level, optname, buffer, &size) < 0)
					continue;

				if (setsockopt_forward(newfd, optmap->level, optname, buffer, size) < 0)
					continue;
			}
		}
	}

	return 0;
}
