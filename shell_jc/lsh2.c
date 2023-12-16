//Shell. Author: Adrian Wilhelmi

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#include<fcntl.h>

#include"lsh_command.h"
#include"lsh_job.h"
#include"lsh_terminal.h"


//definitions
int read_line(char**line);
int split_line_into_words(char*words[], char*line);
int initialize_background(char*words[], int number_of_words);
void lsh_loop();

//built in functions
int handle_cd(Command*commands);
int handle_exit(Command*commands);
int handle_bg(Command*commands);
int handle_fg(Command*commands);
int handle_jobs(Command*commands);

char*built_in_functions_keys[] = {"cd", "exit", "fg", "bg", "jobs"};
int(*built_in_functions[])(Command*) = {handle_cd, handle_exit, handle_fg, handle_bg, handle_jobs};

int read_line(char**line){
	//prints current directory and parses input from terminal to *line
	//input:
	//	*line (*char) - pointer to string where input will be stored
	//output:
	//	buffer_size (int) - smallest 2^n that is > sizeof(*line), amount of memory allocated for *line
	
	//print current directory
	int buffer_size = 1;
	char*cwd = malloc(buffer_size*sizeof(char));
	while(getcwd(cwd, buffer_size) == NULL){
		buffer_size *= 2;
		cwd = realloc(cwd, buffer_size*sizeof(char));
		if(cwd == NULL){
			perror("cwd realloc err");
			exit(EXIT_FAILURE);
		}
	}
	printf("lsh: %s> ", cwd);
	free(cwd);
	
	//read line
	int position = 0;
	buffer_size = 1;
	int c;
	*line = malloc(buffer_size*sizeof(char));
			
	while((c = getchar()) != EOF && c != '\n'){
		(*line)[position++] = c;
		if(position >= buffer_size){
			buffer_size *= 2;
			*line = realloc(*line, buffer_size*sizeof(char));
			if(!*line){
				perror("line realloc err");
				exit(EXIT_FAILURE);
			}
		}
	}
	(*line)[position] = '\0';
	
	if(c == EOF){
		printf("\nexit.\n");
		free(*line);
		exit(EXIT_SUCCESS);
	}

	return buffer_size;
}

int split_line_into_words(char*words[], char*line){
	//parses line from input into seperate words and puts it into words[]
	//input:
	//	words[] (*char) - array of strings where separated words will be located
	//	line (*char) - input from console in form of a string
	//output:
	//	number_of_words (int) - number of words in the line from the input

	char delimiters[2] = " "; //empty space and \0
	
	int number_of_words = 0;
	char*word = strtok(line, delimiters);
	while(word != NULL){
		words[number_of_words] = calloc(strlen(word) + 1, sizeof(char));
		strcpy(words[number_of_words], word);
		++number_of_words;
		word = strtok(NULL, delimiters);
	}
	
	return number_of_words;
}

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

int handle_cd(Command*commands){
	//handle cd built in function
	
	if(commands->command[1] == NULL){
		printf("enter path\n");
		return -1;
	}
	else{
		if(commands->command[2] != NULL){
			printf("too many argumentz\n");
			return -1;
		}
		if(chdir(commands->command[1]) != 0){
			perror("cd");
		}
	}
	return 1;
}

int handle_exit(Command*commands){
	//handle exit built in function
	
	free_commands(commands, 1);
	printf("exit\n");
	exit(EXIT_SUCCESS);
	return 1;
}

int handle_fg(Command*commands){
	return 1;
}

int handle_bg(Command*commands){
	return 1;
}
int handle_jobs(Command*commands){
	return 1;
}

int handle_built_in_functions(Command*commands){
	//handles built in functions
	//input:
	//	commands (Command*) - commands
	//output:
	//	1 if built in function was executed with success
	//	-1 if built in function was executed without success
	//	0 if there was no built in function to execute
	
	int functions_count = (sizeof(built_in_functions_keys) / sizeof(char *));

	for(int i = 0; i < functions_count; ++i){
		if(strcmp(commands->command[0], built_in_functions_keys[i]) == 0){
			return (*built_in_functions[i])(commands);
		}
	}
	return 0;
}


void lsh_loop(){
	//main loop
	
	char*line;
	char**words;
	Job*current_job = malloc(sizeof(Job));
	initialize_job(current_job);
	Job*first_job = current_job;
	Command*commands;
	
	//local 'PATH_MAX'
	static int path_max;
	//used to handle &, initially we wait 
	static int background = 1;
	
	
	while(1){
		//read line
		path_max = read_line(&line);
		
		//parse input into words
		words = malloc(path_max*sizeof(char*));
		int number_of_words = split_line_into_words(words, line);
		free(line);
		if(number_of_words == 0){
			continue;
		}
		
		//handle &
		int background = initialize_background(words, number_of_words);
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
		
		//handle commands
		//handle built in functions
		if(handle_built_in_functions(commands)){
			free_commands(commands, number_of_commands);
			continue;
		}
		else if(handle_built_in_functions(commands) == -1){
			free_commands(commands, number_of_commands);
			printf("err executing command\n");
			continue;
		}
		
		current_job->commands = commands;
		current_job->number_of_commands = number_of_commands;
		//handle pipes	and redirections
		handle_job(current_job, background);
		
		//free memory before allocating it again
		free_commands(commands, number_of_commands);
		
		current_job->next = malloc(sizeof(Job));
		initialize_job(current_job->next);
		current_job = current_job->next;
	}
}

int main(){
	shell_initialization();
	lsh_loop();
	return 0;
}
