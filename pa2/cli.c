#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#define COMMANDS "commands.txt"
#define PARSE "parse.txt"

// Mutex for synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int countLines(char *fname)
{
    FILE *file = fopen(fname, "r");
    int lineCount = 0;
    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        lineCount++;
    }
    fclose(file);
    return lineCount;
}

// Thread function that will handle different processes' outputs
void *handleOutput(void *args)
{
    int *pipe_fd = (int *)args;
    close(pipe_fd[1]);

    FILE *threadPipe = fdopen(pipe_fd[0], "r");

    char buffer[100];
    // Lock
    pthread_mutex_lock(&mutex);
    // Read from the pipe until it is closed
    printf("---- %lu\n", pthread_self());
    while (fgets(buffer, sizeof(buffer), threadPipe))
    {
        printf("%s", buffer);
    }
    printf("---- %lu\n", pthread_self());
    fsync(STDOUT_FILENO);
    // Unlock
    pthread_mutex_unlock(&mutex);
    return NULL;
}

// Initalize pipes
void createPipes(int n, int ***pipes)
{
    *pipes = (int **)(malloc(sizeof(int *) * n));
    int i;
    for (i = 0; i < n; i++)
    {
        (*pipes)[i] = (int *)(malloc(sizeof(int) * 2));
        pipe((*pipes)[i]);
    }
}

// Free pipes
void closePipes(int n, int ***pipes)
{
    int i;
    for (i = 0; i < n; i++)
    {
        close((*pipes)[i][0]);
        close((*pipes)[i][1]);
        free((*pipes)[i]);
    }

    free(*pipes);
}

int main(int argc, char *argv[])
{
    // Max number of child processes will be numLines
    int numLines = countLines(COMMANDS);
    // Open commands.txt and parse.txt files
    FILE *commandsFile = fopen(COMMANDS, "r");
    FILE *parseFile = fopen(PARSE, "w");

    // Check if the files were opened successfully
    if (commandsFile == NULL || parseFile == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    // To store lines in file
    char line[256];

    // To iterate over arguments
    char *arg;
    // Array to store arguments
    char *execArgs[6];
    // Arrays to store child processes / threads
    int childPIDs[numLines];
    pthread_t threads[numLines];
    // Initialize pipes
    int **pipes;
    createPipes(numLines, &pipes);

    // Read each line until the end of the file
    int numForks = 0;
    int numPipes = 0;
    int numThreads = 0;
    while (fgets(line, sizeof(line), commandsFile))
    {
        // If \n exists at the end, get rid of it
        int lineLength = strlen(line);
        if (lineLength > 0 && line[lineLength - 1] == '\n')
        {
            line[lineLength - 1] = '\0'; // Replace with null char
        }

        // Variables to store indexes of arguments
        int inRedirect = 0;
        int outRedirect = 0;
        int options = 0;
        int isBackground = 0;
        char *input = "";
        char *option = "";
        char *command = "";
        // Get first arg from line
        arg = strtok(line, " ");
        if (strcmp(arg, "wait") == 0)
        {
            fprintf(parseFile, "----------\nCommand: wait\nInputs:\nOptions:\nRedirection: -\nBackground Job: n\n----------\n");
            int t = 0;
            for (t = 0; t < numThreads; t++)
            {
                pthread_join(threads[t], NULL);
            }
            int p;
            for (p = 0; p < numForks; p++)
            {
                int status;
                waitpid(childPIDs[p], &status, 0);
            }
            continue;
        }
        // Iterate over arguments and save them
        int i = 0;
        while (arg != NULL)
        {
            char *temp;
            char *freeEscaped;
            temp = strdup(arg);
            // If an argument is redirection set redirection
            if (i == 0)
            {
                command = arg;
            }
            else if (arg[0] == '>')
            {
                outRedirect = i;
            }
            else if (arg[0] == '<')
            {
                inRedirect = i;
            }
            else if (arg[0] == '-' && strcmp(option, "") == 0)
            {
                options = i;
                option = arg;
            }
            else if (arg[0] == '-' && strcmp(option, "") != 0)
            {
                // If argument starts with '-' and option is already set,
                // set it as input and escape with '\\' (there cannot be 2 options)
                input = arg;
                size_t newLen = strlen(temp) + 2;
                char *escaped = (char *)malloc(newLen);

                if (escaped == NULL)
                {
                    printf("MEMORY ALLOCATION FAILED!\n");
                    exit(1);
                }
                sprintf(escaped, "%s%s", "\\", temp);
                temp = escaped;
            }
            else if (!outRedirect && !inRedirect && strcmp(arg, "&") != 0)
            {
                input = arg;
            }
            execArgs[i] = temp;
            // Continue with next argument
            arg = strtok(NULL, " ");
            i++;
        }

        // At the end, if command is grep and no input is provided:
        // Set option as the input because there must be an input for grep.
        if (strcmp(command, "grep") == 0 && strcmp(input, "") == 0)
        {
            input = option;
            size_t newLen = strlen(option) + 2;
            char *escaped = (char *)malloc(newLen);

            if (escaped == NULL)
            {
                printf("MEMORY ALLOCATION FAILED!\n");
                exit(1);
            }
            sprintf(escaped, "%s%s", "\\", option);

            option = "";
            execArgs[options] = escaped;
        }

        // Check if background job
        if (execArgs[i - 1][0] == '&')
        {
            isBackground = 1;
        }

        if (!outRedirect)
        {
            numPipes++;
        }

        int rc = fork();
        numForks++;
        if (rc < 0)
        {
            printf("Fork failed!\n");
            exit(1);
        }
        else if (rc > 0)
        {
            // Parent process
            childPIDs[numForks] = (int)rc;
            // Create a thread if command does not have output redirection
            if (!outRedirect)
            {
                pthread_create(&threads[numThreads], NULL, handleOutput, pipes[numPipes - 1]);
                numThreads++;
            }

            fprintf(parseFile, "----------\n");
            fprintf(parseFile, "Command: %s\n", execArgs[0]);
            fprintf(parseFile, "Inputs: %s\n", input);
            fprintf(parseFile, "Options: %s\n", option);

            if (inRedirect)
            {
                fprintf(parseFile, "Redirection: <\n");
            }
            else if (outRedirect)
            {
                fprintf(parseFile, "Redirection: >\n");
            }
            else
            {
                fprintf(parseFile, "Redirection: -\n");
            }
            if (isBackground)
            {
                fprintf(parseFile, "Background Job: y\n");
            }
            else
            {
                fprintf(parseFile, "Background Job: n\n");
            }
            fprintf(parseFile, "----------\n");

            // If created job is not background, wait for that job to terminate
            if (!isBackground)
            {
                int status;
                waitpid(rc, &status, 0);
            }
        }
        else
        {
            // Child process
            // If command has output redirection, no thread will be created
            if (outRedirect)
            {
                char *outputFile = execArgs[outRedirect + 1];
                // Redirect STDOUT to file
                int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd == -1)
                {
                    printf("Error redirecting STDOUT!\n");
                    exit(1);
                }
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);

                // Set last arg as NULL
                execArgs[outRedirect] = NULL;
            }
            // If the command has no output redirction, redirect STDOUT to pipe
            else
            {
                dup2(pipes[numPipes - 1][1], STDOUT_FILENO);
                close(pipes[numPipes - 1][0]);
                close(pipes[numPipes - 1][1]);

                // If command has input redirection, redirect STDIN to file
                if (inRedirect)
                {
                    char *inputFile = execArgs[inRedirect + 1];
                    // Redirect STDOUT to file
                    int input_fd = open(inputFile, O_RDONLY);
                    if (input_fd == -1)
                    {
                        printf("Error redirecting STDIN!\n");
                        exit(1);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);

                    // Set last arg as NULL
                    execArgs[inRedirect] = NULL;
                }
                else
                {
                    // Set last arg as NULL
                    if (isBackground)
                    {
                        execArgs[i - 1] = NULL;
                    }
                    else
                    {
                        execArgs[i] = NULL;
                    }
                }
            }
            // Execute command
            execvp(execArgs[0], execArgs);
        }
    }

    // Wait for all threads and processes before terminating
    int t = 0;
    for (t = 0; t < numThreads; t++)
    {
        pthread_join(threads[t], NULL);
    }
    int p;
    for (p = 0; p < numForks; p++)
    {
        int status;
        waitpid(childPIDs[p], &status, 0);
    }
    // Close the files
    closePipes(numPipes, &pipes);
    fclose(commandsFile);
    fclose(parseFile);
    return 0;
}