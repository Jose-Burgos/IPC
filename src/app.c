// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "master.h"

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
    fprintf(stdout, "%s\n", SHM_DATA_PATH);
    
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
                    sem_post(&shm_data->sem_reader); // Available data
                    sendFilesToSlave(&data, i, argv); //  Send next file to this slave if available.
                }
            }
        }
    }
    sem_wait(&shm_data->sem_writer); // Waiting for the reader
    destroyshm(shm_data);
    terminateApp(&data);
    return 0;
}

