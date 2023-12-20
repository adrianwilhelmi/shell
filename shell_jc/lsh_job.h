#ifndef __LSH_JOB_H__
#define __LSH_JOB_H__

#include<termios.h>

#include"lsh_terminal.h"
#include"lsh_command.h"

typedef struct Job{
	//linked list of jobs. each job is representing a pipeline of commands
	Command*commands;		//commands
	int number_of_commands;	//number of commands in the pipeline
	pid_t pgid;			//process group id
	char*paths[3];			//paths redirected with <, > or 2>
	int notified;			//if user is notified about status of the job
	struct termios tmodes;	//needed to backup terminal state before job taking control
	struct Job*next;		//pointer to the next job
} Job;

extern Job*current_job;
extern Job*first_job;
extern int background;

void initialize_job(Job*job);
void free_job(Job*job);
int is_stopped(Job*job);
int is_completed(Job*job);
void put_job_in_background(Job*job, int cont);
void put_job_in_foreground(Job*job, int cont);
int check_process_status(pid_t pid, int status);
void update_status();
void wait_for_job(Job*job);
void print_job_info();
void continue_job(Job*job, int background);
void set_redirects(Job*job);
void handle_job(Job*job, int background);
void update_job_queue();

#endif
