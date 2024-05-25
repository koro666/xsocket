#pragma once
#include "cleanup.h"

typedef uintptr_t xs_sw_bitarray;

struct xs_sw_optmap
{
	int level;
	volatile xs_sw_bitarray* array;
};

#define XS_SW_MAXOPTNAME 256
#define XS_SW_ARRINDBITS (sizeof(xs_sw_bitarray) * 8)
#define XS_SW_OPTARRSIZE (XS_SW_MAXOPTNAME / XS_SW_ARRINDBITS)

extern volatile xs_sw_bitarray sw_opt_socket[XS_SW_OPTARRSIZE];
extern volatile xs_sw_bitarray sw_opt_ip[XS_SW_OPTARRSIZE];
extern volatile xs_sw_bitarray sw_opt_ip6[XS_SW_OPTARRSIZE];
extern volatile xs_sw_bitarray sw_opt_tcp[XS_SW_OPTARRSIZE];
extern volatile xs_sw_bitarray sw_opt_udp[XS_SW_OPTARRSIZE];
extern volatile socklen_t sw_optlen_max;

extern const struct xs_sw_optmap sw_optmap[];

typedef int (*setsockopt_fn)(int, int, int, const void*, socklen_t);

int setsockopt_forward(fd_t, int, int, const void*, socklen_t);

bool switcheroo(fd_t, fd_t);
