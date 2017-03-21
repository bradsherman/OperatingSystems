// Brad Sherman
// Operating Systems
// Project 1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// help user use program correctly
void usage(int args) {
    char * message = "Not enough arguments";
    if(args > 3) {
        message = "Too many arguments";
    }
    fprintf(stderr, "%s: %s\n", "copyit", message);
    fprintf(stderr, "usage: ./copyit <sourcefile> <targetfile>\n");
}

// alarm handler function
void display_message() {
    char message[] = "copyit: still copying...\n";
    write(STDOUT_FILENO, message, sizeof(message)-1);
    alarm(1);
}

int main(int argc, char *argv[]) {

    // Check for correct usage
    if(argc != 3) {
        usage(argc);
        exit(1);
    }

    // set up alarm
    signal(SIGALRM, display_message);
    alarm(1);

    char * src = argv[1];
    char * target = argv[2];

    // setup source file descriptor
    int sfd = open(src, O_RDONLY,0);
    if(sfd < 0) {
        fprintf(stderr, "%s: couldn't open %s: %s\n",argv[0], src, strerror(errno));
        exit(1);
    }

    // setup target file descriptor
    // if file already exists truncate
    int tfd = creat(target, 0666);
    if(tfd < 0) {
        fprintf(stderr, "%s: couldn't create %s: %s\n",argv[0], target, strerror(errno));
        exit(1);
    }

    size_t count = 4096; // amount we read/write at one time
    unsigned int total_bytes = 0; // total # of bytes copied
    char buf[4096];      // buffer that stores data
    int loop = 1;        // loop variable
    while(loop) {

        // read first
        int bytes_read = read(sfd, buf, count);
        while(bytes_read < 0 && errno==EINTR) {
            // read interrupted, try again
            // each time we get interrupted
            bytes_read = read(sfd, buf, count);
            printf("read interrupted\n");
        }
        if(bytes_read < 0) {
            // exit with error
            fprintf(stderr, "%s: couldn't read %s: %s\n",argv[0], src, strerror(errno));
            exit(1);
        } else if(bytes_read == 0) {
            // no data left
            loop = 0;
        }

        // write data
        int bytes_written = write(tfd, buf, bytes_read);
        while(bytes_written < 0 && errno==EINTR) {
            // write interrupted, try again
            // each time we get interrupted
            printf("write interrupted\n");
            bytes_written = write(tfd, buf, bytes_read);
        }
        if(bytes_written < 0 || bytes_written < bytes_read) {
            // exit with error
            fprintf(stderr, "%s: couldn't write to %s: %s\n",argv[0], target, strerror(errno));
            exit(1);
        }
        total_bytes += bytes_written;
    }

    // close source file descriptor
    int s_close = close(sfd);
    if(s_close < 0) {
        fprintf(stderr, "%s: couldn't close %s: %s\n",argv[0], src, strerror(errno));
        exit(1);
    }
    // close target file descriptor
    int t_close = close(tfd);
    if(t_close < 0) {
        fprintf(stderr, "%s: couldn't close %s: %s\n",argv[0], target, strerror(errno));
        exit(1);
    }

    // print message with success
    fprintf(stdout, "%s: Copied %u bytes from file %s to %s\n", argv[0], total_bytes, src, target);

    return 0;
}
