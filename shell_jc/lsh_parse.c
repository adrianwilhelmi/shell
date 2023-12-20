#include"lsh_parse.h"

#include<string.h>

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
