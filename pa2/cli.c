#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

/*
// Structure to hold information for each thread
struct ThreadInfo {
    FILE *pipe;
    pthread_t thread_id;
};
*/

// Mutex for synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread function that will handle different processes' outputs
void *handleOutput(void *args) {
    struct FILE *threadPipe = (FILE *)args;
    char buffer[100];

    // Read from the pipe until it is closed
    while (fgets(buffer, sizeof(buffer), threadPipe) != NULL) {
        // Lock
        pthread_mutex_lock(&mutex);
        // Print statements
        printf("Thread %lu: %s", pthread_self(), buffer);
        // Unlock
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    // Read the commands.txt file
    FILE *file = fopen("test_commands.txt", "r");

    // Check if the file was opened successfully
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // To store lines in file
    char line[256];

    // To iterate over arguments
    char* arg;
    // Array to store arguments
    char *execArgs[6];
    
    // Read each line until the end of the file
    while (fgets(line, sizeof(line), file))
    {
        // If \n exists at the end, get rid of it
        int lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] == '\n') {
            line[lineLength - 1] = '\0'; // Replace with null char
        }
        
        int inRedirect = 0;
        int outRedirect = 0;    
        int isBackground = 0;    
        // Get first arg from line
        arg = strtok(line, " ");
        if (arg == "wait") {
            // TODO: wait operation
            continue;
        }
        // Iterate over arguments and save them
        int i = 0;
        while( arg != NULL ) {
            // If an argument is redirection set redirection
            if (arg[0] == '>') {
                outRedirect = i;
            }
            else if (arg[0] == '<') {
                inRedirect = i;
            }

            execArgs[i] = arg;
            
            // Continue with next argument
            arg = strtok(NULL, " ");
            i++;
        }
        // Set last argument as NULL
        execArgs[i] = NULL;
        // Check if background job
        if (execArgs[i - 1][0] == '&') {
            isBackground = 1;
        }

        // If has redirect
        if (outRedirect) {
            int rc = fork();
            if (rc < 0) {
                printf("Fork failed!\n");
                exit(1);
            }
            else if (rc > 0) {
                // Parent process
                if (!isBackground) {
                    // If created job is background, wait for that job to terminate
                    int status;
                    waitpid(rc, &status, 0);
                }
            }
            else {
                // Child process
                char* outputFile = execArgs[outRedirect + 1];
                // Redirect STDOUT to file
                int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd == -1) {
                    printf("Error redirecting STDOUT!\n");
                    exit(1);
                }
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);

                // Set last arg as NULL
                execArgs[outRedirect] = NULL;
                // Execute command
                execvp(execArgs[0], execArgs);
            }
        }
        // Does not have redirect: threads will handle outputs
        else {
            // Prepare pipe
            int pipe_fd[2];
            int piped = pipe(pipe_fd);
            if (piped < 0)
            {
                printf("Piping failed!\n");
                exit(1);
            }
        }
        /*
        int j = 0;
        for (j = 0; j < i; j++) {
            printf("%s ", execArgs[j]);
        }
        if (outRedirect) {
            printf("Has redirect");
        }
        printf("\n");
        */
    }
    wait(NULL);
    wait(NULL);
    // Close the file
    fclose(file);
    return 0;
}