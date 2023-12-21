#ifndef __SHELL_COMMAND_H__
#define __SHELL_COMMAND_H__

#include<stdlib.h>

typedef struct Command{
	//linked list of commands. each command is a different process in a pipeline
	//	command - 
	//	input and output - input and output file descriptors respectively, used only when dealing with pipes
	//	next - 
	char**command;			//command in a form of array of strings, each array field containing one word
	pid_t pid;			//command's id
	int status;			//status reported from waitpid
	int completed;			//'1' if process has completed, '0' othersie
	int stopped;			//'1' if process has stopped, '0' otherwise
	struct Command*next;	//pointer to the next command
} Command;

void free_commands(Command*commands, int number_of_commands);
int parse_words_into_commands(Command*commands, char*words[], const int number_of_words);
void handle_command(Command*commands, int input_file, int output_file, 
				int output_err_file, int group_pid, int background);

#endif
