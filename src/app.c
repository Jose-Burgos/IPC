#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include "app.h"

int main(int argc, char *argv[]) {
    AppData data;
    int fdout = open("output.txt", O_CREAT | O_RDWR);

    initAppData(&data);
    distributeFiles(&data, argc, argv);
    waitForChildren(&data);
    gatherResults(&data, fdout);

    close(fdout);
    return 0;
}

void initAppData(AppData *data) {
    createPipes(data->fd_results, data->fd_args);
}

void distributeFiles(AppData *data, int argc, char **argv) {
    int current_arg = 1;
    int total_files = argc - 1;
    writeArguments(total_files, &current_arg, total_files / SLAVES, total_files % SLAVES, argv, data->fd_args);
    createChildren(data->child_pids, total_files, argv, data->fd_results, data->fd_args);
    closeAppPipes(data->fd_results,OUTPUT);
}

void waitForChildren(AppData *data) {
    for (int i = 0; i < SLAVES; i++) {
        waitpid(data->child_pids[i], NULL, 0);
    }
}

void gatherResults(AppData *data, int fdout) {
    closeAppPipes(data->fd_args, OUTPUT);
    closeAppPipes(data->fd_args, INPUT);

    char buffer[4096];
    ssize_t bytesRead = 0;

    fd_set read_fds;
    int max_fd = -1;
    FD_ZERO(&read_fds);

    for (int i = 0; i < SLAVES; i++) {
        FD_SET(data->fd_results[i * 2], &read_fds);
        if (data->fd_results[i * 2] > max_fd) {
            max_fd = data->fd_results[i * 2];
        }
    }

    int slaves_remaining = SLAVES;
    while (slaves_remaining > 0) {
        fd_set temp_set = read_fds;
        int activity = select(max_fd + 1, &temp_set, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i < SLAVES; i++) {
            if (FD_ISSET(data->fd_results[i * 2], &temp_set)) {
                while ((bytesRead = read(data->fd_results[i * 2], buffer, sizeof(buffer))) > 0) {
                    write(fdout, buffer, bytesRead);
                }
                FD_CLR(data->fd_results[i * 2], &read_fds);
                slaves_remaining--;
            }
        }
    }
    closeAppPipes(data->fd_results, INPUT);
}

void createPipes(int *res, int *arg)
{
    for (int i = 0; i < SLAVES; i++)
    {
        if (pipe(res + i * 2) == -1 || pipe(arg + i * 2) == -1)
        {
            perror("pipe");
            exit(1);
        }
    }
    return;
}

void closeAndRedirect(int *res, int *arg, int i)
{

    // Child process: Read arguments from arg Pipe
    close(arg[i * 2 + 1]); // Close the write-end of the arguments pipe
    dup2(arg[i * 2], INPUT);   // Redirect stdin to the read-end of the arguments pipe
    close(arg[i * 2]);     // Close the original read-end of the arguments pipe
    // Child process: Write results to res Pipe
    close(res[i * 2]);       // Close the read-end of the result pipe
    dup2(res[i * 2 + 1], OUTPUT); // Redirect stdout to the write-end of the pipe
    close(res[i * 2 + 1]);   // Close the original write-end of the pipe

    // Close every remaining unwanted pipe
    closeSiblingPipes(res, arg, i);

    return;
}

void writeArguments(int total_files, int *current_arg_ptr, int files_per_slave, int extra_file_slaves, char **argv, int *fd_args)
{
    for (int i = 0; i < SLAVES; i++)
    {
        int current_slave_files = files_per_slave + (i < extra_file_slaves ? 1 : 0); // Check if the current slave processes an extra file

        for (int j = 0; j < current_slave_files; j++)
        {
            write(fd_args[i * 2 + 1], argv[*current_arg_ptr], strlen(argv[*current_arg_ptr]));
            write(fd_args[i * 2 + 1], "\n", 1);
            (*current_arg_ptr)++;
        }
    }
}

void closeSiblingPipes(int *res, int *arg, int i)
{
    for (int j = 0; j < SLAVES; j++)
    {
        if (j != i)
        {
            close(arg[j * 2]);
            close(arg[j * 2 + 1]);
            close(res[j * 2]);
            close(res[j * 2 + 1]);
        }
    }
}

void createChildren(pid_t *child_pids, int total_files, char **argv, int *res, int *arg)
{
    pid_t pid;

    int files_per_slave = total_files / SLAVES;
    int extra_file_slaves = total_files % SLAVES;

    for (int i = 0; i < SLAVES; i++)
    {
        int current_slave_files = files_per_slave + (i < extra_file_slaves ? 1 : 0);

        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(2);
        }
        if (pid == 0)
        { // Child process
            closeAndRedirect(res, arg, i);

            char *args[current_slave_files + 2];
            args[0] = "./worker";

            char buffer[256];
            for (int j = 0; j < current_slave_files; j++)
            {
                int idx = 0; // position in the buffer
                char ch;     // read character
                while (read(0, &ch, 1) == 1 && ch != '\n')
                {
                    buffer[idx++] = ch;
                }
                buffer[idx] = '\0'; // null-terminate
                args[j + 1] = strdup(buffer);
            }
            args[current_slave_files + 1] = NULL;

            execv(args[0], args);
            perror("execv");

            // Free allocated memory in case execv fails
            for (int j = 1; j <= current_slave_files; j++)
            {
                free(args[j]);
            }
            exit(1);
        }
        else
        {
            child_pids[i] = pid;
        }
    }
}

void closeAppPipes(int *fds, int flag)
{
    for (int i = 0; i < SLAVES; i++)
    {
        close(fds[i * 2 + flag]);
    }
    return;
}
