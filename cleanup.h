#pragma once

typedef int fd_t;

void close_p(fd_t*);
void free_p(void*);

#define AUTO_CLOSE __attribute__((cleanup(close_p)))
#define AUTO_FREE __attribute__((cleanup(free_p)))
