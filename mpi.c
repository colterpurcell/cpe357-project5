#define _DEFAULT_SOURCE
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MATRIX_DIMENSION_XY 10

/**
 * @brief Takes two matrices and multiplies them as well as two arguments:
 * @param program_name the name of the program called
 * @param instance_count the number of instances to be created
 */
int main(int argc, char const *argv[])
{
    /* Called in the format $ ./program2 par 4 */
    int i, instance_count = atoi(argv[2]);
    int fd[4];
    float *A, *B, *C;
    int *ready;
    fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, 0777);
    fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, 0777);
    fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, 0777);
    fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, 0777);
    ftruncate(fd[0], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    ftruncate(fd[1], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    ftruncate(fd[2], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    ftruncate(fd[3], sizeof(int) * instance_count);
    A = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    B = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    C = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    ready = mmap(NULL, sizeof(int) * instance_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);

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
        char *args[8];
        char total_processes[4];
        char process_id[4];
        sprintf(process_id, "%d", i);
        sprintf(total_processes, "%d", instance_count);
        args[0] = malloc(96);
        args[1] = malloc(96);
        args[2] = malloc(96);
        args[3] = (char *)&A;
        args[4] = (char *)&B;
        args[5] = (char *)&C;
        args[6] = (char *)&ready;
        args[7] = NULL;
        strcpy(args[0], "./");
        strcat(args[0], argv[1]);
        strcpy(args[1], process_id);
        strcpy(args[2], total_processes);

        if (fork() == 0)
        {
            execv(args[0], args);
        }

        free(args[0]);
        free(args[1]);
        free(args[2]);
    }

    munmap(A, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(B, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(C, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(ready, sizeof(int) * instance_count);

    wait(NULL);
    return 0;
}
