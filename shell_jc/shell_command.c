#include<string.h>
#include<signal.h>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>

#include"shell_command.h"
#include"shell_terminal.h"
#include"shell_job.h"

void free_commands(Command*commands, const int number_of_commands){
	//cleans memory when job is terminated
	//input:
	//	commands (Command*) - commands to be freed
	//	number_of_commands (int) - number of command
	
	Command* prev = commands;
	for(int i = 0; i < number_of_commands; ++i){
		for(int j = 0; commands->command[j] != NULL; ++j){
			free(commands->command[j]);
		}
		free(commands->command);
		if(commands->next == NULL){
			free(commands);
			break;
		}
		prev = commands;
		commands = commands->next;
		free(prev);
	}
}

int parse_words_into_commands(Command *commands, char*words[], const int number_of_words){
	//parses separateed words from the input into commands
	//each command is an array of strings
	//commands are separated by '|'
	//commands are represented by a linked list
	//input:
	//	commands (Command*) - pointer to the beggining of commands linked list
	//							commands will be put there
	//	words[] - (char*) - array of words from the input
	//	number_of_words (int) - number of words
	//output:
	//	number_of_commands (int) - number of commands
	
	int number_of_commands = 0;
	
	//index of latest '|' found 
	int pipe_index = 0;
	
	for(int i = 0; i <= number_of_words; ++i){
		if(i == number_of_words || words[i][0] == '|'){
			int k = 0;
			int j = pipe_index;
			//initialize new command
			commands->command = malloc((i-pipe_index+1)*sizeof(char*));
			commands->next = NULL;
	
			while(j < i){
				//copy words to command until the moment when word[j] = '|'
				commands->command[k] = calloc(strlen(words[j]) + 1, sizeof(char));
				strcpy(commands->command[k], words[j]);
				++k;
				++j;
			
			}
			//last word in command is NULL
			commands->command[k] = NULL;
			
			//allocate memory for new command
			commands->next = malloc(sizeof(Command));
			commands = commands->next;

			++number_of_commands;
			pipe_index = i+1;
		}
	}
	
	//allocated one more before jumping out of loop, need to free it
	free(commands);
	
	return number_of_commands;
}

void handle_command(Command*command, int input_file, int output_file, int output_err_file, int group_pid, int background){
	pid_t pid;
	pid = getpid();
	if(group_pid == 0){
		group_pid = pid;
	}
	setpgid(pid, group_pid);
	if(!background){
		tcsetpgrp(shell_terminal, group_pid);
	}
	
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);
	
	if(input_file != STDIN_FILENO){
		dup2(input_file, STDIN_FILENO);
		close(input_file);
	}
	if(output_file != STDOUT_FILENO){
		dup2(output_file, STDOUT_FILENO);
		close(output_file);
	}
	if(output_err_file != STDERR_FILENO){
		dup2(output_err_file, STDERR_FILENO);
		close(output_err_file);
	}
	
//	printf("current_command->pid: %d\n", command->pid);
	execvp(command->command[0], command->command);
	perror("command err");
	exit(EXIT_FAILURE);
}
