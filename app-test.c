#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#define SLAVES 2

int main(int argc, char *argv[])
{
    int fd_results[SLAVES * 2];
    int fd_args[SLAVES * 2];
    int child_pid_arr[SLAVES];
    int child_pid;

    int fdout = open("output.txt", O_CREAT | O_RDWR);
    close(0); // I just want to write

    if (pipe(fd_results) == -1 || pipe(fd_args) == -1)
    {
        perror("pipe");
        exit(1);
    }
    for (int i = 0; i < SLAVES; i++)
    {
        if ((child_pid = fork()) < 0)
        {
            perror("fork");
            exit(2);
        }
        child_pid_arr[i] = child_pid;
    }

    if (child_pid == 0)
    {
        close(fd_results[0]);   // Close the read-end of the result pipe
        dup2(fd_results[1], 1); // Redirect stdout to the write-end of the pipe
        close(fd_results[1]);   // Close the original write-end of the pipe

        // Child process: Read arguments from argsPipe
        close(fd_args[1]);   // Close the write-end of the arguments pipe
        dup2(fd_args[0], 0); // Redirect stdin to the read-end of the arguments pipe
        close(fd_args[0]);   // Close the original read-end of the arguments pipe

        // Read arguments from the pipe and construct the argument list for execv
        char *args[argc + 1];
        args[0] = "./worker";
        char arg_buffer[4096];
        ssize_t bytesRead = 0;
        int i = 1;

        while (read(0, arg_buffer + bytesRead, 1) > 0)
        {
            if (arg_buffer[bytesRead] == ' ')
            {
                arg_buffer[bytesRead] = '\0';
                args[i] = strdup(arg_buffer);
                bytesRead = 0;
                i++;
            }
            else
            {
                bytesRead++;
            }
        }
        args[i] = NULL;
        execv(args[0], args); // Execute "./worker" with the provided arguments
        perror("execv");
        return 1;
    }
    close(fd_results[1]); // close write-end
    close(fd_args[0]);    // close read-end

    for (int i = 1; i < argc; i++)
    {
        write(fd_args[1], argv[i], strlen(argv[i])); // Write each argument to the pipe
        write(fd_args[1], " ", 1);                   // Separate arguments with a space
    }
    close(fd_args[1]); // Close the write-end

    // Read from the result pipe and write to the output file
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd_results[0], buffer, sizeof(buffer))) > 0)
    {
        write(fdout, buffer, bytesRead); // Write to the output.txt file
    }

    close(fd_results[0]);
    close(fdout);

    return 0;
}