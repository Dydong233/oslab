#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

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
    assert(pid>=0);
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
            // fflush(fp);
            fclose(fp);
        }

        // register the function
        if(!input_class)    printf("[Added : ] %s\n",line);
        else{
            
        }


        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}
