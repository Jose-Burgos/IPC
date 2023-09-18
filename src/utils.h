#ifndef _UTILS_H
#define _UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <semaphore.h>

#define WORKER_PATH "./bin/worker"
#define SHM_DATA_PATH "/view"
#define SHM_PATH "/shm"
#define SHM_PATH_LEN 6 

#define SLEEP 2
#define BUFFER_SIZE 2048
#define PATH_MAX 30
#define MD5_LEN 33
#define PID 4
#define RESULT_MAX (PATH_MAX + MD5_LEN + PID + 14)


#define READ_END 0
#define WRITE_END 1
#define ERROR -1

#define SLAVES 5

typedef struct Shm
{
    sem_t sem_writer;         
    sem_t sem_reader;         
    size_t files_count; 
    char *buffer_app;
    char *buffer_view;         
} Shm;

#endif