#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *file_name) {

    struct dirent de;
    struct stat st;
    int fd;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type != T_DIR) {
        fprintf(2, "find: %s isnot dir \n", path);
        close(fd);
        return;
    }

    char buffer[512] = {0};
    char *p;
    memmove(buffer, path, strlen(path));

    p = buffer + strlen(buffer);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if (stat(buffer, &st) < 0) {
            printf("find: cannot stat %s\n", buffer);
            continue;
        }

        switch (st.type) {
        case T_FILE:
            if (strcmp(p, file_name) == 0) {
                printf("%s\n", buffer);
            }
            break;
        case T_DIR:
            if (strcmp(p, ".") != 0 && strcmp(p, "..") != 0) {
                find(buffer, file_name);
            }
            break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "find: need three params\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}