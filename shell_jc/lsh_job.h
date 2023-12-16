#ifndef LSH_JOB_H
#define LSH_JOB_H

#include<termios.h>

#include"lsh_terminal.h"
#include"lsh_command.h"

typedef struct Job{
	//linked list of jobs. each job is representing a pipeline of commands
	Command*commands;		//commands
	int number_of_commands;	//number of commands in the pipeline
	pid_t pgid;			//process group id
	char*paths[3];			//paths redirected with <, > or 2>
	struct Job*next;		//pointer to the next job
	
	struct termios tmodes;
} Job;

void initialize_job(Job*job);
void put_job_in_background(Job*job, int cont);
void put_job_in_foreground(Job*job, int cont);
void set_redirects(Job*job);
void handle_job(Job*job, int background);

#endif
