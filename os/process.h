#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>

void process_start_user(void);
uint64_t process_syscalls(void);

#endif
