#include <unistd.h>

int main() {
	write(1, "\033[2J\033[H", 7); // ANSI escape code to clear screen and move cursor to home
	return 0;
}