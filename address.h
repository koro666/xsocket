#pragma once
#include "cleanup.h"

ssize_t parse_port_list(const char*, in_port_t**);

int check_socket(fd_t, int*, int*, int*);
bool check_address(const struct sockaddr*, socklen_t, const in_port_t*, ssize_t);
