#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    int    myid, numprocs;
    int    namelen;
    char   processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&myid);
    MPI_Get_processor_name(processor_name,&namelen);
   

    fprintf(stdout,"Hello, world! This is MPI process %d of %d on node %s\n",
	    myid, numprocs, processor_name);
    fflush(stdout);

    MPI_Finalize();
   
    return 0;
}