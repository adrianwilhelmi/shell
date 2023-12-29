#ifndef __SHELL_TERMINAL_H__
#define __SHELL_TERMINAL_H__

#include<sys/types.h>
#include<termios.h>
#include<unistd.h>

extern pid_t shell_pgid;
extern struct termios shell_terminal_modes;
extern int shell_terminal;
extern int shell_is_interactive;
extern int loop_status;

void shell_initialization();
void enable_raw_mode();
void disable_raw_mode();

#endif
