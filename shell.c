#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>


/*Print out the possible error*/
void err(char* msg){
	perror(msg);
	exit(EXIT_FAILURE);
}

int main (int argc, char * argv[], char * envp[]) {

	int child;

	child = fork();

	if (child == -1)
		err("Failed creating child\n");

	if (child == 0) {
		printf("In child process\n");
		printf("Test 2\n");
		printf("Test 3\n");
	} else {
		printf("In parent process\n");
		printf("Test 4\n");
		printf("Test 5\n");
		printf("\n");
		execlp("ls", "ls", NULL);
	}


	waitpid(child, NULL, 0);

	return 0;
}
