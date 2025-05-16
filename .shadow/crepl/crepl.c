#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    static char line[4096];
    static char *type = "int";

    while (1) {
        printf("crepl> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))  break;

        if (memcmp(type,line,strlen(type)) == 0){
            printf("Define a new function.\n");

        }
        else{
            printf("Define a new variable.\n");
            
        }

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}
