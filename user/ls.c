#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path) {
    static char buf[DIRSIZ + 1]; // 静态缓冲区，DIRSIZ是目录项名称最大长度
    char *p;

    // 从路径末尾向前找，找到最后一个'/'后的文件名
    // strlen 计算字符串的长度（不包括结尾的\0）。
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;

    // 指向文件名开始
    p++;

    // Return blank-padded name.
    // 如果文件名长度 >= DIRSIZ，直接返回
    if (strlen(p) >= DIRSIZ)
        return p;

    // 否则复制文件名到buf，并用空格填充到DIRSIZ长度
    // 对齐输出，让文件名列在终端中整齐排列：
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void ls(char *path) {
    char buf[512], *p;
    int fd;
    // 目录项
    struct dirent de;
    struct stat st;

    // 打开文件/目录
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    // 获取文件状态信息
    if (fstat(fd, &st) < 0) {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    // 如果是设备或普通文件，直接打印信息
    case T_DEVICE:
    case T_FILE:
        printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
        break;

    case T_DIR:
        // 如果是目录，需要遍历目录项
        // 检查路径长度是否会溢出缓冲区
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("ls: path too long\n");
            break;
        }

        // 构造路径前缀 "path/"
        strcpy(buf, path);
        p = buf + strlen(buf);
        // 替换 '\0'
        *p++ = '/';

        // 在Unix/xv6文件系统中，目录文件的内容是一系列的dirent结构：
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0) // 跳过已删除的条目
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0; // 确保字符串终止

            // 获取该条目的状态信息
            if (stat(buf, &st) < 0) {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }

            // 打印条目信息
            printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    int i;

    if (argc < 2) {
        ls("."); // 没有参数时列出当前目录
        exit(0);
    }
    // 依次处理所有参数
    for (i = 1; i < argc; i++)
        ls(argv[i]);
    exit(0);
}