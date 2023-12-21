#include<stdio.h>

#include"shell_history.h"
#include"shell_terminal.h"
#include"shell_command.h"

void handle_arrows(char c){
	if(c == '\x1b'){
		char seq[3];
		if(read(shell_terminal, &seq[0], 1) != 1){
			return;
		}
		if(read(shell_terminal, &seq[1], 1) != 1){
			return;
		}
		
		if(seq[0] == '['){
			switch(seq[1]){
				case 'A':
					if(command_history_curr == NULL){
						command_history_curr = command_history->command;
					}
					else{
						while(command_history_curr != command_history->command){
							command_history = command_history->next;
						}
						if(command_history->next != NULL){
							command_history = command_history->next;
						}
						command_history_curr = command_history->command;
					}
					return;
				case 'B':
					if(command_history_curr == NULL){
						return;
					}
					else{
						while(command_history_curr != command_history->command){
							command_history = command_history->prev;
						}
						if(command_history->prev != NULL){
							command_history = command_history->prev;
						}
						command_history_curr = command_history->command;
					}
					return;
			}
		}
	}
}
