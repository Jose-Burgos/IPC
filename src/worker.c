#include "worker.h"

int main()
{
    char filePath[PATH_MAX]; // Assuming file paths won't be longer than 256 characters
    size_t len;
    while (true)
    {
        if (fgets(filePath, sizeof(filePath), stdin) == NULL)
        {
            //EOF, master closed write-end of arguments pipe
            break;
        }
        // Remove the trailing newline character
        len = strlen(filePath);
        if (len > 0 && filePath[len - 1] == '\n')
        {
            filePath[len - 1] = '\0';
        }
        else
        {
            fprintf(stderr, "Unexpected input format or truncated path. Length: %zu, Last char: %c\n", len, filePath[len - 1]);
            // Decide whether to continue, skip or exit
        }
        // Perform MD5 calculation (or whatever processing you need)
        calculate_md5(filePath);
    }
    // When fgets returns NULL, it's an indicator that the pipe has been closed (EOF).
    // Perform any cleanup or final operations here.

    return 0;
}

void calculate_md5(const char *filePath)
{
    char command[100];
    FILE *fp;

    // Build command to execute md5Sum with filePath
    snprintf(command, sizeof(command), "md5sum %s", filePath);

    // Use popen to read through a pipe the output of the command
    fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("popen");
        exit(1);
    }

    // array to store the md5 string
    char md5[MD5_LEN];

    // Read with fgets from the pipe
    if (fgets(md5, MD5_LEN, fp) == NULL)
    {
        perror("fgets");
        pclose(fp);
        exit(1);
    }

    md5[MD5_LEN] = '\0';
    pclose(fp);

    char result[RESULT_MAX];
    snprintf(result, RESULT_MAX, "PID: %d - %s - %s\n", getpid(), md5, filePath);
    write(STDOUT_FILENO, result, strlen(result));
    return;
}
