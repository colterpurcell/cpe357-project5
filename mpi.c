#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/**
 * @brief Takes two matrices and multiplies them as well as two arguments:
 * @param program_name the name of the program called
 * @param instance_count the number of instances to be created
 */
int main(int argc, char const *argv[])
{
    /* Called in the format $ ./program2 par 4 */
    int i, instance_count = atoi(argv[2]);

    if (argc != 3)
    {
        printf("Usage: %s par instance_count\n", argv[0]);
        exit(1);
    }
    if (instance_count > 10)
    {
        printf("instance_count must be <= 10\n");
        exit(1);
    }

    for (i = 0; i < instance_count; i++)
    {
        char *args[4];
        char total_processes[4];
        char process_id[4];
        sprintf(process_id, "%d", i);
        sprintf(total_processes, "%d", instance_count);
        args[0] = malloc(96);
        args[1] = malloc(96);
        args[2] = malloc(96);
        args[3] = NULL;
        strcpy(args[0], "./");
        strcat(args[0], argv[1]);
        strcpy(args[1], process_id);
        strcpy(args[2], total_processes);

        if (fork() == 0)
        {
            execvp(args[0], args);
        }

        free(args[0]);
        free(args[1]);
        free(args[2]);
    }
    wait(NULL);
    return 0;
}
