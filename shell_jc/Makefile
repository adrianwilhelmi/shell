LFLAG=-Iinclude

all:
	gcc -g $(LFLAG) -o shell src/shell.c src/shell_job.c src/shell_command.c src/shell_terminal.c src/shell_parse.c src/shell_builtin.c src/shell_sgrep.c src/shell_globbing.c src/shell_history.c
clean:
	rm shell
