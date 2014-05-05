#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

#define MAXLEN 70

int activeBackground;

/**
 * Print out the possible error
 */
void err(char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Returns the current time in milliseconds
 */
long getTime() {
    /*TODO: FIXA*/
    return 0;
}

/**
 * Parses the arguments given. Argv needs to be null-terminated
 * Terminates the copy with NULL
 * Returns 0 if it worked
 * Returns 1 if something went wrong (like no params)
 */
int parseArgs(char * argv, char * copy[]) {
    char * token = strtok(argv, " ");

    /* Did we get any params at all?*/
    if(token==NULL) return 1;

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

/**
 * Runs the given commands in the foreground
 */
void openInForeground(char * input_cmd[]) {

    /* Fork to child process */
    signal(SIGINT, SIG_IGN);
    int child = fork();

    /* Child or main */
    if (child == -1)
        err("Failed creating child\n");

    if (child == 0) {
        signal(SIGINT, SIG_DFL);
        if (execvp(input_cmd[0], input_cmd) == -1) {
            err("Couldn't run the given inputs");
        }
    } else {
        printf("Foreground process started with pid: %d\n", child);

        int status;
        int waiting = waitpid(child, &status, 0);

        if(waiting == -1) {
            err("Failed waiting for Foreground");
        }



        /*Output tid-information*/
        printf("Foreground process terminated, pid: %d. Status: %d.\n", child, status);
        signal(SIGINT, SIG_DFL);
    }

}

/**
 * Opens the given command in the background
 */
void openInBackground(char * input_cmd[]) {

    /* Fork to child process */
    int child = fork();

    /* Child or main */
    if (child == -1)
        err("Failed creating child\n");

    if (child == 0) {
        if (execvp(input_cmd[0], input_cmd) == -1) {
            err("Couldn't run the given inputs");
        }
    } else {
        printf("Background process started with pid: %d\n", child);
        activeBackground++;
    }

}

/**
 * Checks the status of the children
 */
void checkBackgroundStatus(int options) {

    /* Check if any child have terminated */
    int i;
    int exitCount = 0;

    for(i = 0; i < activeBackground; i++) {
        int status;
        int id = waitpid(-1, &status, options );

        if(id == -1) {
            err("Failed while waiting for background children");
        } else if(id == 0) {
            /* Nothing has happened. Continue*/
            continue;
        }

        if(!WIFCONTINUED(status)) {
            exitCount++;
            fprintf(stdout, "Background process id:%d terminated with status %d\n", id, status);
        }
    }
    /* Update the number of running background processes */
    activeBackground -= exitCount;
}

/**
 * Kills all child processes
 * Avoids zombies by waiting for them
 */
void killChildren() {

    /* Ignore SIGTERM so we don't kill ourselves */
    sigset(SIGTERM, SIG_IGN);

    /* Commit prolicide */
    int res = kill(0, SIGTERM);
    if(res == -1) {
        err("Failed to kill our children");
    }

    sigset(SIGTERM, SIG_DFL);

    /* Watch our children die */
    checkBackgroundStatus(0);
}


/**
 * Handles SIGTERM.
 * Kills all children then procedes to exit
 */
void terminationHandler() {
    killChildren();
    exit(0);
}

/**
 * Handles SIGINT
 */
void interruptHandler() {
    killChildren();
    exit(0);
}

int main (int argc, char * argv[], char * envp[]) {

    char input[MAXLEN];
    char * input_cmd[7];

    /* We start with no background children */
    activeBackground = 0;

    /* Setup signal handlers*/
    if(sigset(SIGINT, interruptHandler)==-1) {
        err("Failed to setup interrupt handler");
    }

    if(sigset(SIGTERM, terminationHandler)==-1) {
        err("Failed to setup termination handler");
    }

    while (1) {

        /* Check the status on our children */
        checkBackgroundStatus(WNOHANG);

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
        if(parseArgs(input, input_cmd)!=0) continue;


        /* Check if we should run in the background or not */
        int i=0;
        bool foreground = true;
        while(input_cmd[i]!=NULL) {
            if(strcmp(input_cmd[i],"&")==0) foreground=false;
            i++;
        }

        if(foreground) {
            openInForeground(input_cmd);
        } else {
            openInBackground(input_cmd);
        }

    }

    /* Kill eventual background child */
    killChildren();
    return 0;
}
