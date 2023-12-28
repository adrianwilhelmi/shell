#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

int shell_terminal = STDIN_FILENO;
int cursor_position;
int current_line_length;

char*command_history[10];

void insert_char_into_word(char*word, char c){
	for(int i = current_line_length; i >= cursor_position; --i){
		word[i+1] = word[i];
	}
	
	word[cursor_position] = c;
}

void handle_backspace(char*word){

}

void move_cursor_left(){
	if(cursor_position > 0){
		printf("\x1b[%dD", 1);
		fflush(stdout);
		--cursor_position;
	}
}

void move_cursor_right(){
	if(cursor_position < current_line_length){
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

int main() {
	shell_terminal = STDIN_FILENO;
	struct termios original, raw;

	tcgetattr(shell_terminal, &original);
	raw = original;

	cfmakeraw(&raw);
	tcsetattr(shell_terminal, TCSANOW, &raw);

	cursor_position = 0;
	current_line_length = 0;

	int buff_size = 1;
	char*current_line = malloc(sizeof(char*)*buff_size);
	


	char prev_c = 0;
	char c;
	while(1){
		read(shell_terminal, &c, 1);
		
		if(c == 'q'){
			break;
		}
		
		if(c == '\n' || c == '\r' || (prev_c == '\r' && c == '\n') || c == 4){
			write(shell_terminal, "\n", 1);
			current_line[current_line_length] = '\0';
			if(command_history[0] == NULL){
				command_history[0] = strdup(current_line);
			}
			else{
				free(command_history[0]);
				command_history[0] = strdup(current_line);
			}
			cursor_position = 0;
			current_line_length = 0;
			
			free(current_line);
			buff_size = 1;
			current_line = malloc(buff_size*sizeof(char));
		}
		
		if(c == '\b' || c == 127){
			if(cursor_position > 0){
				//handle_backspace(current_line);
				for(int i = cursor_position - 1; i < current_line_length; ++i){
					current_line[i] = current_line[i+1];
				}
				clear_line();
				--current_line_length;
				--cursor_position;
				int i = current_line_length - cursor_position;
				cursor_position = current_line_length;
				write(shell_terminal, current_line, current_line_length);
				while(i){
					move_cursor_left();
					--i;
				}
			}
		}
		else if(c == '\x1b'){
			char arrow_seq[3];
			if(read(shell_terminal, &arrow_seq[0], 1) == 0){
				break;
			}
			if(read(shell_terminal, &arrow_seq[1], 1) == 0){
				break;
			}
			
			if(arrow_seq[0] == '['){
				if(arrow_seq[1] == 'A'){
					clear_line();
					current_line_length = 0;
					cursor_position = 0;
					free(current_line);
//					write(shell_terminal, "uparro", 6);
					write(shell_terminal, command_history[0], strlen(command_history[0]));
					current_line = strdup(command_history[0]);
					current_line_length += strlen(command_history[0]);
					cursor_position += strlen(command_history[0]);
				}
				if(arrow_seq[1] == 'B'){
					clear_line();
					current_line_length = 0;
					cursor_position = 0;
					free(current_line);
					write(shell_terminal, "dowarro", 7);
					
					current_line_length += 7;
					cursor_position += 7;
					
				}
				if(arrow_seq[1] == 'D'){
					move_cursor_left();
				}
				if(arrow_seq[1] == 'C'){
					move_cursor_right();
				}
			}
		}
		else{
			write(shell_terminal, &c, 1);
			if(current_line_length + 1 >= buff_size){
				buff_size *= 2;
				current_line = realloc(current_line, buff_size*sizeof(char));
			}
			insert_char_into_word(current_line, c);
			clear_line();
			++current_line_length;
			++cursor_position;
			int i = current_line_length - cursor_position;
			cursor_position = current_line_length;
			write(shell_terminal, current_line, current_line_length);
			while(i){
				move_cursor_left();
				--i;
			}
			
		}
		
		if(current_line_length >= buff_size){
			buff_size *= 2;
			current_line = realloc(current_line, buff_size*sizeof(char));
			current_line[current_line_length-1] = c;
		}
		prev_c = c;
	}
	
	free(current_line);
	for(int i = 0; i < 10; ++i){
		free(command_history[i]);
	}
	
	tcsetattr(shell_terminal, TCSANOW, &original);

	return 0;
}
