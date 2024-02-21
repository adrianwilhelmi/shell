#ifndef __SHELL_BUILTIN_H__
#define __SHELL_BUILTIN_H__

#include"shell_command.h"

extern const char*built_in_functions_keys[];
extern const int(*built_in_functions[])(Command*command);

//built in functions
int handle_cd(Command*command);
int handle_exit(Command*command);
int handle_bg(Command*command);
int handle_fg(Command*command);
int handle_jobs(Command*command);

#endif
