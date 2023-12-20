//Shell. Author: Adrian Wilhelmi

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#include<fcntl.h>

#include"lsh_terminal.h"
#include"lsh_parse.h"
#include"lsh_command.h"
#include"lsh_job.h"
#include"lsh_builtin.h"

//global variables
pid_t shell_pgid;
struct termios shell_terminal_modes;
int shell_terminal;
Job*first_job;	
Job*current_job;

//used to handle &, initially we wait 
int background = 0;

//built in functions
char*built_in_functions_keys[] = {"cd", "exit", "fg", "bg", "jobs"};
int(*built_in_functions[])(Command*command) = {handle_cd, handle_exit, handle_fg, handle_bg, handle_jobs};

int initialize_background(char*words[], int number_of_words){
	//decides if job should be handled in background
	//it depends on if last word is '&'
	//if so then last word is freed and 1 is returned
	//input:
	//	words[] (char*) - pointer to words;
	//	number_of_words (int) - number of words;
	//output:
	//	background (int) = 1 if output should be handled in background
	//					0 otherwise
	
	if(strcmp(words[number_of_words-1], "&") == 0){
		free(words[number_of_words-1]);
		words[number_of_words-1] = NULL;
		return 1;
	}
	return 0;
}

int handle_built_in_functions(Command*command){
	//handles built in functions
	//input:
	//	commands (Command*) - commands
	//output:
	//	1 if built in function was executed with success
	//	-1 if built in function was executed without success
	//	0 if there was no built in function to execute
	
	int functions_count = (sizeof(built_in_functions_keys) / sizeof(char *));
		
	for(int i = 0; i < functions_count; ++i){
		if(strcmp(command->command[0], built_in_functions_keys[i]) == 0){
			return (*built_in_functions[i])(command);
		}
	}
	return 0;
}

void lsh_loop(){
	//main loop
	
//	char*line;
//	char**words;

	Job*current_job;
	
	Command*commands;
	
	//local 'PATH_MAX'
	static int path_max;

	
	while(1){	
		//read line
		char*line;
		path_max = read_line(&line);
		
		//parse input into words
		char**words = malloc(path_max*sizeof(char*));
		int number_of_words = split_line_into_words(words, line);
		free(line);
		if(number_of_words == 0){
			continue;
		}
		
		//handle &
		background = initialize_background(words, number_of_words);
		if(background){
			--number_of_words;
		}
		//parse words into commands
		commands = malloc(sizeof(Command));
		int number_of_commands = parse_words_into_commands(commands, words, number_of_words);
		
		for(int i = 0; i < number_of_words; ++i){
			free(words[i]);
		}
		free(words);
		
		//handle built in functions
		int builtin_status = handle_built_in_functions(commands);
		if(builtin_status != 0){
			free_commands(commands, number_of_commands);
			if(builtin_status == -1){
				printf("err executing command\n");
			}
			continue;
		}
		
		//setup and run job, handle pipes and redirections
		current_job = malloc(sizeof(Job));
		initialize_job(current_job);
		current_job->commands = commands;
		current_job->number_of_commands = number_of_commands;
		
		//set the job queue
		if(first_job == NULL){
			printf("first job = nul\n");
			first_job = current_job;
		}
		else{
			Job*last_job = first_job;
			while(last_job->next != NULL){
				last_job = last_job->next;
			}
			last_job->next = current_job;
		}
		
		handle_job(current_job, background);

		
		print_job_info();
		printf("wyszlo\n");
		
	}
}

int main(){
	shell_initialization();
	lsh_loop();
	return 0;
}