#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MD5_LEN 33

char* calculate_md5(const char* filePath) {
    char command[100];
    FILE* fp;

    // Armo al comando para ejectuar md5sum con el path
    snprintf(command, sizeof(command), "md5sum %s", filePath);

    // Uso popen para leer mediante un pipe la salida del comando
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen");
        return NULL;
    }

    // Creo un vector dinamico para almacenar el md5
    char* md5 = (char*)malloc(MD5_LEN);
    if (md5 == NULL) {
        perror("malloc");
        pclose(fp);
        return NULL;
    }

    // Leo con fgets del pipe
    if (fgets(md5, MD5_LEN, fp) == NULL) {
        perror("fgets");
        free(md5);
        pclose(fp);
        return NULL;
    }

    pclose(fp);
    return md5;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 6) {
        fprintf(stderr, "Please pass at least one file path as argument.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char* filePath = argv[i];
        char* md5 = calculate_md5(filePath);
        if (md5 != NULL) {
            printf("PID: %d - %s - %s\n",getpid(),md5,filePath);
            free(md5);
        } else {
            fprintf(stderr, "Process %d failed to calculate MD5 hash for %s\n", getpid(),filePath);
        }
    }

    return 0;
}