/**
* NAME:
* shell - a simplified version of the standard command line shell
*
* DESCRIPTION:
* The shell program is a small and simplified version of the standard command line functionality.
* It supports background and foreground processes, by defualt a process is a foreground process, 
* if you want it to run in the background instead simply add an ampersand at any point in your input 
* (separated by spaces from other input).
* Does not support pipes and links.
* Type exit to exit.
*
* EXAMPLES:
* > ls
* > myProgram
* > myProgram &
* > cd myDir/myOtherDir
* > cd ..
* > exit
*
* AUTHOR:
* Johan Storby, Yuri Stange, 2014
*
* SEE ALSO:
* ls(0), cd(1)
*
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>

#define MAXLEN 70

/* Antal aktiva bakgrundsprocesser */
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
    /* Reserves space for the struct and creates a pointer to it. */
    struct timeval tv;
    struct timeval * tvpek = &tv;

    /* Save the current time into tv */
    gettimeofday(&tv, NULL);

    /* Returns the amount of milliseconds since the epoch. Casts to long to ensure we have no overflow. */
    return (long)(((long)(tvpek->tv_sec)*1000L) + ((long)(tvpek->tv_usec)/1000L));
}

/**
 * Parses the arguments given. Argv needs to be null-terminated
 * Terminates the copy with NULL
 * Returns 0 if it worked
 * Returns 1 if something went wrong (like no params)
 */
int parseArgs(char * argv, char * copy[]) {
    /* Splits the string on spaces */
    char * token = strtok(argv, " ");

    /* Did we get any params at all? */
    if(token==NULL) return 1;

    /* Loop through the string and split on all spaces */
    int i=0;
    while(token!=NULL) {
        copy[i]=token;

        /* Splits the string on spaces */
        token = strtok(NULL, " ");
        i++;

        /* If we reach 7 then argv wasn't terminated */
        if(i>8) err("parseArgs: argv lacks null-termination");
    }

    /* Make sure the last element is NULL to ensure the array is correctly terminated */
    copy[i] = NULL;

    /* It worked! */
    return 0;
}

/**
 * Runs the given commands in the foreground
 */
void openInForeground(char * input_cmd[]) {

    /* Fork to child process */

    /*Foreground process running, so make sure the main process doesn't get interrupted by Ctrl+C */
    signal(SIGINT, SIG_IGN);

    /* Record the start time.*/
    long starttime = getTime();

    /* Fork */
    int child = fork();


    /* Error?  */
    if (child == -1)
        err("Failed creating child\n");

    /* Child or main */
    if (child == 0) {
        /* Child! */

        /* The child should respond to Ctrl+C to close it. */
        signal(SIGINT, SIG_DFL);

        /* Execute the given command */
        if (execvp(input_cmd[0], input_cmd) == -1) {
            err("Couldn't run the given inputs");
        }
    } else {
        /* Main! */

        /*Tell the user that the foreground process has started. */
        printf("Foreground process started with pid: %d\n", child);

        /* Wait for the child process to finish */
        int status;
        int waiting = waitpid(child, &status, 0);

        if(waiting == -1) {
            err("Failed waiting for Foreground");
        }


        /* Child has terminated, output info about it. */
        printf("Foreground process terminated, pid: %d. Status: %d. Time: %ldms\n", child, status, getTime()-starttime);

        /* We can now be killed by Ctrl-C again. */
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
        /* We are the child, run the background process */
        if (execvp(input_cmd[0], input_cmd) == -1) {
            err("Couldn't run the given inputs");
        }
    } else {
        /* We are main, print some info, and update how many background processes we have running */
        printf("Background process started with pid: %d\n", child);
        activeBackground++;
    }

}

/**
 * Checks the status of the children running in the background
 */
void checkBackgroundStatus(int options) {
    int i;

    /* Loop through all children running in the background */
    for(i = 0; i < activeBackground; i++) {
        int status;

        /* Check for any new statuses */
        int id = waitpid(-1, &status, options );

        if(id == -1) {
            /* Error? */
            err("Failed while waiting for background children");
        } else if(id == 0) {
            /* Nothing has happened. Continue*/
            continue;
        }

        /* Has the child stopped running? */
        if(!WIFCONTINUED(status)) {
            /* Print some info. */
            fprintf(stdout, "Background process id:%d terminated with status %d.\n", id, status);

            /* One less background process runing. */
            activeBackground--;
        }
    }
}

/**
 * Kills all child processes
 * Avoids zombies by waiting for them
 */
void killChildren() {

    /* Ignore SIGTERM so we don't kill ourselves */
    signal(SIGTERM, SIG_IGN);

    /* Send a kill signal to all children */
    int res = kill(0, SIGTERM);

    if(res == -1) {
        /* Error? */
        err("Failed to kill our children");
    }

    /* We can now be killed by Ctrl-C again */
    signal(SIGTERM, SIG_DFL);

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
 * Kills all children then procedes to exit
 */
void interruptHandler() {
    killChildren();
    exit(0);
}

int main (int argc, char * argv[], char * envp[]) {

    char input[MAXLEN]; /* The input string */
    char * input_cmd[7]; /* The tokenized input string */
    char cur_dir[1024]; /* Current working directory */

    /* We start with no background children */
    activeBackground = 0;


    /* Setup signal handlers */
    if (signal(SIGINT, interruptHandler) == SIG_ERR) {
        err("Failed to setup interrupt handler");
    }

    if (signal(SIGTERM, terminationHandler) == SIG_ERR) {
        err("Failed to setup termination handler");
    }

    /* Run the main loop infinitely until terminated */
    while (1) {

        /* Check the status on our children */
        checkBackgroundStatus(WNOHANG);

        /* Get working directory */
	getcwd(cur_dir, sizeof(cur_dir));

        /* Output the working directory and a > */
        printf("%s> ", cur_dir);

        /* Read input. This is a blocking function so our loop will wait here for input. */
        fgets(input, MAXLEN, stdin);
        /* Make sure the input is terminated with a NULL byte */
        input[strlen(input)-1] = '\0';

        /* Did the user type exit? If so, then break out of the infinite loop. */
        if (strcmp(input, "exit") == 0) {
            break;
        }

        /* Parse the inputs */
        if(parseArgs(input, input_cmd)!=0) continue;

        /* cd? */
        if (strcmp(input_cmd[0], "cd") == 0) {
             /* Was a directory supplied? */
             if (input_cmd[1] == NULL || strcmp(input_cmd[1], "") == 0) {
                 /* No. Set to home directory instead. */
                 printf("Could not change directory. Setting to Home directory instead.\n");
                 chdir(getenv("HOME"));
                 continue;
	     }

             /* Yes. Check if it exists and we have access to it. */
             struct stat sb;
             if (stat(input_cmd[1], &sb) == 0 && S_ISDIR(sb.st_mode)) {
                   /* It is ok. Set it to our working dir. */
                   if (chdir(input_cmd[1]) == -1) {

                       /* Something went wrong anyway, set to HOME. */
                       printf("Could not change directory. Setting to Home directory instead.\n");
                       chdir(getenv("HOME"));
                   }

             } else {
                /* It doesn't exist or we don't have access, set to HOME instead. */
                printf("Could not change directory. Setting to Home directory instead.\n");
                chdir(getenv("HOME"));
             }
             continue;
        }

        /* Check if we should run in the background or the foreground */
        int i = 0;
        bool foreground = true; /* By default, we run in the foreground */

        /* Loop through input_cmd */
        while (input_cmd[i]!=NULL) {
            /* Is this token an ampersand? */
            if (strcmp(input_cmd[i],"&")==0) { 
               /* That means we should run in the background instead. */
               foreground = false;
               
               /* Loop through the rest of the array and move each element to one index lower, so we don't send the ampersand to execvp(). */
               while (input_cmd[i]!=NULL) {
                  input_cmd[i] = input_cmd[i+1];
                  i++;
               }
               break; /* There will only be one ampersand, so we can break out here. */
            }
            i++;
        }

        /* Call the apropriate function to run eitehr in the foreground or background. */
        if(foreground) {
            openInForeground(input_cmd);
        } else {
            openInBackground(input_cmd);
        }

    }

    /* We have broken out of the infinite loop! */

    /* Kill eventual background children */
    killChildren();

    /* Exit */
    return 0;
}

