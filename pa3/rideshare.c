#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    // Get arguments from console
    int supportersCountA = atoi(argv[1]);
    int supportersCountB = atoi(argv[2]);

    // Check if arguments are valid, if not return from main
    if (supportersCountA % 2 != 0 || supportersCountB % 2 != 0 || (supportersCountA + supportersCountB) % 4 != 0)
    {
        printf("The main terminates\n");
        return 0;
    }

    // If there are no errors regarding inputs, start the program
    printf("The main terminates\n");
    return 0;
}