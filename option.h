#pragma once
#include "cleanup.h"

typedef uintptr_t xs_option_bitarray;

struct xs_option_map
{
	int level;
	volatile xs_option_bitarray* array;
};

#define XS_OPTION_MAXNAME 256
#define XS_OPTION_ARRBITS (sizeof(xs_option_bitarray) * 8)
#define XS_OPTION_ARRSIZE (XS_OPTION_MAXNAME / XS_OPTION_ARRBITS)

extern volatile xs_option_bitarray xo_socket[XS_OPTION_ARRSIZE];
extern volatile xs_option_bitarray xo_ip[XS_OPTION_ARRSIZE];
extern volatile xs_option_bitarray xo_ip6[XS_OPTION_ARRSIZE];
extern volatile xs_option_bitarray xo_tcp[XS_OPTION_ARRSIZE];
extern volatile xs_option_bitarray xo_udp[XS_OPTION_ARRSIZE];
extern volatile socklen_t xo_optlen_max;

extern const struct xs_option_map xo_optmap[];

typedef int (*setsockopt_fn)(int, int, int, const void*, socklen_t);

int setsockopt_forward(fd_t, int, int, const void*, socklen_t);
int copysockopt(fd_t, fd_t);
