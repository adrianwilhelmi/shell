#ifndef __LSH_TERMINAL_H__
#define __LSH_TERMINAL_H__

#include<sys/types.h>
#include<termios.h>
#include<unistd.h>

extern pid_t shell_pgid;
extern struct termios shell_terminal_modes;
extern int shell_terminal;

void shell_initialization();

#endif
