#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define MAXLEN 70

/*Print out the possible error*/
void err(char* msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

int main (int argc, char * argv[], char * envp[]) {

	int child, i;
	char input[MAXLEN];
	char input_cmd[MAXLEN];

	while (1) {	

		printf("small-shell> ");
		fgets(input, MAXLEN, stdin);
		input[strlen(input)-1] = '\0';

		if (strcmp(input, "exit") == 0) {
			break;
		}

		/*input_command = strtok(input, " ");*/
		
		for (i = 0; i < strlen(input); i++) {
			if (input[i] == ' ') {
				input_cmd[i] = '\0';
				break;
			} else {
				input_cmd[i] = input[i];
			}
		}

		child = fork();

		if (child == -1)
			err("Failed creating child\n");

		if (child == 0) {
			if (execlp(input_cmd, input, NULL) == -1) {
				err("Error");
			}
		}
		waitpid(child, NULL, 0);

	}


	waitpid(child, NULL, 0);

	return 0;
}
