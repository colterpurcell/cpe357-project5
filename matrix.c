#define _DEFAULT_SOURCE
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
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
    printf("\n");
    for (a = 0; a < MATRIX_DIMENSION_XY; a++)
    {
        printf("\n");
        for (b = 0; b < MATRIX_DIMENSION_XY; b++)
            printf("%.2f,", C[a + b * MATRIX_DIMENSION_XY]);
    }
    printf("\n");
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
     * use the shared memory object "ready" for this. it is an array of ints. each process
     * sets its own entry to 1. if all entries are 1, all processes are here. if not, the
     * process with the lowest ID sets its entry to 0 and waits for a second. then it checks
     * again.
     */
    int i;
    int allready = 0;
    while (allready == 0)
    {
        allready = 1;
        for (i = 0; i < par_count; i++)
        {
            if (ready[i] == 0)
            {
                allready = 0;
                break;
            }
        }
        if (allready == 0)
        {
            ready[par_id] = 0;
            usleep(10);
        }
    }
    for (i = 0; i < par_count; i++)
    {
        ready[i] = 0;
    }
}

void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float *A, float *B, float *C, int *ready)
{
    int x, y, c;
    int start = par_id * MATRIX_DIMENSION_XY / par_count;
    int end = (par_id + 1) * MATRIX_DIMENSION_XY / par_count;
    /* nullify the result matrix first */
    for (x = 0; x < MATRIX_DIMENSION_XY; x++)
        for (y = 0; y < MATRIX_DIMENSION_XY; y++)
            C[x + y * MATRIX_DIMENSION_XY] = 0.0;
    /* multiply */
    for (x = start; x < end; x++)                     /* over all cols a */
        for (y = 0; y < MATRIX_DIMENSION_XY; y++)     /* over all rows b */
            for (c = 0; c < MATRIX_DIMENSION_XY; c++) /* over all rows/cols left */
            {
                C[x + y * MATRIX_DIMENSION_XY] += A[c + y * MATRIX_DIMENSION_XY] * B[x + c * MATRIX_DIMENSION_XY];
            }

    ready[par_id] = 1;
}

/************************************************************************************************************************* */
int main(int argc, char *argv[])
{
    int i;
    int par_id = 0;    /* the parallel ID of this process */
    int par_count = 1; /* the amount of processes */
    float *A, *B, *C;  /* matrices A,B and C */
    int *ready;        /* needed for synch */
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];

    par_id = atoi(argv[1]);
    par_count = atoi(argv[2]);
    /* strcpy(shared_mem_matrix,argv[3]); */

    if (par_count == 1)
    {
        printf("only one process\n");
    }

    A = (float *)argv[3];
    B = (float *)argv[4];
    C = (float *)argv[5];
    ready = (int *)argv[6];

    ready[par_id] = 1;

    printf("all processes here\n");
    printf("%d here %p\n", par_id + 1, (void *)ready);
    for (i = 0; i < par_count; i++)
        printf("ready[%d]=%d\n", i, ready[i]);
    synch(par_id, par_count, ready);

    if (par_id == 0)
    {
        /* TODO: initialize the matrices A and B */
        int x, y;
        for (x = 0; x < MATRIX_DIMENSION_XY; x++)
            for (y = 0; y < MATRIX_DIMENSION_XY; y++)
            {
                set_matrix_elem(A, x, y, x + y);
                set_matrix_elem(B, x, y, x + y);
            }
    }

    synch(par_id, par_count, ready);

    /* TODO: quadratic_matrix_multiplication_parallel(par_id, par_count,A,B,C, ...); */
    /*
     * Take a portion of the rows (not columns) and multiply the matrices A and B, placing the yield into matrix C.
     * When finished, mark as so in the ready structure
     */
    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C, ready);

    synch(par_id, par_count, ready);

    if (par_id == 0)
        quadratic_matrix_print(C);
    synch(par_id, par_count, ready);

    /* lets test the result: */
    quadratic_matrix_multiplication(A, B, M);
    if (quadratic_matrix_compare(C, M))
        printf("full points!\n");
    else
        printf("buuug!\n");
    if (par_id == 0)
        quadratic_matrix_print(M);

    synch(par_id, par_count, ready);
    shm_unlink("matrixA");
    shm_unlink("matrixB");
    shm_unlink("matrixC");
    shm_unlink("synchobject");

    return 0;
}
