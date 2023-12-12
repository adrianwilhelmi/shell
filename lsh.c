//Shell. Author: Adrian Wilhelmi

#include<stdio.h>
#include<stdlib.h>
#include<limits.h> //needed for PATH_MAX (for me its 4096), dunno if ill use it
#include<unistd.h> //needed for getcwd, pipe()..
#include<string.h>
#include<signal.h>
#include<sys/wait.h>

struct Commands{
	//list of commands
	//	command - command in a form of array of strings, each array field containing one word
	//	input and output - input and output file descriptors respectively, used only when dealing with pipes
	//	next - pointer to the next command
	char**command;
	int fd[2];		//pipe descriptors
	struct Commands*next;
};

int read_line(char**line);
int split_line_into_words(char*words[], char*line);
int parse_words_into_commands(struct Commands*commands, char**words, const int number_of_words);
void free_commands(struct Commands*commands, int number_of_commands);
void lsh_loop();
void sigint_handler();
void sigchld_handler();


void sigint_handler(){
	//handling SIGINT
	printf("\nsigint\n");
	exit(0);
}

void sigchld_handler(){
	waitpid(-1, NULL, WNOHANG);
}

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

int parse_words_into_commands(struct Commands *commands, char*words[], const int number_of_words){
	//parses separateed words from the input into commands
	//each command is an array of strings
	//commands are separated by '|'
	//commands are represented by a linked list
	//input:
	//	commands (struct Commands*) - pointer to the beggining of commands linked list
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
			commands->next = malloc(sizeof(struct Commands));
			commands = commands->next;

			++number_of_commands;
			pipe_index = i+1;
		}
	}
	
	//allocated one more before jumping out of loop, need to free it
	free(commands);
	
	return number_of_commands;
}

void free_commands(struct Commands*commands, const int number_of_commands){
	//cleans memory after commands are executed
	//input:
	//	commands (struct Commands*) - commands to be freed
	//	number_of_commands (int) - number of commands
	
	struct Commands* prev = commands;
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

void handle_commands(struct Commands*commands, int number_of_commands, int should_wait){
	//handles commands from the input line
	//input:
	//	commands (struct Commands*) - commands to handle
	//	number_of_commands (int) - number of commands

	//
	int fd_terminal_input = dup(0);
	int fd_terminal_output = dup(1);
	
	int fd_temp_in = dup(fd_terminal_input);
	int fd_temp_out;
	pid_t pid;
	
	for(int i = 0; i < number_of_commands; ++i){
		dup2(fd_temp_in, 0);
		close(fd_temp_in);
		if(i == number_of_commands - 1){
			fd_temp_out = dup(fd_terminal_output);
		}
		else{
			pipe(commands->fd);
			fd_temp_out = commands->fd[1];
			fd_temp_in = commands->fd[0];
		}
		dup2(fd_temp_out, 1);
		close(fd_temp_out);
		
		pid = fork();
		if(pid == 0){
			if(i != number_of_commands - 1){
				close(commands->fd[0]);
				close(commands->fd[1]);
			}
			execvp(commands->command[0], commands->command);
			perror("unknown command");
			exit(EXIT_FAILURE);
		}
		wait(NULL);
		
		if(i != number_of_commands -1){
			commands = commands->next;
		}
	}
	
	dup2(fd_terminal_input, 0);
	dup2(fd_terminal_output, 1);
	close(fd_terminal_input);
	close(fd_terminal_output);
}



void lsh_loop(){
	//main loop
	//input:
	//	loop_status - flag to iterate to the next loop
	
	char*line;
	char**words;
	struct Commands*commands;
	
	static int loop_status = 1;
	//local 'PATH_MAX'
	static int path_max;
	//used to handle &, initially we wait 
	static int should_wait = 1;
	
	signal(SIGINT, sigint_handler);
	
	while(loop_status){
		//read line
		path_max = read_line(&line);
		
		//parse input into words
		words = malloc(path_max*sizeof(char*));
		int number_of_words = split_line_into_words(words, line);
		free(line);
		if(number_of_words == 0){
			continue;
		}
		
		//parse words into commands
		commands = malloc(sizeof(struct Commands));
		int number_of_commands = parse_words_into_commands(commands, words, number_of_words);	
		
		//handle commands
		signal(SIGCHLD, sigchld_handler);
		int child = fork();
		if(child == 0){
			handle_commands(commands, number_of_commands, should_wait);
		}
		else{
			if(should_wait){
				int status = 0;
				int wpid = waitpid(child, &status, 0);
			}
		}
		
		
		//free memory before allocating it again
		for(int i = 0; i < number_of_words; ++i){
			free(words[i]);
		}
		free(words);
		
		free_commands(commands, number_of_commands);
	}
	
}

int main(){
	lsh_loop();
	return 0;
}
