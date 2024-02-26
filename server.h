#pragma once
#include "cleanup.h"

bool xs_setup_signals(sigset_t*, sigset_t**, fd_t*);
bool xs_setup_socket(int, char* const*, struct sockaddr_un*, fd_t*, char**);

bool xs_poll_sockets(fd_t, fd_t, bool*, bool*);
uint32_t xs_read_signal(fd_t);
bool xs_split(fd_t, bool*);
bool xs_handle_request(fd_t, bool*);

void xs_cleanup_restore(sigset_t**);
void xs_cleanup_unlink(char**);
