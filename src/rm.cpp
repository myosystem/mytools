#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>

static void rmtree(const char* path) {
    int fd = open(path, O_DIRECTORY);
    if (fd < 0) { unlink(path); return; }   // not a directory -> file
    char names[128][64];
    int  types[128];
    int  n = 0;
    uint64_t reclen;
    while (n < 128 && read(fd, &reclen, sizeof(reclen)) > 0) {
        char rest[300];
        read(fd, rest, (unsigned)(reclen - sizeof(reclen)));
        int type = (unsigned char)rest[0];
        char* nm = rest + 5;
        int i = 0;
        while (nm[i] && i < 63) { names[n][i] = nm[i]; i++; }
        names[n][i] = 0;
        types[n] = type;
        n++;
    }
    close(fd);
    char child[256];
    for (int k = 0; k < n; k++) {
        int p = 0;
        for (int i = 0; path[i]; i++) child[p++] = path[i];
        child[p++] = '/';
        for (int i = 0; names[k][i]; i++) child[p++] = names[k][i];
        child[p] = 0;
        if (types[k] == 1) rmtree(child);   // DT_DIR
        else unlink(child);
    }
    rmdir(path);
}

int main(int argc, char* argv[]) {
    bool recursive = false;
    const char* target = 0;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++)
                if (argv[i][j] == 'r' || argv[i][j] == 'R') recursive = true;
        } else target = argv[i];
    }
    if (!target) { printf("usage: rm [-r] <path>\n"); return 1; }
    if (recursive) { rmtree(target); return 0; }
    if (unlink(target) != 0) { printf("rm: cannot remove '%s'\n", target); return 1; }
    return 0;
}
