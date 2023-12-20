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
	
	signal(SIGCHLD, SIG_DFL);
	update_status();
	signal(SIGCHLD, SIG_IGN);
	
	Job*job;
	int counter = 1;
	for(job = first_job; job != NULL; job = job->next){
		printf("[%d]: ", counter);
		counter++;
		if(is_completed(job)){
			printf("Done ");
		}
		else if(is_stopped(job)){
			printf("Stopped ");
		}
		else{
			printf("Running ");
		}
		for(int i = 0; i < job->number_of_commands; ++i){
			printf("%s ",job->commands->command[i]);
		}
		printf("\n");
	}
	
	return 1;
}
