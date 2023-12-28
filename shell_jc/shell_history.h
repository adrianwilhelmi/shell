#ifndef __SHELL_HISTORY_H__
#define __SHELL_HISTORY_H__

extern char**command_history;
extern int command_history_size;

void initialize_command_history();
void free_command_history();
void add_command_to_history(int index, char*command);
char*get_command_from_history(int index);

#endif
