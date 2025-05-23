#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1
#define N 555

int main(int argc, char *argv[]) {

    int i, j, filas, numprocs, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // Number of rows for each process
    filas = (N + numprocs -1) / numprocs;

    // Allocate correct memory for future scatter
    float matrix[filas*numprocs][N];
    float vector[N];
    float result[N];
    struct timeval tv1, tv2;


    /* Initialize Matrix and Vector */
    if (rank == 0) {
        for (i = 0; i < N; i++) {
            vector[i] = i;
            for (j = 0; j < N; j++) {
                matrix[i][j] = i + j;
            }
        }
    }


    float localMatrix[filas][N];
    float localResult[filas];

    gettimeofday(&tv1, NULL);

    // Scatter matrix to each process
    MPI_Scatter(matrix, N * filas, MPI_FLOAT, localMatrix, N * filas, MPI_FLOAT, 0, MPI_COMM_WORLD);
    // Broadcast vector to all processes
    MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
    gettimeofday(&tv2, NULL);
    int microseconds_comm = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);
    gettimeofday(&tv1, NULL);

    // Correct number of rows in (numprocs -1)
    if (rank == numprocs - 1) {
        filas = N - filas*(numprocs-1);
    }

    // Compute local result
    for (i = 0; i < filas; i++) {
        localResult[i] = 0;
        
        for (j = 0; j < N; j++) {
            localResult[i] += localMatrix[i][j] * vector[j];      
        }
        
    }
    gettimeofday(&tv2, NULL);
    int microseconds_comp = (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

    gettimeofday(&tv1, NULL);

    // Gather results to the root process
    MPI_Gather(localResult, filas, MPI_FLOAT, result, filas, MPI_FLOAT, 0, MPI_COMM_WORLD);
    gettimeofday(&tv2, NULL);
    microseconds_comm += (tv2.tv_usec - tv1.tv_usec) + 1000000 * (tv2.tv_sec - tv1.tv_sec);

   

    /* Display result */
    if (DEBUG) {
        if (rank == 0) {
            for (i = 0; i < N; i++) {
                printf(" %f \t ", result[i]);
            }
            printf("\n");
        }
    } else {
        printf("Communication Time (seconds) = %lf\n", (double)microseconds_comm / 1E6);
        printf("Computation Time (seconds) = %lf\n", (double)microseconds_comp / 1E6);
    }

    MPI_Finalize();
    return 0;
}