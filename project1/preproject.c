// Brad Sherman
// Operating Systems
// Pre Project 1

#include <stdio.h>
#include <signal.h>

char bKeepLooking = 1;

void sighandler(int x) {
    bKeepLooking = 0;
}

int main( int argc, char * argv[] )
{
    signal(SIGINT, sighandler);

    while(bKeepLooking)
    {
    }
    printf("Exited successfully\n");
    return 0;
}
