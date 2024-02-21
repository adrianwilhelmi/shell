#include"shell_globbing.h"

#include<string.h>
#include<wordexp.h>

void handle_globbing_patterns(Command*input_command){
	//handles globalization (*, [a-z], ?, etc.) in given command
	//creates new array of words with expanded glob patterns and replaces command in input_command with the new one
	
	wordexp_t glob_results;
	
	char**handled_command = malloc(sizeof(char*));
	int handled_command_wordc = 1;
	int index = 0;
	
	int j = 0;
	while(input_command->command[j] != NULL){
		if(wordexp(input_command->command[j], &glob_results, 0) == 0){
			handled_command_wordc += glob_results.we_wordc;
			handled_command = realloc(handled_command, sizeof(char*)*handled_command_wordc);
			
			for(int k = 0; k < glob_results.we_wordc; k++){
				handled_command[index] = strdup(glob_results.we_wordv[k]);
				++index;
			}
			wordfree(&glob_results);
		}
		else{
			handled_command_wordc += 1;
			handled_command = realloc(handled_command, sizeof(char*)*handled_command_wordc);
			handled_command[index] = strdup(input_command->command[j]);
			++index;
		}
		++j;
	}
	handled_command[index] = NULL;
	
	input_command->command = realloc(input_command->command, sizeof(char*)*handled_command_wordc);
	j = 0;
	while(input_command->command[j] != NULL){
		free(input_command->command[j]);
		input_command->command[j] = strdup(handled_command[j]);
		free(handled_command[j]);
		++j;
	}
	index = j;
	while(handled_command[index] != NULL){
		input_command->command[index] = strdup(handled_command[index]);
		free(handled_command[index]);
		++index;
	}
	input_command->command[index] = NULL;
	free(handled_command);
}
