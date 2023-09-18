#ifndef _VIEW_H
#define _VIEW_H

#include "utils.h"
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h> 

void openshm(const char *path, Shm **shm_data);
void closeshm(Shm *shm_data);

#endif