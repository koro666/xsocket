#include "system.h"
#include "option.h"

volatile xs_sw_bitarray sw_opt_socket[XS_SW_OPTARRSIZE];
volatile xs_sw_bitarray sw_opt_ip[XS_SW_OPTARRSIZE];
volatile xs_sw_bitarray sw_opt_ip6[XS_SW_OPTARRSIZE];
volatile xs_sw_bitarray sw_opt_tcp[XS_SW_OPTARRSIZE];
volatile xs_sw_bitarray sw_opt_udp[XS_SW_OPTARRSIZE];
volatile socklen_t sw_optlen_max;

const struct xs_sw_optmap sw_optmap[] =
{
	{ SOL_SOCKET, sw_opt_socket },
	{ SOL_IP, sw_opt_ip },
	{ SOL_IPV6, sw_opt_ip6 },
	{ IPPROTO_TCP, sw_opt_tcp },
	{ IPPROTO_UDP, sw_opt_udp }
};

volatile setsockopt_fn sw_setsockopt;

__attribute__((visibility("default")))
int setsockopt(fd_t fd, int level, int optname, const void* optval, socklen_t optlen)
{
	int result = setsockopt_forward(fd, level, optname, optval, optlen);
	if (result < 0)
		return result;

	if (optname >= XS_SW_MAXOPTNAME)
		return result;

	const size_t count = sizeof(sw_optmap) / sizeof(struct xs_sw_optmap);
	const struct xs_sw_optmap* optmap = NULL;

	for (int i = 0; i < count; ++i)
	{
		const struct xs_sw_optmap* tmp = sw_optmap + i;
		if (tmp->level == level)
		{
			optmap = tmp;
			break;
		}
	}

	if (!optmap)
		return result;

	size_t index = optname / XS_SW_ARRINDBITS;
	xs_sw_bitarray mask = ((xs_sw_bitarray)1) << (optname % XS_SW_ARRINDBITS);

	__atomic_or_fetch(optmap->array + index, mask, __ATOMIC_RELEASE);

	do
	{
		socklen_t curmax = sw_optlen_max;
		if (curmax >= optlen)
			break;

		socklen_t newmax = optlen;
		if (__atomic_compare_exchange(&sw_optlen_max, &curmax, &newmax, false, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
			break;
	} while(true);

	return result;
}

int setsockopt_forward(fd_t fd, int level, int optname, const void* optval, socklen_t optlen)
{
	const setsockopt_fn invalid = (setsockopt_fn)(intptr_t)(-1);

	setsockopt_fn ptr = sw_setsockopt;
	if (__builtin_expect(!ptr, 0))
	{
		ptr = (setsockopt_fn)dlsym(RTLD_NEXT, "setsockopt");

		if (!ptr)
			ptr = invalid;

		sw_setsockopt = ptr;
	}

	if (__builtin_expect(ptr != invalid, 1))
		return ptr(fd, level, optname, optval, optlen);

	errno = ENOSYS;
	return -1;
}

int copysockopt(fd_t oldfd, fd_t newfd)
{
	const size_t count = sizeof(sw_optmap) / sizeof(struct xs_sw_optmap);
	socklen_t optlen_max = sw_optlen_max;

	for (size_t i = 0; i < count; ++i)
	{
		const struct xs_sw_optmap* optmap = sw_optmap + i;

		for (size_t j = 0; j < XS_SW_OPTARRSIZE; ++j)
		{
			uintptr_t bits = optmap->array[j];
			for (int k = 0; k < XS_SW_ARRINDBITS; ++k)
			{
				uintptr_t mask = ((xs_sw_bitarray)1) << k;
				if (!(bits & mask))
					continue;

				int optname = (j * XS_SW_ARRINDBITS) + k;
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
