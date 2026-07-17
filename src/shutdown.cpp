#include <myos>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char** argv) {
	bool reboot = true;
	int delay = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0) {
			reboot = false;
		}
		else if (strcmp(argv[i], "-t") == 0) {
			if (i + 1 < argc) {
				delay = atoi(argv[++i]);
			}
		}
	}
	printf("System will %s in %d seconds...\n", reboot ? "reboot" : "shutdown", delay);
	if (delay > 0) {
		if (fork() != 0) {
			return 0;
		}
		sleep(delay * 1000ull);
	}
	if (reboot) {
		::reboot();
	} else {
		shutdown();
	}
	return 0;
}