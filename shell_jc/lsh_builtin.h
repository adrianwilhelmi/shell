#ifndef __LSH_BUILTIN_H__
#define __LSH_BUILTIN_H__

#include"lsh_command.h"

extern char*built_in_functions_keys[];
extern int(*built_in_functions[])(Command*command);

//built in functions
int handle_cd(Command*command);
int handle_exit(Command*command);
int handle_bg(Command*command);
int handle_fg(Command*command);
int handle_jobs(Command*command);

#endif
