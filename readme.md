# Read-Eval-Print-Loop

## 实验描述

`crepl` - 逐行从 stdin 中输入单行 C 语言代码，并根据输入内容分别处理，同时不会调用一些未定义或者非法的函数，实际上是编写一个建议的交互式解释器：

- 如果输入一个 C 函数的定义，则把函数编译并加载到进程的地址空间中；
- 如果输入是一个 C 语言表达式，则把它的值输出。

## 实验指南

### 解析读入的命令

```c
while (1) {
    printf("crepl> ");
    fflush(stdout);

    if (!fgets(line, sizeof(line), stdin)) {
        break;
    }

    // To be implemented.
    printf("Got %zu chars.\n", strlen(line));
}
```

在上面的代码里，如果读入的字符串以 `int` 开头，你就可以假设是一个函数；否则就可以假设是一个表达式。

### 把函数编译成共享库

这个实验最核心的技术处理，就是在程序的外部，通过另一个进程完成一小段代码到二进制代码的编译，所以需要维护一个外部的进程。比如：

```c
int gcd(int a, int b) { return b ? gcd(b, a % b) : a; }
```

可以用编译器把它翻译为指令序列：

```assembly
0:    endbr64 
4:    mov    %edi,%eax
6:    test   %esi,%esi
8:    je     13
a:    cltd   
b:    idiv   %esi
d:    mov    %esi,%eax
f:    mov    %edx,%esi
11:    jmp    6
13:    ret 
```

需要把这些代码封装在一个共享库中，在其它程序运行的时候可以被加载。使用mkstemp family API，在 `/tmp`目录下创建临时文件，在编译的时候加入如下选项。
### 把表达式编译成共享库

表达式也可以通过共享库的思维，把不同的表达式用`wrapper`封装起来，然后就可以编译成不同的共享库。如以下，把动态库加载到地址空间并得到 `__expr_wrapper_4` 的地址，直接进行函数调用就能得到表达式的值了。

```c
int __expr_wrapper_4() {
    return gcd(256, 144);
}
```

### 共享库的加载

当有一个.so文件的时候可以使用动态链接库的功能来加载它，分为以下几个步骤：

1. **包含必要的头文件**：你需要包含 `<dlfcn.h>` 头文件，它提供了动态加载库所需的 `dlopen`、`dlsym` 和 `dlclose` 函数。
2. **打开共享库**：使用 `dlopen()` 函数加载 `.so` 文件。你需要提供库的路径和加载模式（通常是 `RTLD_LAZY` 或 `RTLD_NOW`）。
3. **获取函数指针**：使用 `dlsym()` 函数获取共享库中函数的地址。你需要提供 `dlopen` 返回的句柄和函数名。
4. **调用函数**：通过函数指针调用函数。
5. **关闭共享库**：使用 `dlclose()` 关闭加载的库。

------







