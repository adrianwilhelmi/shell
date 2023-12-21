#ifndef __SHELL_HISTORY_H__
#define __SHELL_HISTORY_H__

#include"shell_command.h"

typedef struct CHistory{
	Command*command;
	struct CHistory*next;
	struct CHistory*prev;
} CHistory;

extern CHistory*command_history;
extern Command*command_history_curr;

void handle_arrows(char c);

#endif
