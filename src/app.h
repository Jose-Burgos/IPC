#ifndef _APP_H
#define _APP_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define SLAVES 6
#define INPUT 0
#define OUTPUT 1

typedef struct {
    int fd_args[SLAVES * 2];
    int fd_results[SLAVES * 2];
    pid_t child_pids[SLAVES];
} AppData;

// Initializes the data structure that holds file descriptors and child process IDs.
void initAppData(AppData *data);

// Distributes file arguments among the slave processes.
void distributeFiles(AppData *data, int argc, char **argv);

// Waits for all children (slaves) processes to finish execution.
void waitForChildren(AppData *data);

// Gathers and writes results from slave processes to the output file.
void gatherResults(AppData *data, int fdout);

// Creates two sets of pipes: one for results and one for passing arguments.
void createPipes(int *res, int *arg);

// For the child process: Closes unused pipe ends and redirects standard input/output to the relevant pipes.
void closeAndRedirect(int *res, int *arg, int i);

// Closes all file descriptors of a given type (either INPUT or OUTPUT) in the given set (either results or arguments)
void closeAppPipes(int *fds, int flag);

// Writes file arguments to the pipe ends for slave processes to read.
void writeArguments(int total_files, int *current_arg_ptr, int files_per_slave, int extra_file_slaves, char **argv, int *fd_args);

// Closes pipes that a child process shouldn't interact with.
void closeSiblingPipes(int *res, int *arg, int i);

// Forks child processes, each representing a "slave", and sets up their environment.
void createChildren(pid_t *child_pids, int total_files, char **argv, int *res, int *arg);

#endif
