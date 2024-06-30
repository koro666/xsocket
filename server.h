#pragma once
#include "cleanup.h"

#ifdef XSOCKET_SYSTEMD
extern bool xs_journal;

bool xs_check_journal(void);

void xs_perror(const char*);
void xs_printf(int, const char*, ...);
#else
#define xs_perror perror
#define xs_printf(priority, format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#endif

bool xs_setup_signals(sigset_t*, sigset_t**, fd_t*);
bool xs_setup_socket(int, char* const*, struct sockaddr_un*, fd_t*, char**);

bool xs_poll_sockets(fd_t, fd_t, bool*, bool*);
uint32_t xs_read_signal(fd_t);
bool xs_split(fd_t, bool*);
bool xs_handle_request(fd_t, bool*);

void xs_cleanup_restore(sigset_t**);
void xs_cleanup_unlink(char**);
