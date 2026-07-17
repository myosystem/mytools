#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    const char* path = (argc >= 2) ? argv[1] : "/.";

    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        printf("ls: cannot open '%s'\n", path);
        return 1;
    }

    const int WIDTH = 80;
    int col = 0;

    while (1) {
        uint64_t reclen;
        int n = read(fd, &reclen, sizeof(reclen));
        if (n <= 0) break;

        char* rest = new char[reclen - sizeof(reclen)];
        read(fd, rest, (unsigned)(reclen - sizeof(reclen)));

        uint8_t type = (uint8_t)rest[0];
        char*   name = rest + 5;

        int len = 0;
        while (name[len]) len++;
        int is_dir = (type == DT_DIR);
        int field  = len + (is_dir ? 1 : 0); // 이름 + 디렉터리면 '/'

        if (col > 0) {
            if (col + 2 + field > WIDTH) { printf("\n"); col = 0; }
            else                         { printf("  "); col += 2; }
        }

        if (is_dir) printf("%s/", name);
        else        printf("%s", name);
        col += field;

        delete[] rest;
    }
    if (col > 0) printf("\n");

    close(fd);
    return 0;
}
