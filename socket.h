#pragma once
#include "cleanup.h"

int check_socket(fd_t, int*, int*, int*);
bool check_address(const struct sockaddr*, socklen_t, const in_port_t*, ssize_t);

void to_sockaddr_un(const char*, struct sockaddr_un*);
size_t sockaddr_un_len(const struct sockaddr_un*);

ssize_t send_packet(fd_t, const void*, size_t, fd_t);
ssize_t recv_packet(fd_t, void*, size_t, struct ucred*, fd_t*, bool);
