#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <stdbool.h>

int main(int argc, char *argv[])
{
    int numprocs, rank, done = 0;
    int i, n = 0;
    double PI25DT = 3.141592653589793238462643;
    double pi, h, sum, x;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while (true)
    {
        if (rank == 0)
        {
            printf("Enter the number of intervals: (0 quits) \n");
            scanf("%d", &n);   

            for (i = 1; i < numprocs; i++)
            {
                MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
        else
        {
            // los otros procesos recogen los datos introducidos
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        if(n==0)
        {
            break;
        }

        h = 1.0 / (double)n;
        sum = 0.0;
        for (i = rank + 1; i <= n; i += numprocs)
        {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x * x);
        }
        double pi2 = h * sum;
        if (rank != 0)
        {
            MPI_Send(&pi2, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
        else
        {
            pi = pi2;
            for (i = 1; i < numprocs; i++)
            {
                MPI_Recv(&pi2, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                pi += pi2;
            }
            printf("pi is approx. %.16f, Error: %.16f\n", pi, fabs(pi - PI25DT));
        }

    }

    MPI_Finalize();
}
