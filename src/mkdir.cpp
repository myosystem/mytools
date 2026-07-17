#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) { printf("usage: mkdir <dir>\n"); return 1; }
    if (mkdir(argv[1]) != 0) { printf("mkdir: cannot create '%s'\n", argv[1]); return 1; }
    return 0;
}
