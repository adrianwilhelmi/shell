#ifndef __SHELL_PARSE_H__
#define __SHELL_PARSE_H__

#include<stdio.h>

#include"shell_terminal.h"
#include"shell_command.h"

extern int cursor_position;
extern int current_line_length;
extern char*cwd_buffer;
extern int cwd_size;
extern int buffer_size;

void move_cursor_left();
void move_cursor_right();
void clear_line();
void insert_char_into_word(char*word, char c);
int read_line(char**line);
int split_line_into_words(char*words[], char*line);

#endif
