#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) { printf("usage: rmdir <dir>\n"); return 1; }
    if (rmdir(argv[1]) != 0) { printf("rmdir: cannot remove '%s' (not empty or not a dir)\n", argv[1]); return 1; }
    return 0;
}
