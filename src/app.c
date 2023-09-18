// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "app.h"

int main(int argc, char **argv)
{
    unlink("/dev/shm/shm");
    unlink("/dev/shm/view");

    AppData data;

    initAppData(&data, argc);
    launchSlaves(&data);

    Shm *shm_data;
    if (createshm(SHM_DATA_PATH, (argc - 1), &shm_data) == -1)
    {
        terminateApp(&data);
        return ERROR;
    }
    printf("%s\n", SHM_DATA_PATH);
    sleep(2);

    for (int i = 0; i < SLAVES; i++) // Initial single file distribution to each slave.
    {
        sendFilesToSlave(&data, i, argv);
    }

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    int max_fd;

    int shm_idx = 0;

    while (data.pendingResults)
    {
        FD_ZERO(&read_fds);
        max_fd = -1;
        for (int i = 0; i < SLAVES; i++) // Add slave pipes to the fd set for reading
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
            destroyshm(shm_data);
            exit(1);
        }

        for (int i = 0; i < SLAVES; i++)
        {
            if (FD_ISSET(data.fd_results[i * 2 + READ_END], &read_fds))
            {
                size_t bytesRead = read(data.fd_results[i * 2 + READ_END], buffer, sizeof(buffer));

                if (bytesRead > 0)
                {
                    data.pendingResults--; // One less result pending.
                    data.slaves_load[i]--;
                    write(data.fdout, buffer, bytesRead); // Write result to output file.
                    strncpy(shm_data->buffer_path + (shm_idx * RESULT_MAX), buffer, bytesRead);
                    shm_idx++;
                    sem_post(&shm_data->sem_writer);
                    sendFilesToSlave(&data, i, argv); //  Send next file to this slave if available.
                }
            }
        }
    }
    sem_wait(&shm_data->sem_reader);
    destroyshm(shm_data);
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
    // printf("Master: Closing slave number %d's argument pipe\n", slaveIndex);
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

        // printf("Master: writing argument %s to slave number %d's pipe\n", argv[data->currentFileIndex], slaveIndex);

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
        }
        else
        {
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

int createshm(char *path, int argc, Shm **shm_data)
{
    if (argc <= 0)
    {
        return ERROR;
    }

    int fd_data = shm_open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd_data == ERROR)
    {
        perror("shm open error");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd_data, sizeof(Shm)) == -1)
    {
        close(fd_data);
        unlink("/dev/shm/view");
        perror("space reservation failed");
        exit(EXIT_FAILURE);
    }

    Shm *shm_aux = mmap(NULL, sizeof(Shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0);
    close(fd_data);
    if (shm_aux == MAP_FAILED)
    {
        unlink("/dev/shm/view");
        perror("map failed");
        exit(EXIT_FAILURE);
    }

    shm_aux->files_count = argc;

    if (sem_init(&shm_aux->sem_reader, 1, 1) == ERROR)
    {
        destroyshm(shm_aux);
        perror("semaphore init failed");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&shm_aux->sem_writer, 1, 0) == ERROR)
    {
        destroyshm(shm_aux);
        perror("semaphore init failed");
        exit(EXIT_FAILURE);
    }

    int fd_buffer = shm_open(SHM_PATH, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd_buffer == ERROR)
    {
        perror("shm open error");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd_buffer, RESULT_MAX * argc) == -1)
    {
        close(fd_buffer);
        perror("space reservation failed");
        exit(EXIT_FAILURE);
    }

    shm_aux->buffer_path = mmap(NULL, RESULT_MAX * argc, PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0);
    close(fd_buffer);
    if (shm_aux->buffer_path == MAP_FAILED)
    {
        destroyshm(shm_aux);
        perror("map failed");
        exit(ERROR);
    }
    *shm_data = shm_aux;
    return 0;
}

void destroyshm(Shm *shm_data)
{
    sem_destroy(&shm_data->sem_reader);
    sem_destroy(&shm_data->sem_writer);
    munmap(shm_data->buffer_path, shm_data->files_count * RESULT_MAX);
    munmap(shm_data, sizeof(Shm));
}