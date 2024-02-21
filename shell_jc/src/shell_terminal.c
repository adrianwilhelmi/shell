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
	shell_is_interactive = isatty(shell_terminal);
	
	if(shell_is_interactive){
		//make sure shell has control over terminal
		shell_pgid = getpgrp();
		while(tcgetpgrp(shell_terminal) != getpgrp()){
			shell_pgid = getpgrp();
			kill(-shell_pgid, SIGTTIN);
		}
		
		struct sigaction sa;
		sa.sa_handler = SIG_IGN;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;

		//ignore signals
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGQUIT, &sa, NULL);
		sigaction(SIGTSTP, &sa, NULL);
		sigaction(SIGTTIN, &sa, NULL);
		sigaction(SIGTTOU, &sa, NULL);
		sigaction(SIGCHLD, &sa, NULL);
		
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
}

void enable_raw_mode(){
	struct termios raw;
	fflush(stdout);
	tcgetattr(shell_terminal, &raw);
	cfmakeraw(&raw);
//	tcsetattr(shell_terminal, TCSADRAIN, &raw);
//	tcsetattr(shell_terminal, TCSAFLUSH, &raw);
	tcsetattr(shell_terminal, TCSANOW, &raw);
}

void disable_raw_mode(){
	tcsetattr(shell_terminal, TCSAFLUSH, &shell_terminal_modes);
}
