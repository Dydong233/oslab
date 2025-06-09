#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <regex.h>

#define BUFSIZE 4096
#define TOP_T 5

typedef struct syscall_node{
    char name[20];
    double sys_time;
    int state;
    struct syscall_node *next;
}syscall_node;
syscall_node *head_node = NULL;
unsigned long long current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000ULL + tv.tv_usec / 1000;
}
void add_to_list(char *line,int line_len)
{
    // get syscall_name&syscall_time
    regex_t regex;
    regmatch_t matches[3];
    char syscall[64], time_str[32];
    double time_val = 0;
    const char *pattern = "^([a-zA-Z_][a-zA-Z0-9_]*)\\(.*<([0-9]+\\.[0-9]+)>";
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        return;
    }
    if (regexec(&regex, line, 3, matches, 0) == 0) {
        int len1 = matches[1].rm_eo - matches[1].rm_so;
        int len2 = matches[2].rm_eo - matches[2].rm_so;
        strncpy(syscall, line + matches[1].rm_so, len1);
        syscall[len1] = '\0';
        strncpy(time_str, line + matches[2].rm_so, len2);
        time_str[len2] = '\0';
        time_val = atof(time_str);
        // printf("SYSCALL: %-20s  TIME: %.6f s\n", syscall, time_val);
    }
    regfree(&regex);
    // search the list
    syscall_node *now_point = head_node; 
    while(now_point != NULL)
    {
        if(strcmp(now_point->name,syscall) == 0)
        {
            now_point->sys_time += time_val;
            return;
        }
        now_point = now_point->next;
    }
    // insert to the list
    syscall_node *new_node = malloc(sizeof(syscall_node));
    strcpy(new_node->name,syscall);
    new_node->sys_time = time_val;
    new_node->next = head_node;
    head_node = new_node;
}
void print_list()
{
    syscall_node *now_point = head_node;
    double thread_arr[TOP_T+1];  // large -> small
    double total_time = 0;
    for(int i=0;i<=TOP_T;i++)   thread_arr[i] = -1;
    // get max TOP_T
    while(now_point != NULL)
    {
        now_point->state = 1;
        thread_arr[TOP_T] = now_point->sys_time;  
        total_time+= now_point->sys_time;
        for(int i=TOP_T-1;i>=0;i--)
            if(thread_arr[i]<thread_arr[i+1])
            {
                double tmp = thread_arr[i];
                thread_arr[i] = thread_arr[i+1];
                thread_arr[i+1] = tmp;
            } 
        now_point = now_point->next;
    }
    // output the result
    for(int i=0;i<TOP_T;i++)
    {
        if(thread_arr[i] == -1) break;
        now_point = head_node;
        while(now_point != NULL)
        {
            if(now_point->state==1 && now_point->sys_time==thread_arr[i])
            {
                int radio = (int)(now_point->sys_time/total_time*100);
                printf("%s (%d%%)\n", now_point->name, radio);
                break;
            }
            now_point = now_point->next;
        }
    }
    for (int i = 0; i < 30; i++) putchar('=');
    puts("");
    for (int i = 0; i < 80; i++) putchar('\0');
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    if (argc<2){
        printf("Need more argv...\n");
        return 0;
    }    
    // create an anonymous pipe
    int pipe_fd[2];
    if(pipe(pipe_fd) == -1) {  
        perror("pipe");
        exit(1);
    }
    // fork a thread
    pid_t pid = fork();
    if(pid == -1)  {
        perror("fork");
        exit(1);
    }
    // child thread
    if(pid==0){
        close(pipe_fd[0]);  // close the read
        dup2(pipe_fd[1],STDOUT_FILENO);
        dup2(pipe_fd[1],STDERR_FILENO);
        close(pipe_fd[1]);  // close the write

        // construct the args
        int strace_argc = argc+4;
        char **new_argv = calloc(strace_argc+1,sizeof(char *));
        new_argv[0] = "/usr/bin/strace";
        new_argv[1] = "-T";
        new_argv[2] = "-e";
        new_argv[3] = "trace=all";
        for(int i=1;i<argc;i++) new_argv[i+3] = argv[i];
        new_argv[strace_argc] = NULL;
        extern char **environ;
        execve("/usr/bin/strace", new_argv, environ);
        perror("execve");
        exit(1);
    } 
    // father child
    close(pipe_fd[1]);
    unsigned long long last_time = current_time_ms();
    char buf[BUFSIZE];
    size_t bytes_read;
    char line[BUFSIZE];
    size_t line_len = 0;
    while(233)
    {
        bytes_read = read(pipe_fd[0],buf,BUFSIZE);
        if(bytes_read <= 0) break;
        for(int i=0;i<bytes_read;i++)
            if(buf[i] == '\n'){ 
                // add to list
                add_to_list(line,line_len); 
                line_len = 0;
                memset(line,0,sizeof line);
            }
            else if(line_len < BUFSIZE-1)   line[line_len++] = buf[i];
        unsigned long long now_time = current_time_ms();
        if(now_time > last_time){
            // printf("[STATS] Output at %.3lf s\n", (double)now_time/1000);
            // analysis the syscall's time
            print_list(); 
            // printf("=================================================\n");
            last_time = now_time;
        }
    }
    // print_debug();
    wait(NULL);
    return 0;
}
