
#include<signal.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<sys/wait.h>

#include"lsh_terminal.h"
#include"lsh_command.h"
#include"lsh_job.h"

void initialize_job(Job*job){
	job->commands = NULL;
	job->number_of_commands = 0;
	job->pgid = 0;
	job->paths[0] = NULL;
	job->paths[1] = NULL;
	job->paths[2] = NULL;
	job->next = NULL;
}

void put_job_in_background(Job*job, int cont){
	if(cont){
		if(kill(-job->pgid, SIGCONT) < 0){
			perror("job continuation");
		}
	}
}

void put_job_in_foreground(Job*job, int cont){
	//job takes control over terminal
	tcsetpgrp(shell_terminal, job->pgid);
	if(cont){
		tcsetattr(shell_terminal, TCSADRAIN, &job->tmodes);
		if(kill(-job->pgid, SIGCONT) < 0){
			perror("job continuation");
		}
	}
	
	//wait for job to finish
//	wait_for_job(job);
	
	//put shell back into foreground
	tcsetpgrp(shell_terminal, shell_pgid);
	
	//get shell's terminal modes from backup
	tcgetattr(shell_terminal, &job->tmodes);
	tcsetattr(shell_terminal, TCSADRAIN, &shell_terminal_modes);
}

void set_redirects(Job*job){
	//analyses if theres < in the first command or if theres > or 2> in the last command
	//if so: redirects input, output or error output to path located in the word after the redirection command
	//input:
	//	commands (Command*) - commands
	//	number_of_commands (int) - number of commands
	//	paths[3] (char*) - array containing paths for redirection
	//		paths[0] - path to input
	//		paths[1] - path to output
	//		paths[3] - path to error output

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
			current_command->pid = pid;
			//set this group's pid as this process' pid if its the first process
			if(job->pgid == 0){
				job->pgid = current_command->pid;
			}
			setpgid(pid, job->pgid);
			
			int status;
			waitpid(pid, &status, 0);
		}
		close(fd_temp_out);
		close(fd_temp_in);
		fd_temp_in = pipe_fds[0];
		current_command = current_command -> next;
	}
	
//TODO:
//	wait_for_job(job);
	printf("background, %d\n", background);
	if(background){
		put_job_in_background(job, 0);
	}
	else{
		put_job_in_foreground(job, 0);
	}
}


