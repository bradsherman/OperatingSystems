// Brad Sherman
// Operating Systems
// Project2 - myshell

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>

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

// wrapper function to fork and exec
pid_t fork_exec(char ** words, int nwords) {
    pid_t pid = fork();
    if(pid < 0) {
        printf("Unable to fork: %s\n", strerror(errno));
    } else if(pid == 0) {
        // run command starting at words[1] and insert NULL at the end
        char * myargs[nwords];
        myargs[nwords-1] = NULL;
        int i;
        for(i = 0; i < nwords - 1; i++) {
            myargs[i] = words[i+1];
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
    pid_t w = waitpid(pid, &status, WUNTRACED);
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
            // make sure we have a command to run
            if(nwords > 1) fork_exec(words, nwords);
            else printf("usage: start <command> <args>\n");

        } else if(strcmp(words[0], "wait") == 0) {
            wait_wrapper(-1);

        } else if(strcmp(words[0], "run") == 0) {
            // make sure we have a command to run
            if(nwords > 1) {
                pid_t pid = fork_exec(words, nwords);
                wait_wrapper(pid);
            } else printf("usage: run <command> <args>\n");

        } else if(strcmp(words[0], "stop") == 0) {
            // send SIGSTOP (19)
            // make sure we are given a pid
            if(nwords == 2) send_sig(words[1], 19);
            else printf("usage: stop <pid>\n");

        } else if(strcmp(words[0], "kill") == 0) {
            // send SIGKILL (9)
            // make sure we are given a pid
            if(nwords == 2) send_sig(words[1], 9);
            else printf("usage: kill <pid>\n");

        } else if(strcmp(words[0], "continue") == 0) {
            // send SIGCONT (18)
            // make sure we are given a pid
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
