运行qemu:

```bash
sudo make qemu
```

xv6 has no `ps` command, but, if you type Ctrl-p, the kernel will print information about each process. 

If you try it now, you'll see two lines: one for `init`, and one for `sh`.

To quit qemu type: Ctrl-a x.

debug: [MIT 6.S081 xv6调试不完全指北 - KatyuMarisa - 博客园 (cnblogs.com)](https://www.cnblogs.com/KatyuMarisaBlog/p/13727565.html)

头文件必须按照这个顺序引用, 因为有先后依赖顺序...

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
```







## `user/sleep.c`

没有参数就报错. 使用系统调用`sleep`.See `kernel/sysproc.c` for the xv6 kernel code that implements the `sleep` system call (look for `sys_sleep`), `user/user.h` for the C definition of `sleep` callable from a user program, and `user/usys.S` for the assembler code that jumps from user code into the kernel for `sleep`.

```c
#include "user/ulib.c"
#include "kernel/"

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("sleep should have arguments\n");
        exit(0);
    }
    sleep(atoi(argv[1]));
    exit(0);
}
```

## `user/pingpong.c`

pingpong指乒乓球, 这里的意思是子进程与父进程之间打乒乓球, The parent should send a byte to the child; the child should print "<pid>: received ping", where <pid> is its process ID, write the byte on the pipe to the parent, and exit; the parent should read the byte from the child, print "<pid>: received pong", and exit. 

```c
#include "user/user.h"

int p1[2], p2[2];
int main() {
    int pid;
    char buf[14];
    pipe(p1), pipe(p2);
    pid = fork();
    if(pid == 0) {
        // child process
        read(p1[0], buf, sizeof(buf));
        printf("%d: reveiving ping", getpid());
        write(p2[1], "Hello, world\n", 13);
        close(p2[1]);
        close(p2[0]);
    } else {
        write(p1[1], "Hello, world\n", 13);
        close(p1[1]);
        close(p1[0]);
        read(p2[0], buf, sizeof(buf));
        printf("%d: reveiving pong", getpid());
    }

}
```

## /user/primes.c

多进程(?)的筛法求素数

原理:

```python
p = get a number from left neighbor
print p
loop:
    n = get a number from left neighbor
    if (p does not divide n)
        send n to right neighbor
```

![img](https://gitee.com/oldataraxia/pic-bad/raw/master/img/sieve.gif)

> [Bell Labs and CSP Threads (swtch.com)](https://swtch.com/~rsc/thread/)

这种情况下显然是应该用递归的...

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void sieving(int *p) {
    int div, i;
    int p1[2];
    pipe(p1);
    if(read(p[0], &div, sizeof(int)) == 0) {
        exit(0);
    }
    printf("prime %d\n", div);
    while(read(p[0], &i, sizeof(int)) != 0) {
        if(i % div != 0) {
            write(p1[1], &i, sizeof(int));
        }
    }
    close(p1[1]);
    if(fork() == 0) {
        sieving(p1);
    } else {
        wait((int *) 0);
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int p[2];
    pipe(p);
    for(int i = 2; i <= 35; i++ ) {
        write(p[1], &i, sizeof(int));
    }
    close(p[1]);
    sieving(p);
    return 0;
}
```

## /user/find.c

实现的功能是find all files in a directory tree with a specific name. Note that == does not compare strings like in Python. Use strcmp() instead.

首先来看一下`ls.c`是怎么实现的:

```c
void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/'; // 此时buf是"path/"
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}
```

这里用到了两个结构体, dirent和stat, 前者是目录文件中文件信息的存放格式, 后者是通过`stat`函数拿到的文件信息

```c
// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum; //  == 0说明无文件
  char name[DIRSIZ];
};

struct stat {
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short type;  // Type of file
  short nlink; // Number of links to file
  uint64 size; // Size of file in bytes
};
```

`memmove`是c标准库函数, 原版c的`string.h`里, xv6在/user/ulibc里

>  **void \*memmove(void \*str1, const void \*str2, size_t n)** 从 **str2** 复制 **n** 个字符到 **str1**，但是在重叠内存块这方面，memmove() 是比 memcpy() 更安全的方法。如果目标区域和源区域有重叠的话，memmove() 能够保证源串在被覆盖之前将重叠区域的字节拷贝到目标区域中，复制后源区域的内容会被更改。

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void find(char *path, char *filename) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        printf("finding arrange must be a dictionary\n");
        break;

    case T_DIR:
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("ls: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf); // p->buf[strlen(buf)]
        *p++ = '/'; // 此时buf是"path/"
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0 || !strcmp(de.name, ".") || !strcmp(de.name, "..")) continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if(st.type == T_DIR) {
                find(buf, filename);
            } else if (!strcmp(de.name, filename)) {
                printf("%s\n", buf);
            }
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("wrong args format\n");
    } else {
        find(argv[1], argv[2]);
    }
    exit(0);
}
```



![image-20220101161521153](https://gitee.com/oldataraxia/pic-bad/raw/master/img/image-20220101161521153.png)

## /user/xargs

read lines from the standard input and run a command for each line, supplying the line as arguments to the command. 

这个命令做的事情就是把标准输入作为参数一起输入到`xargs`后面跟的命令里.

这个命令一般的用法:用在管道里, lecture里说过管道会通过重定向标准输入之后fork的形式实现...

```
somecommand |xargs -item  command
```

![image-20220101163818985](https://gitee.com/oldataraxia/pic-bad/raw/master/img/image-20220101163818985.png)

这里我一开始还理解错了这个命令的意思...是说每一行对应一次命令调用, 所以每一行内的参数通过空格来分隔, 每读到`\n`就进行一次调用并抛弃之前的参数.

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char* argv[]) {
    char buf[512]; // 相当于读入时的缓冲池
    char* newargv[MAXARG];
    char c;
    int idx = 0, args = argc - 1; // 分别用于buf和args的计数
    if(argc < 2) {
        printf("wrong argvs format\n");
    }
    for(int i = 1; i < argc; i++) {
        newargv[i - 1] = argv[i];
    }
    while(read(0, &c, sizeof(char)) != 0 ) {
        if(c == ' ' || c == '\n') {
            buf[idx++] = '\0';
            newargv[args] = malloc(sizeof(char) * strlen(buf));
            strcpy(newargv[args++], buf);
            idx = 0;
            if(c == '\n') {
                newargv[args++] = 0;
                if(fork() == 0) {
                    // printf("%s\n", newargv[0]);
                    exec(newargv[0], newargv);
                    exit(0);
                }
                wait((int *)0);           
                args = argc - 1; //初始值了2333
            }
        } else {
            buf[idx++] = c;
        }
    } 
    if(args != argc - 1) {
        // 最后一行不为空
        newargv[args++] = 0;
        if(fork() == 0) {
            exec(newargv[0], newargv);
            exit(0);
        }
        wait((int *)0);           
    }
    exit(0);
}
```

![image-20220101180610881](https://gitee.com/oldataraxia/pic-bad/raw/master/img/image-20220101180610881.png)

> 基本上没什么难度, 感觉难度更多的来自于C没什么语言特性...

