#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <dlfcn.h>

#define MAX_LINE 1<<12

static char *function_file = "/tmp/function_file.c";

int check_function_syntax(const char *function_body)
{
    // open the source file
    FILE *src_fp = fopen(function_file,"a+");
    if(!src_fp) {perror("fopen");  return -1;}

    // check the line's syntax
    char tmp_file[] = "/tmp/tmp_func_XXXXXX.c";
    int fd = mkstemps(tmp_file,2);
    if(fd == -1)    {perror("mkstemps");    return -1;}
    FILE *fp = fdopen(fd,"w");
    if(!fd) {perror("fdopen");  return -1;}
    char buf[1024];
    int n;
    while ((n = fread(buf, 1, sizeof(buf), src_fp)) > 0) {
        if (fwrite(buf, 1, n, fp) != n) {
            perror("fwrite");
            fclose(src_fp);
            fclose(fp);
            unlink(tmp_file);
            return -1;
        }
    }

    fprintf(fp,"%s\n",function_body);
    // fflush(fp);
    fclose(fp);
    
    // compile the function and check for syntax errors
    pid_t pid = fork();
    if(pid == 0){
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull != -1) {
            dup2(devnull, STDERR_FILENO);  // close the original stderr
            close(devnull);
        }
        const char *args[] = {"gcc", "-fsyntax-only", "-Werror=implicit-function-declaration", tmp_file, NULL};
        execvp("gcc", (char *const *)args);
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

void get_function_name(char *new_line,const char *function)
{
    int idx=0;
    for(int i=4;function[i]!=' ';i++)   new_line[idx++] = function[i];
    new_line[idx] = '\0';
    printf("Function name: %s\n", new_line);
    return;
}
int call_the_function(const char *function_body)
{
    pid_t pid = fork();
    // child pid
    if(pid == 0){
        const char *argv[] = {"gcc", "-fPIC", "-shared", "-o", "/tmp/function_file.so", "/tmp/function_file.c", NULL};
        execvp("gcc", (char *const *)argv);
        perror("execvp failed");
        exit(127);
    }

    // father pid
    int status;
    waitpid(pid, &status, 0);
    // call the functions
    char *error;
    void *handle = dlopen("/tmp/function_file.so", RTLD_LAZY);
    if (!handle) {fprintf(stderr, "%s\n", dlerror()); exit(1);}
    dlerror();
    char new_line[MAX_LINE];
    get_function_name(function_body, new_line);
    int (*func)() = dlsym(handle, new_line);
    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        dlclose(handle);
        return -1;
    }
    int res = func();
    printf("Result: %d\n", res);
    dlclose(handle);
    unlink("/tmp/function_file.so");
    return 0;
}

int main(int argc, char *argv[]) {
    static char line[MAX_LINE>>1];
    static char tmp_line[MAX_LINE];
    static char *type = "int";
    static int idx = 0;

    while (1) {
        printf("crepl> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))  
        {
            unlink(function_file);
            break;
        }
        line[strlen(line)-1] = '\x00';

        // clarify the input_class
        int input_class = 1;
        if(memcmp(type,line,strlen(type)) == 0) input_class = 0;
        else   {
            // change the expression to a function
            // use a wrapper function
            sprintf(tmp_line,"int __expr_wrapper_%d() {return %s;}",idx++, line);
            memcpy(line,tmp_line,MAX_LINE>>1);
        }
        // check the syntax of the function
        int res = check_function_syntax(line);
        if(res != 0) {
            printf("Syntax Error\n");
            continue;
        }
        else{
            FILE *fp = fopen(function_file,"a+");
            if(!fp) {perror("fopen");  return -1;}
            fprintf(fp,"%s\n",line);
            fflush(fp);
            fclose(fp);
        }

        // register the function
        if(!input_class)    printf("[Added: ] %s\n",line);
        else    call_the_function(line);

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}
