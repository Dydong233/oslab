#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

int write_function_to_file(const char *function_body)
{
    // check the line's syntax
    char tmp_file[] = "/tmp/tmp_func_XXXXXX.c";
    int fd = mkstemps(tmp_file,2);
    if(fd == -1)    {perror("mkstemps");    return -1;}
    FILE *fp = fdopen(fd,"w");
    assert(fp>=0);
    fprintf(fp,"%s\n",function_body);
    fclose(fp);
    
    // compile the function and check for syntax errors
    pid_t pid = fork();
    assert(pid>=0);
    if(pid == 0){
        const char *args[] = {"gcc", "-fsyntax-only", tmp_file, NULL};
        execvp("gcc", (char *const *)args);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDERR_FILENO);  // close the original stderr
            close(devnull);
        }
        perror("execvp");
        exit(127);
    }
    // father pid
    int status;
    if(waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        exit(1);
    }
    unlink(tmp_file);
    if (WIFEXITED(status))   return WEXITSTATUS(status);
    return -1;
}

int main(int argc, char *argv[]) {
    static char line[4096];
    static char *type = "int";

    while (1) {
        printf("crepl> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))  break;

        int res = write_function_to_file(line);
        res == 0 ? printf("Syntax OK\n") : printf("Syntax Error\n");

        // if (memcmp(type,line,strlen(type)) == 0){
        //     printf("Define a new function.\n");
        //     // Define a new function.
            
        // }
        // else{
        //     printf("Define a new variable.\n");
            
        // }

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}
