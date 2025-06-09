# M5

## 实验指南

这个实验本质是把strace和perf进行结合，从而开发一个感知系统调用时间的工具。

限制要求为：为了强迫大家理解 execve 系统调用，sperf 实现时，只能使用 execve 系统调用；使用 glibc 对 execve 的包装 (execl, execlp, execle, execv, execvp, execvpe) 将导致编译错误。在实际编程中，exec 系列函数，例如以 p (path) 结尾的函数会从 PATH 环境变量中搜索可执行文件。

---

## Frame

```c
// 本质上通过fork父子进程进行通信，子进程执行相关的程序，通过管道给父进程信息
// 父进程不断处理数据，然后通过计时器感知时间，通过链表进行排序，输出结果
[Parent Process]
    |
    |--- create pipe (fd[2])
    |
    |--- fork()
         |
         |--- [Child Process]
         |      |
         |      |--- dup2(pipe_write, STDOUT_FILENO)
         |      |--- execve("strace", argv, environ)
         |
         |--- [Parent Process]
                |
                |--- read(pipe_read)
                |--- parse strace output
                |--- every 100ms print stats
```

---

