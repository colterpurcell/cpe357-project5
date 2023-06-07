#define _BSD_SOURCE
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MATRIX_DIMENSION_XY 10

/* SEARCH FOR TODO */

/************************************************************************************************************************* */
/* sets one element of the matrix */
void set_matrix_elem(float *M, int x, int y, float f)
{
    M[x + y * MATRIX_DIMENSION_XY] = f;
}
/************************************************************************************************************************* */
/* lets see it both are the same */
int quadratic_matrix_compare(float *A, float *B)
{
    int a, b;
    for (a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (b = 0; b < MATRIX_DIMENSION_XY; b++)
            if (A[a + b * MATRIX_DIMENSION_XY] != B[a + b * MATRIX_DIMENSION_XY])
                return 0;

    return 1;
}
/************************************************************************************************************************* */
/* print a matrix */
void quadratic_matrix_print(float *C)
{
    int a, b;
    fprintf(stderr, "\n");
    for (a = 0; a < MATRIX_DIMENSION_XY; a++)
    {
        fprintf(stderr, "\n");
        for (b = 0; b < MATRIX_DIMENSION_XY; b++)
            fprintf(stderr, "%.2f,", C[a + b * MATRIX_DIMENSION_XY]);
    }
    fprintf(stderr, "\n");
}
/************************************************************************************************************************* */
/* multiply two matrices */
void quadratic_matrix_multiplication(float *A, float *B, float *C)
{
    int a, b, c;
    /* nullify the result matrix first */
    for (a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (b = 0; b < MATRIX_DIMENSION_XY; b++)
            C[a + b * MATRIX_DIMENSION_XY] = 0.0;
    /* multiply */
    for (a = 0; a < MATRIX_DIMENSION_XY; a++)         /* over all cols a */
        for (b = 0; b < MATRIX_DIMENSION_XY; b++)     /* over all rows b */
            for (c = 0; c < MATRIX_DIMENSION_XY; c++) /* over all rows/cols left */
            {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
}
/************************************************************************************************************************* */
void synch(int par_id, int par_count, int *ready)
{
    /* TODO: synch algorithm. make sure, ALL processes get stuck here until all ARE here
     * use the shared memory object "ready" for this. once all ready bits equal one another, all processes are here
     * values can be any positive integer, but they must be equal for all processes
     */
    int i;
    int allready = 0;
    /* fprintf(stderr, "process %d waiting for %d\n", par_id, ready[0]); */

    while (!allready)
    {
        allready = 1;
        for (i = 0; i < par_count; i++)
        {
            if (ready[i] != ready[0])
            {
                allready = 0;
            }
        }
    }

    for (i = 0; i < 100000; i++)
        ;
    /* fprintf(stderr, "process %d ready %d\n", par_id, ready[0]); */
}

void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float *A, float *B, float *C, int *ready, pthread_mutex_t *mut)
{

    int x, y, c;
    int start = (par_id)*MATRIX_DIMENSION_XY / par_count;
    int end = (par_id + 1) * MATRIX_DIMENSION_XY / par_count;
    /* multiply */
    /* for (x = 0; x < 100000; x++)
        ; */
    pthread_mutex_lock(mut);
    for (x = start; x < end; x++)                     /* over all cols a */
        for (y = 0; y < MATRIX_DIMENSION_XY; y++)     /* over all rows b */
            for (c = 0; c < MATRIX_DIMENSION_XY; c++) /* over all rows/cols left */
            {
                C[x + y * MATRIX_DIMENSION_XY] += A[c + y * MATRIX_DIMENSION_XY] * B[x + c * MATRIX_DIMENSION_XY];
            }
    pthread_mutex_unlock(mut);
}

void reset(int par_count, int *ready, pthread_mutex_t *mut)
{
    int i;
    pthread_mutex_lock(mut);
    for (i = 0; i < par_count; i++)
    {
        ready[i] = 0;
    }
    pthread_mutex_unlock(mut);
}

/************************************************************************************************************************* */
int main(int argc, char *argv[])
{
    int par_id = 0;    /* the parallel ID of this process */
    int par_count = 1; /* the amount of processes */
    float *A, *B, *C;  /* matrices A,B and C */
    int *ready;        /* needed for synch */
    int fd[4];
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    par_id = atoi(argv[1]);
    par_count = atoi(argv[2]);
    /* strcpy(shared_mem_matrix,argv[3]); */

    if (par_count == 1)
    {
        printf("only one process\n");
    }

    if (par_id == 0)
    {
        /* TODO: init the shared memory for A,B,C, ready. shm_open with C_CREAT here! then ftruncate! then mmap */
        pthread_mutex_lock(&mutex);
        fd[0] = shm_open("matA", O_CREAT | O_RDWR, 0777);
        fd[1] = shm_open("matB", O_CREAT | O_RDWR, 0777);
        fd[2] = shm_open("matC", O_CREAT | O_RDWR, 0777);
        fd[3] = shm_open("syncobj", O_CREAT | O_RDWR, 0777);
        ftruncate(fd[0], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[1], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[2], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[3], sizeof(int) * par_count);
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        /* TODO: init the shared memory for A,B,C, ready. shm_open withOUT C_CREAT here! NO ftruncate! but yes to mmap */ /* needed for initalizing synch */
        sleep(3);
        fd[0] = shm_open("matA", O_RDWR, 0777);
        fd[1] = shm_open("matB", O_RDWR, 0777);
        fd[2] = shm_open("matC", O_RDWR, 0777);
        fd[3] = shm_open("syncobj", O_RDWR, 0777);
    }

    A = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    B = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    C = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    ready = mmap(NULL, sizeof(int) * par_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);

    if (par_id == 0)
    {
        reset(par_count, ready, &mutex);
    }

    ready[par_id]++;

    /* fprintf(stderr, "process %d hung up on synch 1\n", par_id); */
    synch(par_id, par_count, ready);

    if (par_id == 0)
    {
        /* TODO: initialize the matrices A and B */
        int x, y;
        pthread_mutex_lock(&mutex);
        for (x = 0; x < MATRIX_DIMENSION_XY; x++)
            for (y = 0; y < MATRIX_DIMENSION_XY; y++)
            {
                set_matrix_elem(A, x, y, x + y);
                set_matrix_elem(B, x, y, x + y);
            }
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        usleep(1000);
    }

    ready[par_id]++;
    /* fprintf(stderr, "process %d hung up on synch 2\n", par_id); */
    synch(par_id, par_count, ready);

    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C, ready, &mutex);

    /* lets test the result: */
    quadratic_matrix_multiplication(A, B, M);

    if (par_id == 0)
    {
        pthread_mutex_lock(&mutex);

        quadratic_matrix_print(M);
        quadratic_matrix_print(C);

        pthread_mutex_unlock(&mutex);
        if (quadratic_matrix_compare(C, M))
            printf("full points!\n");
        else
            printf("buuug!\n");
    }
    else
    {
        usleep(1000);
    }

    ready[par_id]++;

    /* fprintf(stderr, "process %d hung up on synch 3\n", par_id); */
    synch(par_id, par_count, ready);

    /* printf("process %d finished\n", par_id); */

    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    munmap(A, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(B, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(C, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
    munmap(ready, sizeof(int) * par_count);
    shm_unlink("matA");
    shm_unlink("matB");
    shm_unlink("matC");
    shm_unlink("syncobj");

    exit(0);
}
