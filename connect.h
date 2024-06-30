#pragma once
#include "cleanup.h"

typedef int (*connect_fn)(int, const struct sockaddr*, socklen_t);

extern char* xsocket_address;
extern ssize_t xc_nport;
extern in_port_t* xc_ports;
extern volatile connect_fn xc_connect;

void xconnect_initialize(void);
int xconnect_forward(fd_t, const struct sockaddr*, socklen_t);
void xconnect_terminate(void);
