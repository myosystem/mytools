#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    const char* path = (argc >= 2) ? argv[1] : "/.";

    int fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        printf("dir: cannot open '%s'\n", path);
        return 1;
    }

    int total = 0;

    while (1) {
        uint64_t reclen;
        int n = read(fd, &reclen, sizeof(reclen));
        if (n <= 0) break;

        char *rest = new char[reclen - sizeof(reclen)];
        read(fd, rest, (unsigned)(reclen - sizeof(reclen)));

        uint8_t  type  = (uint8_t)rest[0];
        uint32_t fsize = *(uint32_t*)(rest + 1);
        char*    name  = rest + 5;

        if (type == DT_DIR)
            printf("[DIR] %s\n", name);
        else
            printf("      %s  (%u bytes)\n", name, fsize);

        total++;
		delete[] rest;
    }

    close(fd);
    printf("\n%d item(s)\n", total);
    return 0;
}
