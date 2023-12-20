#include"lsh_builtin.h"
#include"lsh_job.h"

#include<stdio.h>
#include<signal.h>

int handle_cd(Command*command){
	//handle cd built in function
	
	if(command->command[1] == NULL){
		printf("enter path\n");
		return 1;
	}
	else{
		if(command->command[2] != NULL){
			printf("too many argumentz\n");
			return 1;
		}
		if(chdir(command->command[1]) != 0){
			perror("cd");
		}
	}
	return 1;
}

int handle_exit(Command*command){
	//handle exit built in function
	
	free_commands(command, 1);
	printf("exit\n");
	exit(EXIT_SUCCESS);
	return 1;
}

int handle_fg(Command*command){
	if(first_job == NULL){
		printf("no jobs avail\n");
		return 1;
	}
	
	if(command->command[1] != NULL){
		long job_number;
		char*message;
		job_number = strtol(command->command[1], &message, 10);
		if(message == command->command[1]){
			printf("no such job\n");
			return 1;
		}
		else{
			if(job_number < 1){
				printf("no such job\n");
				return 1;
			}
			
			Job*job_to_continue = first_job;
			while(job_number != 1 && job_to_continue != NULL){
				job_to_continue = job_to_continue->next;
				--job_number;
			}
			if(job_to_continue == NULL){
				printf("no such job\n");
				return 1;
			}
			continue_job(job_to_continue, 0);
			return 1;
		}
	}
	
	continue_job(first_job, 0);
	return 1;
}

int handle_bg(Command*command){
	if(first_job == NULL){
		printf("no jobs avail\n");
		return 1;
	}
	
	if(command->command[1] != NULL){
		long job_number;
		char*message;
		job_number = strtol(command->command[1], &message, 10);
		if(message == command->command[1]){
			printf("no such job\n");
			return 1;
		}
		else{
			if(job_number < 1){
				printf("no such job\n");
				return 1;
			}
			
			Job*job_to_continue = first_job;
			while(job_number != 1 && job_to_continue != NULL){
				job_to_continue = job_to_continue->next;
				--job_number;
			}
			if(job_to_continue == NULL){
				printf("no such job\n");
				return 1;
			}
			continue_job(job_to_continue, 1);
			return 1;
		}
	}
	
	continue_job(first_job, 1);
	return 1;
}
int handle_jobs(Command*command){
	print_job_info();
	update_job_queue();
	return 1;
}
