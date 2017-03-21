// Brad Sherman
// Operating Systems
// Project 3
// Mandel Movie

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>

// relevant globals
char * HEIGHT = "1000";
char * WIDTH = "1000";
char * X_COORD = "-0.5429";
char * Y_COORD = ".599587";
char * M = "2000";
char * MIN_SCALE = ".000005";
char * MAX_SCALE = "2";
double ITERATIONS = 50.0;
int MAX_PROCS = 10;

void usage(char * command) {
    printf("usage: %s <max_procs>\n", command);
    exit(1);
}

void setup_pics_dir() {
    struct stat st = {0};
    if(stat("./pics",&st) == -1) {
        mkdir("./pics", 0777);
    }
}

int main (int argc, char *argv[])
{
    if(argc > 2) {
        usage(argv[0]);
    } else if(argc == 2){
        MAX_PROCS = atoi(argv[1]);
    }

    // make ./pics/ if it doesn't exist
    setup_pics_dir();

    int n_procs = 0;
    int n_pics = 0;
    int n_pics_finished = 0;
    double min_scale_db = strtod(MIN_SCALE,NULL);
    double max_scale_db = strtod(MAX_SCALE,NULL);
    double scale_db = atoi(MAX_SCALE);
    while(n_pics_finished < ITERATIONS) {

        if(n_procs < MAX_PROCS && n_pics < ITERATIONS) {
            char scale[20];
            sprintf(scale, "%lf", scale_db);
            char file_name[20];
            sprintf(file_name, "pics/mandel%d.bmp", n_pics);
            pid_t pid = fork();
            if(pid < 0) {
                printf("Unable to fork: %s\n", strerror(errno));
            } else if(pid == 0) {
                // generate image
                char * myargs[16];
                myargs[0] = "./mandel";
                myargs[1] = "-x";
                myargs[2] = X_COORD;
                myargs[3] = "-y";
                myargs[4] = Y_COORD;
                myargs[5] = "-s";
                myargs[6] = scale;
                myargs[7] = "-m";
                myargs[8] = M;
                myargs[9] = "-W";
                myargs[10] = WIDTH;
                myargs[11] = "-H";
                myargs[12] = HEIGHT;
                myargs[13] = "-o";
                myargs[14] = file_name;
                myargs[15] = NULL;

                execvp(myargs[0], myargs);
                printf("mandelmovie: failed to execute: %s\n", strerror(errno));
            } else {
                // parent
                n_procs++;
            }
            n_pics++;
            scale_db = scale_db * exp(log(min_scale_db/max_scale_db)/ITERATIONS);
        } else {
            // wait for a processes to finish
            int status;
            if(wait(&status) > 0) {
                n_procs--;
                n_pics_finished++;
                if(WIFEXITED(status)) {
                    // exited normally
                }
                if(WIFSIGNALED(status)) {
                    // exited abnormally
                }
            }
        }
    }
    return 0;
}
