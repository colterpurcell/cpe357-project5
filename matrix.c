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

// SEARCH FOR TODO

//************************************************************************************************************************
// sets one element of the matrix
void set_matrix_elem(float *M, int x, int y, float f)
{
    M[x + y * MATRIX_DIMENSION_XY] = f;
}
//************************************************************************************************************************
// lets see it both are the same
int quadratic_matrix_compare(float *A, float *B)
{
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            if (A[a + b * MATRIX_DIMENSION_XY] != B[a + b * MATRIX_DIMENSION_XY])
                return 0;

    return 1;
}
//************************************************************************************************************************
// print a matrix
void quadratic_matrix_print(float *C)
{
    printf("\n");
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
    {
        printf("\n");
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            printf("%.2f,", C[a + b * MATRIX_DIMENSION_XY]);
    }
    printf("\n");
}
//************************************************************************************************************************
// multiply two matrices
void quadratic_matrix_multiplication(float *A, float *B, float *C)
{
    // nullify the result matrix first
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            C[a + b * MATRIX_DIMENSION_XY] = 0.0;
    // multiply
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)         // over all cols a
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)     // over all rows b
            for (int c = 0; c < MATRIX_DIMENSION_XY; c++) // over all rows/cols left
            {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
}
//************************************************************************************************************************
void synch(int par_id, int par_count, int *ready)
{
    /* TODO: synch algorithm. make sure, ALL processes get stuck here until all ARE here
     * use the shared memory object "ready" for this. it is an array of ints. each process
     * sets its own entry to 1. if all entries are 1, all processes are here. if not, the
     * process with the lowest ID sets its entry to 0 and waits for a second. then it checks
     * again.
     */
    if (par_id == 0)
    {
        int i;
        for (i = 0; i < par_count; i++)
        {
            ready[i] = 1;
        }
    }
    else
    {
        ready[par_id] = 1;
        while (ready[0] == 0)
        {
            ready[par_id] = 0;
            usleep(10);
        }
    }
}

void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float *A, float *B, float *C, int *ready)
{
    int x, y, z;
    int start = par_id * MATRIX_DIMENSION_XY / par_count;
    int end = (par_id + 1) * MATRIX_DIMENSION_XY / par_count;
    for (x = start; x < end; x++)
    {
        for (y = 0; y < MATRIX_DIMENSION_XY; y++)
        {
            float sum = 0;
            for (z = 0; z < MATRIX_DIMENSION_XY; z++)
            {
                sum += get_matrix_elem(A, x, z) * get_matrix_elem(B, z, y);
            }
            set_matrix_elem(C, x, y, sum);
        }
    }
    ready[par_id] = 1;
}

//************************************************************************************************************************
int main(int argc, char *argv[])
{
    int par_id = 0;    // the parallel ID of this process
    int par_count = 1; // the amount of processes
    float *A, *B, *C;  // matrices A,B and C
    int *ready;        // needed for synch
    if (argc != 3)
    {
        printf("no shared\n");
    }
    else
    {
        par_id = atoi(argv[1]);
        par_count = atoi(argv[2]);
        // strcpy(shared_mem_matrix,argv[3]);
    }
    if (par_count == 1)
    {
        printf("only one process\n");
    }

    int fd[4];
    if (par_id == 0)
    {
        // TODO: init the shared memory for A,B,C, ready. shm_open with C_CREAT here! then ftruncate! then mmap
        fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, 0777);
        fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, 0777);
        fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, 0777);
        fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, 0777);
        ftruncate(fd[0], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[1], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[2], sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY);
        ftruncate(fd[3], sizeof(int) * par_count);
        A = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = mmap(NULL, sizeof(int) * par_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    }
    else
    {
        // TODO: init the shared memory for A,B,C, ready. shm_open withOUT C_CREAT here! NO ftruncate! but yes to mmap
        sleep(2); // needed for initalizing synch
        fd[0] = shm_open("matrixA", O_RDWR, 0777);
        fd[1] = shm_open("matrixB", O_RDWR, 0777);
        fd[2] = shm_open("matrixC", O_RDWR, 0777);
        fd[3] = shm_open("synchobject", O_RDWR, 0777);
        A = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = mmap(NULL, sizeof(float) * MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY, PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = mmap(NULL, sizeof(int) * par_count, PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    }

    synch(par_id, par_count, ready);

    if (par_id == 0)
    {
        // TODO: initialize the matrices A and B
        int x, y;
        for (x = 0; x < MATRIX_DIMENSION_XY; x++)
            for (y = 0; y < MATRIX_DIMENSION_XY; y++)
            {
                set_matrix_elem(A, x, y, x + y);
                set_matrix_elem(B, x, y, x + y);
            }
    }

    synch(par_id, par_count, ready);

    // TODO: quadratic_matrix_multiplication_parallel(par_id, par_count,A,B,C, ...);
    /*
     * Take a portion of the rows (not columns) and multiply the matrices A and B, placing the yield into matrix C.
     * When finished, mark as so in the ready structure
     */
    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C, ready);

    synch(par_id, par_count, ready);

    if (par_id == 0)
        quadratic_matrix_print(C);
    synch(par_id, par_count, ready);

    // lets test the result:
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    quadratic_matrix_multiplication(A, B, M);
    if (quadratic_matrix_compare(C, M))
        print("full points!\n");
    else
        print("buuug!\n");

    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    shm_unlink("matrixA");
    shm_unlink("matrixB");
    shm_unlink("matrixC");
    shm_unlink("synchobject");

    return 0;
}