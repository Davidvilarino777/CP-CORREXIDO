#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stdbool.h>

int MPI_FlattreeColectiva(void * sendbuff, void *recvbuff, int count, MPI_Datatype datatype, MPI_Op op,int root, MPI_Comm comm){
    int numprocs,rank;
    int i;
    double n,totalCount;
    int ERROR;
    
    MPI_Comm_size(comm,&numprocs);
    MPI_Comm_rank(comm,&rank);

    if(op != MPI_SUM){
        MPI_ERR_OP;
    }
    if(datatype != MPI_DOUBLE){
        MPI_ERR_TYPE;
    }

    if(rank==root){
        totalCount = *(double *) sendbuff;
        for(i=0;i<numprocs;i++){
            if(i!=root){
                if (ERROR=MPI_Recv(&n,count,datatype,MPI_ANY_SOURCE,0,comm,MPI_STATUS_IGNORE)!=MPI_SUCCESS)//El proceso 0 recibe el contador del resto de procesos 
                    return ERROR;
                totalCount+= n;   
            }
        }
        *(double *) recvbuff = totalCount; 
    }
    else {
      //printf("el proceso %d envia su contador al proceso 0\n",rank);
        if (ERROR=MPI_Send(sendbuff, count,datatype,root,0,comm)!=MPI_SUCCESS)
            return ERROR;
    }
    return MPI_SUCCESS;
}

int MPI_BinomialColectiva(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int i = 1, mask = 0x1;
    int rank, numprocs;
    int ERROR;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &numprocs);

    if(root!=0 )
      return MPI_ERR_ROOT;

    while (mask < numprocs) {

        if (rank < mask && (rank + mask < numprocs)) {
            //printf("el proceso %d envia al proceso %d\n",rank,rank+mask);
            if (ERROR=MPI_Send(buf, count,datatype,(rank + mask),0,comm)!=MPI_SUCCESS)
                return ERROR;
            

        } else if (rank >= mask && rank < mask << 1) {
            //printf("el proceso %d recibe del proceso %d\n",rank,rank-mask);
            if (ERROR=MPI_Recv(buf,count,datatype,(rank - mask),0,comm,MPI_STATUS_IGNORE)!=MPI_SUCCESS)//El proceso 0 recibe el contador del resto de procesos 
                    return ERROR;
            
        }

        mask <<= 1;
    }
    return MPI_SUCCESS;
}

int main(int argc, char *argv[]) {
    int numprocs,rank;
    int i, n=0, ERROR;
    double PI25DT = 3.141592653589793238462643;
    double pi=0, pi2=0, h=0, sum=0, x=0;
    
    MPI_Init(&argc,&argv); 
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank); 
    while(true){
        if(rank==0){  
                printf("Enter the number of intervals: (0 quits) \n");
                scanf("%d",&n);
        }

        if (ERROR=MPI_BinomialColectiva(&n,1,MPI_INT,0,MPI_COMM_WORLD) != MPI_SUCCESS) {   //Se suman todos los contadores locales.
            printf("%d\n",ERROR);
            break;
        }

        if (n == 0){
           break;
        }

        h = 1.0 / (double) n;
        sum = 0.0;
        for (i = rank+1; i <= n; i+=numprocs) {
            x = h * ((double)i - 0.5);
            sum += 4.0 / (1.0 + x*x);
        }
        pi2 = h * sum;
        if (ERROR=MPI_FlattreeColectiva(&pi2, &pi, 1, MPI_DOUBLE,MPI_SUM, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {   //Se suman todos los contadores locales.
            printf("%d\n",ERROR);
            break;
        }
        else if (rank == 0) {   
            printf("pi is approx. %.16f, Error: %.16f\n", pi, fabs(pi - PI25DT));
        }
    }   
    MPI_Finalize();
    exit(0);
}