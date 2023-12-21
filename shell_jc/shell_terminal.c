#include<sys/types.h>
#include<termios.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>

#include"shell_terminal.h"

void shell_initialization(){
	//initialize shell
	shell_terminal = STDIN_FILENO;
	
	//make sure shell has control over terminal
	shell_pgid = getpgrp();
	while(tcgetpgrp(shell_terminal) != getpgrp()){
		shell_pgid = getpgrp();
		kill(-shell_pgid, SIGTTIN);
	}

	//ignore signals
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	
	//put shell in it's own process group
	shell_pgid = getpid();
	if(setpgid(shell_pgid, shell_pgid) < 0){
		perror("couldnt put shell in its own process group");
		exit(EXIT_FAILURE);
	}
	
	//get control of the terminal by setting its group pid as shell's group id
	tcsetpgrp(shell_terminal, shell_pgid);

	//save terminal attributes
	tcgetattr(shell_terminal, &shell_terminal_modes);
}

void enable_raw_mode(){
	struct termios raw = shell_terminal_modes;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(shell_terminal, TCSAFLUSH, &raw);
}

void disable_raw_mode(){
	tcsetattr(shell_terminal, TCSAFLUSH, &shell_terminal_modes);
}
