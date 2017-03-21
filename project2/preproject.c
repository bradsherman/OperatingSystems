// Brad Sherman
// Operating Systems
// Preproject 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
	// Ensure correct usage of the program
	if(argc < 2) {
		printf("Please enter a file to be copied\n");
		exit(1);
	} else if(argc > 2) {
		printf("Please enter only one filename\n");
		exit(1);
	}

	// string constants
	char * dst = "CloneFile";
	/* char * cpLoc = "cp"; */

	pid_t pid = fork();
	if (pid < 0) {
		printf("Unable to fork: %s\n", strerror(errno));
	} else if (pid == 0) {
		// child
		char * myargs[4];
		myargs[0] = strdup("cp");
		myargs[1] = strdup(argv[1]);
		myargs[2] = strdup(dst);
		myargs[3] = NULL;
		execvp(myargs[0], myargs);
		/* execl(cpLoc, cpLoc, argv[1], dst, NULL); */
	} else {
		// parent
		int status, r;
		// wait for copy to finish and execute md5sum if successful
		pid_t w = wait(&status);
		if(WIFEXITED(status)) {
	    		// normal exit
    			r = WEXITSTATUS(status); // return value
    			printf("Copy successful: %s. Executing md5sum now.\n", strsignal(r));
    			execl("/usr/bin/md5sum", "md5sum", dst, argv[1], NULL);
			return 1;
		}
		if(WIFSIGNALED(status)) {
	    		// abnormal exit
    			r = WTERMSIG(status); // signal number
    			printf("Copy failed: %s\n", strsignal(r));
		}
	}
	return 0;
}
