#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    int numprocs, rank;
    int i, n = 0;
    double PI25DT = 3.141592653589793238462643;
    double pi = 0, pi2 = 0, h = 0, sum = 0, x = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while (true)
    {
        if (rank == 0){
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d", &n);
        }
        
        MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); // se envia n desde 0 (Proceso raiz) a todos los procesos dentro del comunicador global
        if (n == 0){
           break;
        }
        h = 1.0 / (double)n;
        sum = 0.0;
        for (i = rank + 1; i <= n; i += numprocs){
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x * x);
        }
        pi2 = h * sum;
        MPI_Reduce(&pi2, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0){
            printf("pi is approx. %.16f, Error: %.16f\n", pi, fabs(pi - PI25DT));
        }
    }

    MPI_Finalize();
    exit(0);
}