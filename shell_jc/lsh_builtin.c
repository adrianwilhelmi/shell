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
	continue_job(first_job, 0);
	return 1;
}

int handle_bg(Command*command){
	if(first_job == NULL){
		printf("no jobs avail\n");
		return 1;
	}
	continue_job(first_job, 1);
	return 1;
}
int handle_jobs(Command*command){
	print_job_info();
	update_job_queue();
	return 1;
}
