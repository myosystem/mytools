#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc == 2) {
		FILE* file = fopen(argv[1], "rb");
		if (file) {
			char buffer[256];
			fscanf(file, "%s", buffer);
			printf("%s\n", buffer);
			fclose(file);
		}
		else {
			printf("Failed to open file: %s\n", argv[1]);
		}
	}
	else {
		printf("Usage: %s <file>\n", argv[0]);
	}
	return 0;
}