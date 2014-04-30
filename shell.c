#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define MAXLEN 70

/*Print out the possible error*/
void err(char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main (int argc, char * argv[], char * envp[])
{

    int child, i;
    char input[MAXLEN];
    char * input_cmd[7];

    while (1)
    {

        /* Utskrift */
        printf("small-shell> ");

        /* Ta in en sträng */
        fgets(input, MAXLEN, stdin);
        input[strlen(input)-1] = '\0';

        /* Exit ?*/
        if (strcmp(input, "exit") == 0)
        {
            break;
        }

        char * token = strtok(input, " ");
        i=0;
        while(token!=NULL) {
            input_cmd[i]=token;
            token = strtok(NULL, " ");
            i++;
        }
        input_cmd[i] = NULL;

        child = fork();

        if (child == -1)
            err("Failed creating child\n");

        if (child == 0)
        {

            if (execvp(input_cmd[0], input_cmd) == -1)
            {
                err("Error");
            }
        }
        waitpid(child, NULL, 0);

    }


    waitpid(child, NULL, 0);

    return 0;
}
