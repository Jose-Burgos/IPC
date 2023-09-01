#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd_results[2];
    int fd_args[2];
    int child_pid;
    int fdout = open("output.txt", O_CREAT | O_RDWR);
    close(0); // I just want to write

    if ((pipe(fd_results) || pipe(fd_args)) == -1 )
    {
        perror("Error: pipe");
        exit(1);
    }

    if ((child_pid = fork()) < 0)
    {
        perror("Error: fork");
        exit(2);
    }

    if (child_pid == 0)
    {
        close(fd_results[0]);   // Close the read-end of the result pipe
        dup2(fd_results[1], 1); // Redirect stdout to the write-end of the pipe
        close(fd_results[1]);   // Close the original write-end of the pipe

        char *args[argc + 1];
        args[0] = "./worker";
        for (int i = 1; i < argc; i++) {
            args[i] = argv[i];
        }
        args[argc] = NULL;

        execve("./worker", args, NULL);
        perror("execve");
        return 1;
    }
    close(fd_results[1]); // close write-end

    // Read from the result pipe and write to the output file
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd_results[0], buffer, sizeof(buffer))) > 0) {
        write(fdout, buffer, bytesRead); // Write to the output.txt file
    }

    close(fd_results[0]);
    close(fdout);

    return 0;
}