#include<stdlib.h>
#include<string.h>

#include"shell_history.h"

char**command_history;
int command_history_size;

void initialize_command_history(){
	command_history = malloc(sizeof(char*));
	command_history_size = 0;
}

void free_command_history(){
	for(int i = 0; i < command_history_size; ++i){
		free(command_history[i]);
	}
	free(command_history);
}

void add_command_to_history(int index, char*command){
	if(index == command_history_size){
		command_history = realloc(command_history, (command_history_size + 1)*sizeof(char*));
		command_history[command_history_size] = strdup(command);
		++command_history_size;
	}
	else{
		free(command_history[index]);
		command_history[index] = strdup(command);
	}
}

char*get_command_from_history(int index){
	if(index < 0 || index > command_history_size){
		return "";
	}
	return command_history[index];
}
