//Shell. Author: Adrian Wilhelmi

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
#include<fcntl.h>

struct Commands{
	//list of commands
	//	command - command in a form of array of strings, each array field containing one word
	//	input and output - input and output file descriptors respectively, used only when dealing with pipes
	//	next - pointer to the next command
	char**command;
	int fd[2];		//pipe descriptors
	struct Commands*next;
};


//definitions
int read_line(char**line);
int split_line_into_words(char*words[], char*line);
int parse_words_into_commands(struct Commands*commands, char**words, const int number_of_words);
int handle_process_in_background(struct Commands*commands, int number_of_commands);
void redirect_command(struct Commands*commands, int number_of_commands, char*paths[3]);
void free_commands(struct Commands*commands, int number_of_commands);
void handle_commands(struct Commands*commands, int number_of_commands);
void lsh_loop();
void sigint_handler();
void sigchld_handler();

//built in functions
int handle_cd(struct Commands*commands);
int handle_exit(struct Commands*commands);

char*built_in_functions_keys[] = {"cd", "exit"};
int(*built_in_functions[])(struct Commands*) = {&handle_cd, &handle_exit};



void sigint_handler(){
	//ignore
}

void sigchld_handler(){
	while(waitpid(-1, NULL, WNOHANG) > 0);
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

int handle_process_in_background(struct Commands*commands, int number_of_commands){
	//decides if process should be handled in background
	//it depends on if last word of last command is &
	//if so then last word of last command is freed and flag should_wait is returned\
	//input:
	//	commands (struct Commands*) - pointer to commands;
	//	number_of_commands (int) - number_of_commands;
	//output:
	//	should_wait (int) = 1 if output should be handled in background
	//					0 otherwise
	
	//go to last command
	for(int i = 0; i < number_of_commands-1; ++i){
		commands = commands->next;
	}
	
	//find length of last command
	int last_command_len = 0;
	while(commands->command[last_command_len] != NULL){
		++last_command_len;
	}
	
	//if last word of last command == &: free it and return 0
	if(strcmp(commands->command[last_command_len-1], "&") == 0){
		free(commands->command[last_command_len-1]);
		commands->command[last_command_len-1] = NULL;
		free(commands->command[last_command_len]);
		return 0;
	}
	return 1;
}

void redirect_command(struct Commands*commands, int number_of_commands, char*paths[3]){
	//analyses if theres < in the first command or if theres > or 2> in the last command
	//if so: redirects input, output or error output to path located in the word after the redirection command
	//input:
	//	commands (struct Commands*) - commands
	//	number_of_commands (int) - number of commands
	//	paths[3] (char*) - array containing paths for redirection
	//		paths[0] - path to input
	//		paths[1] - path to output
	//		paths[3] - path to error output

	int index = 0;
	
	//only first command can redirect input
	while(commands->command[index+1] != NULL){
		if(strcmp(commands->command[index], "<") == 0){
			//copy file after > to paths[0]
			paths[0] = strdup(commands->command[index+1]);
			
			//remove ["<", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		++index;
	}
	
	//go to last command
	for(int i = 0; i < number_of_commands - 1; ++i){
		commands = commands->next;
	}
	
	index = 0;
	
	//only last command can redirect output and err output (first command can be last if it's the only one)
	while(commands->command[index+1] != NULL){
		if(strcmp(commands->command[index], ">") == 0){
			//copy file after > to paths[1]
			paths[1] = strdup(commands->command[index+1]);
			
			//remove [">", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		else if(strcmp(commands->command[index], "2>") == 0){
			//copy file after > to paths[2]
			paths[2] = strdup(commands->command[index+1]);
			
			//remove ["2>", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		++index;
	}
}

void free_commands(struct Commands*commands, const int number_of_commands){
	//cleans memory after commands are executed
	//input:
	//	commands (struct Commands*) - commands to be freed
	//	number_of_commands (int) - number of command
	
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

int handle_cd(struct Commands*commands){
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
		if(chdir(commands->command[1]) > 0){
			perror("cd problem");
		}
//		free_commands(commands, 1);
	}
	return 1;
}

int handle_exit(struct Commands*commands){
	//handle exit built in function
	
	free_commands(commands, 1);
	printf("exit\n");
	exit(EXIT_SUCCESS);
	return 1;
}

int handle_built_in_functions(struct Commands*commands){
	//handles built in functions
	//input:
	//	commands (struct Commands*) - commands
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

void handle_commands(struct Commands*commands, int number_of_commands){
	//handles commands from the input line
	//input:
	//	commands (struct Commands*) - commands to handle
	//	number_of_commands (int) - number of commands
	//	should_wait (int) - flag if process should be running if background
	//					if true output is set to 
	
	int fd_terminal_input = dup(0);			//terminal input
	int fd_terminal_output = dup(1);			//terminal input
	
	int fd_redirected_output = dup(1);			//if not redirected its same as terminal outut, input wont be needed
	
	int fd_temp_in = dup(fd_terminal_input);	//initially input is from terminal
	int fd_temp_out;
	
	//handle redirecting
	char*paths[3];
	paths[0] = NULL;
	paths[1] = NULL;
	paths[2] = NULL;
	
	redirect_command(commands, number_of_commands, paths);
	
	//create fds for new files if redirecting
	//< redirection
	if(paths[0] != NULL){
		int new_input_fd = open(paths[0], O_RDONLY | O_CREAT);
		dup2(new_input_fd, fd_temp_in);	
		close(new_input_fd);
	}
	//> redirection
	if(paths[1] != NULL){
		//now only open output file, it will be truncated later
		int new_output_fd = open(paths[1], O_RDWR | O_CREAT);
		dup2(new_output_fd, fd_redirected_output);
		close(new_output_fd);
	}
	//2> redirection
	if(paths[2] != NULL){
		int new_output_err_fd = open(paths[2], O_RDWR | O_CREAT | O_TRUNC);
		dup2(new_output_err_fd, 2);
		close(new_output_err_fd);
	}
	
	//handle pipes
	//first command's input is either set to terminal or proper file if redirected
	pid_t pid;
	for(int i = 0; i < number_of_commands; ++i){
		dup2(fd_temp_in, 0);
		close(fd_temp_in);
		
		//last command? yes -> output goes to terminal or to the proper file if redirected
		if(i == number_of_commands - 1){
			//if output was redirected to the same file as input then clear the file
			if(paths[0] != NULL && paths[0] == paths[1]){
				int truncate_redirect = open(paths[1], O_TRUNC);
			}
			fd_temp_out = dup(fd_redirected_output);
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
			perror("exec err");
			exit(EXIT_FAILURE);
		}
		wait(NULL);

		if(i != number_of_commands -1){
			commands = commands->next;
		}
	}
	
	free(paths[0]);
	free(paths[1]);
	free(paths[2]);
	
	dup2(fd_terminal_input, 0);
	
	close(fd_terminal_input);
	close(fd_terminal_output);
	
	close(fd_redirected_output);
	exit(0);
}

void lsh_loop(){
	//main loop
	
	char*line;
	char**words;
	struct Commands*commands;
	
	//local 'PATH_MAX'
	static int path_max;
	//used to handle &, initially we wait 
	static int should_wait = 1;
	
	signal(SIGINT, sigint_handler);
	signal(SIGCHLD, sigchld_handler);
	
	while(1){
		signal(SIGINT, sigint_handler);
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
		}
		
		//handle &
		should_wait = handle_process_in_background(commands, number_of_commands);
		
		//handle pipes
		if(!should_wait){
		//if process should run in background redirect output to null and ignore signals from child
			signal(SIGCHLD, SIG_IGN);
			pid_t child = fork();
			if(child == 0){
				int fd_null = open("/dev/null", O_WRONLY);
				dup2(fd_null, 1);
				close(fd_null);
				handle_commands(commands, number_of_commands);
			}
			else{
				printf("PID: %d\n", child+1);
			}
		}
		else{
		//otherwise wait for child process to end
			signal(SIGCHLD, sigchld_handler);
			pid_t child = fork();
			if(child == 0){
				handle_commands(commands, number_of_commands);
			}
			else{
				int status = 0;
				int wpid = waitpid(child, &status, 0);
			}
		}
		
		//free memory before allocating it again
		free_commands(commands, number_of_commands);
	}
	
}

int main(){
	lsh_loop();
	return 0;
}
