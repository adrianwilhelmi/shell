#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>

#include"shell_sgrep.h"

int sgrep(Command*command){
	//very simplified version of grep
	//unlike grep in classic bash, this grep is built in command
	//usage: grep <pattern> <file>
	
	char*pattern = command->command[1];
	char*filename;
	char line[PATH_MAX];
	FILE*file;
	
	if(command->command[2] != NULL){
		filename = command->command[2];
	}
	
	file = fopen(filename, "r");
	if(file == NULL){
		perror("opening file err");
		exit(EXIT_FAILURE);
	}
	
	while(fgets(line, sizeof(line), file)){
		if(strstr(line, pattern)){
			printf("%s", line);
		}
	}
	
	fclose(file);
	
	return 0;
}
