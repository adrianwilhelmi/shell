#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#include"shell_parse.h"
#include"shell_terminal.h"
#include"shell_history.h"

int current_line_length;
int cursor_position;
char*cwd_buffer;
int cwd_size;
int buffer_size;
int command_history_index;

void insert_char_into_word(char*word, char c){
	for(int i = current_line_length; i >= cursor_position - cwd_size; --i){
		word[i+1] = word[i];
	}
	
	word[cursor_position - cwd_size] = c;
}

void move_cursor_left(){
	if(cursor_position > cwd_size){
		printf("\x1b[%dD", 1);
		fflush(stdout);
		--cursor_position;
	}
}

void move_cursor_right(){
	if(cursor_position < current_line_length + cwd_size){
		printf("\x1b[%dC", 1);
		fflush(stdout);
		++cursor_position;
	}
}

void clear_line(){
	printf("\r");
	fflush(stdout);
	printf("\x1b[2K");
	fflush(stdout);
}

int read_line(char**line){
	//prints current directory and parses input from terminal
	//input:
	//	*line (*char) - pointer to string where input will be stored
	//output:
	//	buffer_size (int) - smallest 2^n that is > sizeof(*line), amount of memory allocated for *line
	
	
	//enable raw mode to handle everything how we like
	enable_raw_mode();
	
	//print current directory
	buffer_size = 1;
	char*cwd = malloc(buffer_size*sizeof(char));
	while(getcwd(cwd, buffer_size) == NULL){
		buffer_size *= 2;
		cwd = realloc(cwd, buffer_size*sizeof(char));
		if(cwd == NULL){
			perror("cwd realloc err");
			exit(EXIT_FAILURE);
		}
	}
	
	cwd_buffer = malloc(strlen(cwd) + 10);
	snprintf(cwd_buffer, strlen(cwd) + 10, "shell: %s> ", cwd);
	write(shell_terminal, cwd_buffer, strlen(cwd_buffer));

	cwd_size = strlen(cwd_buffer);
	
	free(cwd);
	
	//read line
	buffer_size = 1;
	*line = calloc(buffer_size, sizeof(char));
	char*line_with_cwd = strdup(cwd_buffer);
	
	current_line_length = 0;
	cursor_position = cwd_size;
	
	command_history_index = command_history_size;
	
	add_command_to_history(command_history_size, "\0");
	
	char**history_backup = malloc(command_history_size*sizeof(char*));
	for(int i = 0; i < command_history_size; ++i){
		history_backup[i] = strdup(command_history[i]);
	}
	
	char c;
	char prev_c = 0;
	while(1){
		read(shell_terminal, &c, 1);
		
		//quit if enter or ctrl+d
		if(c == '\n' || c == '\r' || (prev_c == '\r' && c == '\n') || c == 4){ //ctrl+d ascii code is 4
			(*line)[current_line_length] = '\0';
			write(shell_terminal, "\n", 1);
			
			cwd_size = 0;
			while(cursor_position){
				move_cursor_left();
			}
			
			for(int i = 0; i < command_history_size - 1; ++i){
				add_command_to_history(i, history_backup[i]);
				free(history_backup[i]);
			}
			
			free(history_backup[command_history_size - 1]);
			free(history_backup);
			
			if(current_line_length > 1 && strcmp("\0", *line) != 0){
				add_command_to_history(command_history_size - 1, *line);
			}
			else{
				free(command_history[command_history_size - 1]);
				--command_history_size;
			}
			
			current_line_length = 0;
			break;
		}
		
		//backspace
		if(c == '\b' || c == 127){
			if(cursor_position > cwd_size){
				for(int i = cursor_position - cwd_size - 1; i < current_line_length; ++i){
					(*line)[i] = (*line)[i+1];
				}
				add_command_to_history(command_history_index, *line);
				clear_line();
				--current_line_length;
				--cursor_position;
				int i = cwd_size + current_line_length - cursor_position;
				cursor_position =  cwd_size + current_line_length;
				
				free(line_with_cwd);
				line_with_cwd = strdup(cwd_buffer);
				line_with_cwd = realloc(line_with_cwd, (cwd_size + current_line_length + 1)*sizeof(char));
				strcat(line_with_cwd, *line);
				
				write(shell_terminal, line_with_cwd, cwd_size + current_line_length);
				
				while(i){
					move_cursor_left();
					--i;
				}
			}
		}
		//handle arrows
		else if(c == '\x1b'){
			char arrow_seq[3];
			if(read(shell_terminal, &arrow_seq[0], 1) == 0){
				printf("read err\n");
				break;
			}
			if(read(shell_terminal, &arrow_seq[1], 1) == 0){
				printf("read err\n");
				break;
			}
			
			if(arrow_seq[0] == '['){
				if(arrow_seq[1] == 'A'){
					if(command_history_index > 0){
						free(*line);
						
						--command_history_index;
						*line = strdup(get_command_from_history(command_history_index));
						
						current_line_length = strlen(*line);
						buffer_size = current_line_length;
						
						clear_line();
						
						free(line_with_cwd);
						line_with_cwd = strdup(cwd_buffer);
						line_with_cwd = realloc(line_with_cwd, (cwd_size + current_line_length + 1)*sizeof(char));
						strcat(line_with_cwd, *line);
						
						write(shell_terminal, line_with_cwd, cwd_size + current_line_length);
						
						cursor_position = cwd_size + current_line_length;
					}
				}
				else if(arrow_seq[1] == 'B'){
					if(command_history_index + 1 < command_history_size){
						free(*line);
						
						++command_history_index;
						*line = strdup(get_command_from_history(command_history_index));
				
						current_line_length = strlen(*line);
						buffer_size = current_line_length + 1;
						
						clear_line();
						
						free(line_with_cwd);
						line_with_cwd = strdup(cwd_buffer);
						line_with_cwd = realloc(line_with_cwd, (cwd_size + current_line_length + 1)*sizeof(char));
						strcat(line_with_cwd, *line);
						
						write(shell_terminal, line_with_cwd, cwd_size + current_line_length);
						
						cursor_position = cwd_size + current_line_length;
					}
				}
				else if(arrow_seq[1] == 'D'){
					move_cursor_left();
				}
				else if(arrow_seq[1] == 'C'){
					move_cursor_right();
				}
			}
		}
		else{
			//put char into where cursor is
			while(current_line_length + 1 >= buffer_size){
				buffer_size *= 2;
				*line = realloc(*line, buffer_size*sizeof(char));
			}
			
			insert_char_into_word(*line, c);
			++current_line_length;
			++cursor_position;

			(*line)[current_line_length] = '\0';

			int i = cwd_size + current_line_length - cursor_position;
			cursor_position = current_line_length + cwd_size;
			
			clear_line();
			
			free(line_with_cwd);
			line_with_cwd = strdup(cwd_buffer);
			line_with_cwd = realloc(line_with_cwd, (cwd_size + current_line_length + 1)*sizeof(char));
			strcat(line_with_cwd, *line);
			
			write(shell_terminal, line_with_cwd, cwd_size + current_line_length);
			
			while(i){
				move_cursor_left();
				--i;
			}
			
			add_command_to_history(command_history_index, *line);
		}
		
		prev_c = c;
	}
	
	free(cwd_buffer);
	free(line_with_cwd);
	
	disable_raw_mode();
	
	if(c == 4){
		write(shell_terminal, "exit", 4);
		write(shell_terminal, "\n", 1);
		loop_status = 0;
		return 0;
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
