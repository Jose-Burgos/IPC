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

#define BUFFER_SIZE 2048
#define PATH_MAX 30
#define MD5_LEN 32
#define RESULT_MAX (PATH_MAX + MD5_LEN + 30)

#define WORKER_PATH "./bin/worker"

#define READ_END 0
#define WRITE_END 1

#define SLAVES 5

#endif