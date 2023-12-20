#ifndef __LSH_PARSE_H__
#define __LSH_PARSE_H__

#include<stdio.h>

#include"lsh_terminal.h"
#include"lsh_command.h"

int read_line(char**line);
int split_line_into_words(char*words[], char*line);

#endif
