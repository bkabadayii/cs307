#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char *mainCommand = "man touch | grep -A 2 \"\\-r\" > output.txt";
    printf("I’m SHELL process, with PID: %d - Main command is: %s \n", getpid(), mainCommand);
    int shell_rc = fork();
    if (shell_rc < 0)
    {
        printf("Fork Failed!\n");
        exit(1);
    }
    else if (shell_rc == 0)
    {
        // Child process that will run the MAN command
        char *manCommand = "man touch";
        printf("I’m MAN process, with PID: %d - My command is: %s\n", getpid(), manCommand);

        // Prepare pipe
        int fd[2];
        int piped = pipe(fd);
        if (piped < 0)
        {
            printf("Piping failed!\n");
            exit(1);
        }

        // Create grand-child process
        int man_rc = fork();

        if (man_rc < 0)
        {
            printf("Grand child fork failed!\n");
            exit(1);
        }
        // Parent process (man) continues
        else if (man_rc > 0)
        {
            close(1); // Close STDOUT
            dup(fd[1]);
            close(fd[0]);
            // Prepare MAN execution
            char *manArgs[3];
            manArgs[0] = "man";
            manArgs[1] = "touch";
            manArgs[2] = NULL;
            execvp(manArgs[0], manArgs);
            // wait(NULL); // wait for the child process
        }
        // Grand-child process (grep)
        else if (man_rc == 0)
        {
            char *grepCommand = "grep -A 2 \"\\-r\" > output.txt";
            printf("I’m GREP process, with PID: %d - My command is: %s\n", getpid(), grepCommand);

            close(0); // Close STDIN
            dup(fd[0]);
            close(fd[1]);

            // redirect stdout to the output.txt file
            int output_fd = creat("output.txt");
            if (output_fd < -1)
            {
                printf("output.txt direction failed");
                exit(1);
            }
            dup2(output_fd, 1);
            close(output_fd);
            // Prepare GREP execution
            char *grepArgs[5];
            grepArgs[0] = "grep";
            grepArgs[1] = "-A";
            grepArgs[2] = "2";
            grepArgs[3] = "\\-r";
            grepArgs[4] = NULL;
            execvp(grepArgs[0], grepArgs);
        }

        close(fd[0]);
        close(fd[1]);
    }
    else
    {
        // Parent process that is the SHELL process.
        wait(NULL); // wait for the child process
        printf("I’m SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", getpid());
    }
    return 0;
}