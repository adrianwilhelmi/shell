#ifndef __SHELL_PARSE_H__
#define __SHELL_PARSE_H__

#include<stdio.h>

#include"shell_terminal.h"
#include"shell_command.h"

int read_line(char**line);
int split_line_into_words(char*words[], char*line);

#endif
