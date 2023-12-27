#include <stdio.h>
#include <termios.h>

#include"shell_terminal.h"

static struct termios orig_termios;

char get_char_wait_for_keypress(void) {
    struct termios raw;
    // Get stdin file descriptor (0 by default)
    int stdin_fileno = fileno(stdin);
    // Copy terminal io settings
    raw = orig_termios;
    // Set return condition at first byte being received (For input timeout you can use `raw.c_cc[VTIME] = secs`)
    raw.c_cc[VMIN] = 1;
    // Apply settings with new return condition
    tcsetattr(stdin_fileno, TCSANOW, &raw);
    // Get char with new conditions
    char c = getchar();
    // Restore old settings
    tcsetattr(stdin_fileno, TCSANOW, &orig_termios);
    return c;
}

int main(void) {
	struct termios raw;
	char c = get_char_wait_for_keypress();
	while(c != 'q'){
		char c = get_char_wait_for_keypress();
		printf("%d", c);
	}
	
	return 0;
}
