// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "view.h"

int main(int argc, char **argv)
{
    char path[SHM_PATH_LEN];
    if (argc < 2)
    {
        read(0, path, SHM_PATH_LEN);
    }
    else if (argc == 2)
    {
        strncpy(path, argv[1], SHM_PATH_LEN);
    }
    else
    {
        fprintf(stderr, "Incorrect amount of arguments\n");
        return -1;
    }
    path[SHM_PATH_LEN - 1] = '\0';
    Shm *data_shm;
    int idx = 0;
    openshm(path, &data_shm);

    while (idx < (int)data_shm->files_count)
    {
        sem_wait(&data_shm->sem_reader); // Wait for new data to be available
        fprintf(stdout ,data_shm->buffer_view + (idx * RESULT_MAX));
        idx++;
    }
    sem_post(&data_shm->sem_writer); // Reader finish
    closeshm(data_shm);
    return 0;
}

void openshm(const char *path, Shm **shm_data)
{
    int fd_data;
    Shm *data;
    if ((fd_data = shm_open(path, O_RDWR, 0600)) == -1)
    {
        perror("shm open failed");
        exit(EXIT_FAILURE);
    }
    if ((data = mmap(NULL, sizeof(Shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd_data, 0)) == MAP_FAILED)
    {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    close(fd_data);
    int fd_buffer;
    if ((fd_buffer = shm_open(SHM_PATH, O_RDONLY, 0600)) == -1)
    {
        perror("shm open failed");
        exit(EXIT_FAILURE);
    }
    if ((data->buffer_view = mmap(NULL, RESULT_MAX * data->files_count, PROT_READ, MAP_SHARED, fd_buffer, 0)) == MAP_FAILED)
    {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    close(fd_buffer);
    *shm_data = data;
}

void closeshm(Shm *shm_data)
{
    munmap(shm_data->buffer_view, shm_data->files_count * RESULT_MAX);
    unlink("/dev/shm/shm");
    munmap(shm_data, sizeof(Shm));
    unlink("/dev/shm/view");
}
