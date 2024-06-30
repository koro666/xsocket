#pragma once
#include "cleanup.h"

typedef int (*bind_fn)(int, const struct sockaddr*, socklen_t);

extern char* xsocket_address;
extern ssize_t xb_nport;
extern in_port_t* xb_ports;
extern volatile bind_fn xb_bind;

void xbind_initialize(void);
int xbind_forward(fd_t, const struct sockaddr*, socklen_t);
void xbind_terminate(void);
