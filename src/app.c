#include "app.h"

int main(int argc, char **argv) 
{
    AppData data;
    initAppData(&data, argc);
    launchSlaves(&data);
    

    // Initial single file distribution to each slave.
    for (int i = 0; i < SLAVES; i++)
    {
        sendFilesToSlave(&data, i, argv);
    }

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int max_fd;

    while (data.pendingResults)
    {
        FD_ZERO(&read_fds);
        max_fd = -1;

        // Add slave pipes to the fd set for reading
        for (int i = 0; i < SLAVES; i++)
        {
            FD_SET(data.fd_results[i * 2 + READ_END], &read_fds);
            if (data.fd_results[i * 2 + READ_END] > max_fd)
            {
                max_fd = data.fd_results[i * 2 + READ_END];
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("select");
            exit(1);
        }

        for (int i = 0; i < SLAVES; i++)
        {
            if (FD_ISSET(data.fd_results[i * 2 + READ_END], &read_fds))
            {
                size_t bytesRead = read(data.fd_results[i * 2 + READ_END], buffer, sizeof(buffer));

                if (bytesRead > 0)
                {
                    //printf("Read %s from slave %d\n", buffer, i);
                    data.pendingResults--;                // One less result pending.
                    data.slaves_load[i]--;  
                    //printf("Slave %d now has load %d and the total pending results is %d\n", i, data.slaves_load[i], data.pendingResults);              // This slave has one less job.
                    write(data.fdout, buffer, bytesRead); // Write result to output file.
                    
                    // Send next file to this slave if available.
                    sendFilesToSlave(&data, i, argv);
                }
                // else
                // {
                //     // Close pipe when done.
                //     close(data.fd_results[i * 2 + READ_END]);
                // }
            }
        
        }
    }
    terminateApp(&data);
    return 0;
}

void initAppData(AppData *data, int argc)
{
    data->totalFiles = argc - 1;
    data->currentFileIndex = 1;
    data->pendingResults = 0;
    int fdout = open("output.txt", O_CREAT | O_RDWR, 0644);
    if (fdout == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    data->fdout = fdout;
    for (int i = 0; i < SLAVES; i++)
    {
        if (pipe((data->fd_results) + i * 2) == -1 || pipe((data->fd_args) + i * 2) == -1)
        {
            perror("pipe");
            // Close any previously created pipes before exiting
            for (int j = 0; j < i; j++)
            {
                close(data->fd_results[j * 2]);
                close(data->fd_results[j * 2 + 1]);
                close(data->fd_args[j * 2]);
                close(data->fd_args[j * 2 + 1]);
            }
            exit(1);
        }
        data->child_pids[i] = -1;
        data->slaves_load[i] = 0;
    }
    return;
}

static void slaveDone(int slaveIndex, AppData *data)
{
    //printf("Master: Closing slave number %d's argument pipe\n", slaveIndex);
    close(data->fd_args[slaveIndex * 2 + WRITE_END]);
    return;
}

void sendFilesToSlave(AppData *data, int slaveIndex, char **argv)
{
    if (data->currentFileIndex <= data->totalFiles)
    {
        // Update load and pending results
        data->slaves_load[slaveIndex]++;
        data->pendingResults++;

        //printf("Master: writing argument %s to slave number %d's pipe\n", argv[data->currentFileIndex], slaveIndex);

        if (write(data->fd_args[slaveIndex * 2 + WRITE_END], argv[data->currentFileIndex], strlen(argv[data->currentFileIndex])) == -1)
        {
            perror("write");
            exit(1);
        }

        // Send the newline delimiter
        if (write(data->fd_args[slaveIndex * 2 + WRITE_END], "\n", 1) == -1)
        {
            perror("write");
            exit(1);
        }

        data->currentFileIndex++;
    }
    else if (data->slaves_load[slaveIndex] == 0)
    {
        // This worker is done, close its pipe
        slaveDone(slaveIndex, data);
    }
}

void launchSlaves(AppData *data)
{
    for (int i = 0; i < SLAVES; i++)
    {
        data->child_pids[i] = fork();

        if (data->child_pids[i] == -1)
        {
            perror("fork");
            closePipes(data, i);
            exit(1);
        }

        // We're in the child process
        if (data->child_pids[i] == 0)
        {
            if (dup2(data->fd_args[i * 2 + READ_END], STDIN_FILENO) == -1 || dup2(data->fd_results[i * 2 + WRITE_END], STDOUT_FILENO) == -1)
            {
                perror("dup2");
                exit(1);
            }

            closePipes(data, i);

            // Execute the worker using execv
            char *argv[] = {WORKER_PATH, NULL};
            execv(argv[0], argv);

            // If execv fails
            perror("execv");
            exit(1);
        }

        // We're in the parent process
        else
        {
            close(data->fd_args[i * 2 + READ_END]);
            close(data->fd_results[i * 2 + WRITE_END]);
        }
    }
}

// El padre cierra para cada creacion de un hijo el r-end de argumentos y el w-end de resultados
// cada hijo hereda sus 4 correspondientes ends de sus pipes y todos los w-end de argumentos y r-end de resultados del padre
// cada hijo tiene que cerrar sus 4 fds y todos los r-end args y w-end res
// el primero tiene que cerrar todos
void closePipes(AppData *data, int slaveIndex)
{
    for (int i = 0; i <= slaveIndex; i++)
    {
        if (i == slaveIndex)
        {
            close(data->fd_args[i * 2 + WRITE_END]);
            close(data->fd_args[i * 2 + READ_END]);
            close(data->fd_results[i * 2 + WRITE_END]);
            close(data->fd_results[i * 2 + READ_END]);
        } else {
            close(data->fd_args[i * 2 + WRITE_END]);
            close(data->fd_results[i * 2 + READ_END]);
        }
    }
}

void terminateApp(AppData *data)
{
    // All files processed and results gathered. Close remaining file descriptors.
    for (int i = 0; i < SLAVES; i++)
    {
        close(data->fd_args[i * 2 + WRITE_END]);
        close(data->fd_results[i * 2 + READ_END]);
    }

    // Wait for all child processes to finish.
    for (int i = 0; i < SLAVES; i++)
    {
        waitpid(data->child_pids[i], NULL, 0);
    }

    close(data->fdout);

    return;
}