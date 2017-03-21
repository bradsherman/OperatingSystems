// Brad Sherman
// Operating Systems
// Project2 - myshell extra credit

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// tokenize a string input
int tokenize(char * input, char * words[101]) {
    char * tok = strtok(input, " \t\n");
    int nwords = 0;
    words[nwords++] = tok;
    while((tok = strtok(0," \t\n")) && nwords < 100){
        words[nwords++] = tok;
    }
    words[nwords] = 0;
    return nwords;
}

void remove_element(char ** array, int index, int length) {
    int i;
    for(i = index; i < length; i++) {
        array[i] = array[i+1];
    }
}

// wrapper function to fork and exec
pid_t fork_exec(char ** words, int nwords) {
    pid_t pid = fork();
    if(pid < 0) {
        printf("Unable to fork: %s\n", strerror(errno));
    } else if(pid == 0) {
        // run command starting at words[1]
        char * myargs[nwords];
        myargs[nwords-1] = NULL;
        int i;
        for(i = 0; i < nwords - 1; i++) {
            myargs[i] = words[i+1];
        }

        for(i = 0; i < nwords - 1; i++) {
            if(strcmp(myargs[i], "<") == 0) {
                // redirect input
                char * file = myargs[i+1];
                int fd = open(file, O_RDONLY);
                if(fd < 0) {
                    fprintf(stderr, "Couldn't open %s: %s\n", file, strerror(errno));
                    exit(1);
                }
                int r = dup2(fd, 0);
                if(r < 0) {
                    fprintf(stderr, "Couldn't switch file descriptors: %s\n", strerror(errno));
                    exit(1);
                }
                // don't pass I/O redirection stuff as args to command
                remove_element(myargs, i, nwords);
                nwords--;
                remove_element(myargs, i, nwords);
                nwords--;
            }

            if(i >= nwords-1) continue; // prevent accessing last element (NULL)

            if(strcmp(myargs[i], ">") == 0) {
                // redirect output
                char * file = myargs[i+1];
                int fd = open(file, O_CREAT|O_WRONLY|O_TRUNC, 0644);
                if(fd < 0) {
                    fprintf(stderr, "Couldn't open %s: %s\n", file, strerror(errno));
                    exit(1);
                }
                int r = dup2(fd, 1);
                if(r < 0) {
                    fprintf(stderr, "Couldn't switch file descriptors: %s\n", strerror(errno));
                    exit(1);
                }
                // don't pass I/O redirection stuff as args to command
                remove_element(myargs, i, nwords);
                nwords--;
                remove_element(myargs, i, nwords);
                nwords--;
            }
        }
        execvp(myargs[0], myargs);
        printf("myshell: failed to execute %s: %s\n", myargs[0], strerror(errno));
    } else {
        printf("myshell: process %d started\n", pid);
    }
    return pid;

}

// wrapper function to wait and print return status
void wait_wrapper(int pid) {
    int status, r;
    pid_t w = waitpid(pid, &status, 0);
    if(w == -1) {
        // no processes left
        printf("myshell: no processes left - %s\n", strerror(errno));
        return;
    }
    if(WIFEXITED(status)) {
        // normal exit
        r = WEXITSTATUS(status); // return value
        printf("myshell: process %d exited normally with exit status %d\n", w, r);
    }
    if(WIFSIGNALED(status)) {
        // abnormal exit
        r = WTERMSIG(status); // signal number
        printf("myshell: process %d exited abnormally with status %d: %s\n", w, r, strsignal(r));
    }
    if(WIFSTOPPED(status)) {
        r = WSTOPSIG(status);
        printf("myshell: process %d stopped with signal %d: %s\n", w, r, strsignal(r));
    }
}

int is_valid_integer(char * a) { while(a && *a >='0' && *a <= '9') ++a; return !!*a; }

// wrapper function to send a signal
void send_sig(char * pid, int sig) {
    if(is_valid_integer(pid) == 1) {
        printf("myshell: invalid pid entered\n");
        return;
    }
    int i_pid = atoi(pid);
    int r = kill(i_pid, sig);
    if(r < 0) {
        printf("myshell: unable to send signal %d: %s to process %d: %s\n", sig, strsignal(sig), i_pid, strerror(errno));
    } else {
        printf("myshell: signal %d: %s sent to process %d\n", sig, strsignal(sig), i_pid);
    }
}

int main(void)
{
    char input[4097];
    printf("myshell> ");
    fflush(stdout);
    while(fgets(input, 4097, stdin)) {

        char * words[101];
        int nwords = tokenize(input, words);
        // check for empty input
        if(words[0] == NULL) {
            printf("myshell> ");
            fflush(stdout);
            continue;
        }

        if(strcmp(words[0], "exit") == 0 || strcmp(words[0], "quit") == 0) {
            exit(0);
        } else if(strcmp(words[0], "start") == 0) {
            if(nwords > 1) fork_exec(words, nwords);
            else printf("usage: start <command> <args>\n");

        } else if(strcmp(words[0], "wait") == 0) {
            wait_wrapper(-1);

        } else if(strcmp(words[0], "run") == 0) {
            if(nwords > 1) {
                pid_t pid = fork_exec(words, nwords);
                wait_wrapper(pid);
            } else printf("usage: run <command> <args>\n");

        } else if(strcmp(words[0], "stop") == 0) {
            // send SIGSTOP (17)
            if(nwords == 2) send_sig(words[1], 19);
            else printf("usage: stop <pid>\n");

        } else if(strcmp(words[0], "kill") == 0) {
            // send SIGKILL (9)
            if(nwords == 2) send_sig(words[1], 9);
            else printf("usage: kill <pid>\n");

        } else if(strcmp(words[0], "continue") == 0) {
            // send SIGCONT (25)
            if(nwords == 2) send_sig(words[1], 18);
            else printf("usage: continue <pid>\n");

        } else {
            printf("Unrecognized command: %s\n", words[0]);
        }

        printf("myshell> ");
        fflush(stdout);
    }
    return 0;
}
