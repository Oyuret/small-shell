#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define MAXLEN 70

/**
 * Print out the possible error
 */
void err(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Parses the arguments given. Argv needs to be null-terminated
 * Terminates the copy with NULL
 * Returns 0 if it worked
 */
int parseArgs(char * argv, char * copy[]) {
    char * token = strtok(argv, " ");

    int i=0;
    while(token!=NULL) {
        copy[i]=token;
        token = strtok(NULL, " ");
        i++;

        /* If we reach 7 then argv wasn't terminated*/
        if(i>8) err("parseArgs: argv lacks null-termination");
    }

    copy[i] = NULL;

    /* It worked! */
    return 0;
}

void openInForeground(char * input_cmd[]) {

    /* Fork to child process */
    signal(SIGINT, SIG_IGN);
    int child = fork();

    /* Child or main */
    if (child == -1)
        err("Failed creating child\n");

    if (child == 0) {
        signal(SIGINT, NULL);
        if (execvp(input_cmd[0], input_cmd) == -1) {
            err("Couldn't run the given inputs");
        }
    } else {
        waitpid(child, NULL, 0);
        signal(SIGINT, NULL);
    }

}

int main (int argc, char * argv[], char * envp[]) {

    char input[MAXLEN];
    char * input_cmd[7];

    while (1) {

        /* Utskrift */
        printf("small-shell> ");

        /* Ta in en sträng */
        fgets(input, MAXLEN, stdin);
        input[strlen(input)-1] = '\0';

        /* Exit ?*/
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* Parse the inputs */
        parseArgs(input, input_cmd);

        openInForeground(input_cmd);



    }

    return 0;
}
