#include<signal.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<errno.h>

#include"shell_terminal.h"
#include"shell_command.h"
#include"shell_job.h"

void initialize_job(Job*job){
	//initializez the job so it's ready to be used
	
	job->commands = NULL;
	job->number_of_commands = 0;
	job->pgid = 0;
	job->paths[0] = NULL;
	job->paths[1] = NULL;
	job->paths[2] = NULL;
	job->next = NULL;
}

void free_job(Job*job){
	//cleans the memory
	
	if(job == NULL){
		return;
	}
	
	free_commands(job->commands, job->number_of_commands);
	if(job->paths[0] != NULL){ 
		free(job->paths[0]);
	}
	if(job->paths[1] != NULL){ 
		free(job->paths[1]);
	}
	if(job->paths[2] != NULL){ 
		free(job->paths[2]);
	}
	free(job);
}

int is_stopped(Job*job){
	//checks if job is either running (0) or stopped (1)
	
	Command*command;
	command = job->commands;
	for(int i = 0; i < job->number_of_commands; ++i){
		if(!command->completed && !command->stopped){
			return 0;
		}
	}
	return 1;
}

int is_completed(Job*job){
	//checks if job is either completed (1) or running/stopped (0)
	
	Command*command;
	command = job->commands;
	for(int i = 0; i < job->number_of_commands; ++i){
		if(!command->completed){
			return 0;
		}
	}
	return 1;
}

void put_job_in_background(Job*job, int cont){
	//shell has control over terminal
	//continue job if cont is true
	
	signal(SIGCONT, SIG_DFL);
	
	if(cont){
		if(kill(-job->pgid, SIGCONT) < 0){
			perror("job continuation");
		}
	}
	
	tcgetattr(shell_terminal, &job->tmodes);
	
	signal(SIGCONT, SIG_IGN);
}

void put_job_in_foreground(Job*job, int cont){
	//job takes control over terminal
	//continue job if cont is true
	
	tcsetpgrp(shell_terminal, job->pgid);
	if(cont){
		tcsetattr(shell_terminal, TCSADRAIN, &job->tmodes);
		if(kill(-job->pgid, SIGCONT) < 0){
			perror("job continuation");
		}
	}
	
	//ait for job to finish
	//listen to signals from child so we can report
	signal(SIGCHLD, SIG_DFL);
	wait_for_job(job);
	signal(SIGCHLD, SIG_IGN);
	
	//put shell back into foreground
	tcsetpgrp(shell_terminal, shell_pgid);
	
	//get shell's terminal modes from backup
	tcgetattr(shell_terminal, &job->tmodes);
	tcsetattr(shell_terminal, TCSADRAIN, &shell_terminal_modes);
}

int check_process_status(pid_t pid, int status){
	//updates status of the process with given pid
	//searches through jobs and if process with given pid was found:
	//	checks if process was stopped with WIFSTOPPED and updates flag
	//	checks if process was terminated due to a signal with WIFSIGNALED and updates flag
	//input:
	//	pid (pid_t) - pid of process we want to check
	//	status (int) - status returned by waitpid();
	//output:
	//	0 if process was found and status was changed properly
	//	-1 otherwise
	
	Job*job;
	Command*command;
	
	
	if(pid > 0){
		for(job = first_job; job != NULL; job = job->next){
			command = job->commands;
			for(int i = 0; i < job->number_of_commands; ++i){
				if(command->pid == pid){
					command->status = status;
					if(WIFSTOPPED(status)){
						command->stopped = 1;
					}
					else {
						command->completed = 1;
						if(WIFSIGNALED(status)){
							printf("process %d: terminated by signal %d.\n", (int) pid, WTERMSIG(command->status));
						}
					}
					return 0;
				}
				command = command->next;
			}
		}
		return -1;
	}
	else if(pid == 0 || errno == ECHILD){
		//errno = ECHILD means theres no child processes
		return -1;
	}
	else{
		perror("waitpid");
		return -1;
	}

}

void update_status(){
	//check if any process changed it's status
	
	int status;
	pid_t pid;
	
	do{
		pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
	}while(!check_process_status(pid, status));
}

void wait_for_job(Job*job){
	//wait for job untill it either stops or completes
	
	int status;
	pid_t pid;
	 
	do{
		pid = waitpid(WAIT_ANY, &status, WUNTRACED);
		if(pid > 0){
			check_process_status(pid, status);
		}
	}while(!is_stopped(job) && !is_completed(job));
}

void update_job_queue(){
	//removes terminated jobs from the list
	
	Job*job;
	Job*job_last;
	Job*job_next;
	Command*command;
	
	signal(SIGCHLD, SIG_DFL);
	update_status();
	signal(SIGCHLD, SIG_DFL);
	
	job_last = NULL;
	for(job = first_job; job != NULL; job = job_next){
		job_next = job->next;
		
		if(is_completed(job)){
			if(job_last != NULL){
				job_last->next = job->next;
			}
			else{
				first_job = job_next;
			}
			free_job(job);
		}
		else{
			job_last = job;
		}	
	}
}

void print_job_info(){
	//prints info about recent job statuses - if job was stopped, done, or is it running
	
	Job*job;
	Job*job_last;
	Job*job_next;
	Command*command;
	
	update_job_queue();
	//update status info for child processes
	/*
	signal(SIGCHLD, SIG_DFL);
	update_status();
	signal(SIGCHLD, SIG_IGN);
	*/
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
		command = job->commands;
		for(int i = 0; i < job->number_of_commands; ++i){
			int k = 0;
			while(command->command[k] != NULL){
				printf("%s ",job->commands->command[k]);
				k++;
			}
			command = command->next;
		}
		printf("\n");
	}
}

void continue_job(Job*job, int background){	
	//continue given stopped job 
	//background flag decides if it should run in background or foreground
	
	//set flags as appropriate	
	Command*command = job->commands;
	for(int i = 0; i < job->number_of_commands; ++i){
		command->stopped = 0;
	}
	//mark as running
	job->notified = 0;
	
	//actually run the job
	if(background){
		put_job_in_background(job, 1);
	}
	else{
		put_job_in_foreground(job, 1);
	}
}

void set_redirects(Job*job){
	//analyses if theres < in the first command or if theres > or 2> in the last command
	//if so: redirects input, output or error output to path located in the word after the redirection command
	//input:
	//	job (Job*) - piped commands that we want to redirect

	int index = 0;
	
	Command*commands = job->commands;
	
	//only first command can redirect input
	while(commands->command[index+1] != NULL){
		if(strcmp(commands->command[index], "<") == 0){
			//copy file after > to paths[0]
			job->paths[0] = strdup(commands->command[index+1]);
			
			//remove ["<", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		++index;
	}
	
	//go to last command
	for(int i = 0; i < job->number_of_commands - 1; ++i){
		commands = commands->next;
	}
	
	index = 0;
	
	//only last command can redirect output and err output (first command can be last if it's the only one)
	while(commands->command[index+1] != NULL){
		if(strcmp(commands->command[index], ">") == 0){
			//copy file after > to paths[1]
			job->paths[1] = strdup(commands->command[index+1]);
			
			//remove [">", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		else if(strcmp(commands->command[index], "2>") == 0){
			//copy file after > to paths[2]
			job->paths[2] = strdup(commands->command[index+1]);
			
			//remove ["2>", "path"] and concatenate words before and after it
			int i = index;
			while(commands->command[i] != NULL){
				++i;
			}
			for(int j = index; j < i-2; ++j){
				free(commands->command[j]);
				commands->command[j] = strdup(commands->command[j + 2]);
			}
			free(commands->command[i]);
			free(commands->command[i-1]);
			free(commands->command[i-2]);
			commands->command[i-2] = NULL;
			break;
		}
		++index;
	}
}


void handle_job(Job*job, int background){
	//sets redirections, sets up pipes and launches commands of given job
	//if background flag is true then processes run in background
	
	//backup terminal input and output
	int fd_terminal_input = dup(STDIN_FILENO);			//terminal input
	int fd_terminal_output = dup(STDOUT_FILENO);			//terminal output
	
	int fd_redirected_output = dup(STDOUT_FILENO);		//if not redirected its same as terminal outut, input wont be needed
	
	int fd_temp_in = dup(fd_terminal_input);			//initially input is from terminal
	int fd_temp_out;
	int fd_temp_err = dup(STDERR_FILENO);
	
	//handle redirecting
	set_redirects(job);
	
	//create fds for new files if redirecting
	//< redirection
	if(job->paths[0] != NULL){
		int new_input_fd = open(job->paths[0], O_RDONLY | O_CREAT);
		dup2(new_input_fd, fd_temp_in);	
		close(new_input_fd);
	}
	//> redirection
	if(job->paths[1] != NULL){
		//now only open output file, it will be truncated later
		int new_output_fd = open(job->paths[1], O_WRONLY | O_CREAT);
		dup2(new_output_fd, fd_redirected_output);
		close(new_output_fd);
	}
	//2> redirection
	if(job->paths[2] != NULL){
		int new_output_err_fd = open(job->paths[2], O_RDWR | O_CREAT | O_TRUNC);
		dup2(new_output_err_fd, fd_temp_err);
		close(new_output_err_fd);
	}
	
	
	Command*current_command = job->commands;
	int pipe_fds[2];
	pid_t pid;
	for(int i = 0; i < job->number_of_commands; ++i){
		if(i == job->number_of_commands -1){	
			if(job->paths[1]){
				//need to clear the > redirect file before writing to it
				int truncate_redirect = open(job->paths[1], O_TRUNC);
				close(truncate_redirect);
			}
			fd_temp_out = dup(fd_redirected_output);
		}
		else{
			if(pipe(pipe_fds) < 0){
				perror("pipe err");
				exit(EXIT_FAILURE);
			}
			fd_temp_out = pipe_fds[1];
		}
		
		pid = fork();
		if(pid < 0){
			perror("fork err");
			exit(EXIT_FAILURE);
		}
		if(pid == 0){
			//child
			handle_command(current_command, fd_temp_in, fd_temp_out, fd_temp_err, job->pgid, background);
		}
		else{
			//pa
			current_command->completed = 0;
			current_command->stopped = 0;
			current_command->pid = pid;
			//set this group's pid as this process' pid if its the first process
			if(job->pgid == 0){
				job->pgid = pid;
			}
			setpgid(pid, job->pgid);
			job->notified = 0;
		}

		close(fd_temp_out);
		close(fd_temp_in);
		fd_temp_in = pipe_fds[0];
		current_command = current_command -> next;
	}
	
	if(!background){
		put_job_in_foreground(job, 0);
	}
	else if (background){
		put_job_in_background(job, 0);
	}
}


