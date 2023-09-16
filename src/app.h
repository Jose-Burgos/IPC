#ifndef _APP_H
#define _APP_H
#include "utils.h"

typedef struct {
    int fd_args[SLAVES * 2];
    int fd_results[SLAVES * 2];
    pid_t child_pids[SLAVES];
    int totalFiles; //amount of files to process
    int fdout; //file descriptor for the output file with results
    int currentFileIndex; //to keep track of the current argument of argv
    int pendingResults; //amount of results yet to be read
    int slaves_load[SLAVES]; 
} AppData;

// Initializes the data structure
void initAppData(AppData *data, int argc);

// Closes every pipe
void closePipes(AppData *data, int slaveIndex);

// Launches every child process (fork) leaving them ready to receive arguments through STDIN
void launchSlaves(AppData *data);

// Sends file paths as arguments to the slaves.
void sendFilesToSlave(AppData *data, int slaveIndex, char **argv);

// Terminates app when there's nothing left to do
void terminateApp(AppData *data);

#endif 