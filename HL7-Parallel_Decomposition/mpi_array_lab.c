#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define  MASTER        0

float *data = {NULL};

int   numtasks, taskid, rc, dest, offset, i, j, tag1,
      tag2, source, chunksize;
float mysum, sum;
float update(int myoffset, int chunk, int myid);
MPI_Status status;



void initialization(int argc, char **argv, int size) {
//    printf("%d", argc);
    
    MPI_Init(&argc, &argv);                     // MPI_Init executes it causes the creation of the number of process
                                                // Number of processes is based on user input (np)
                                                // Each of the processes executes their own separate versions of the progr
                                                // MPI assigns an integer to each process beginning with 0 (rank)
                                                
    /* find out how many processes were started. */
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    
    if (numtasks % 4 != 0) {
       printf("Quitting. Number of MPI tasks must be divisible by 4.\n");
       MPI_Abort(MPI_COMM_WORLD, rc);
       exit(0);
    }
    
    /* find out the process ID */
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    
    printf ("MPI task %d has started...\n", taskid);
    
    chunksize = (size / numtasks);
    tag2 = 1;
    tag1 = 2;
}

void masterTask(int size) {
    if (taskid == MASTER){

    /* Initialize the array */
    sum = 0;
    for(i=0; i<size; i++) {
      data[i] =  i * 1.0;
      sum = sum + data[i];
      }
    printf("Initialized array sum = %e\n",sum);

    /* Send each task its portion of the array - master keeps 1st part */
    offset = chunksize;
    for (dest=1; dest<numtasks; dest++) {
      MPI_Send(&offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
      MPI_Send(&data[offset], chunksize, MPI_FLOAT, dest, tag2, MPI_COMM_WORLD);
      printf("Sent %d elements to task %d offset= %d\n",chunksize,dest,offset);
      offset = offset + chunksize;
      }

    /* Master does its part of the work */
    offset = 0;
    mysum = update(offset, chunksize, taskid);

    /* Wait to receive results from each task */
    for (i=1; i<numtasks; i++) {
      source = i;
      MPI_Recv(&offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, &status);
      MPI_Recv(&data[offset], chunksize, MPI_FLOAT, source, tag2,
        MPI_COMM_WORLD, &status);
      }

    /* Get final sum and print sample results */
    MPI_Reduce(&mysum, &sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);
    printf("Sample results: \n");
    offset = 0;
    for (i=0; i<numtasks; i++) {
      for (j=0; j<5; j++)
        printf("  %e",data[offset+j]);
      printf("\n");
      offset = offset + chunksize;
      }
    printf("*** Final sum= %e ***\n",sum);

    }  /* end of master section */
}

void nonMasterTask() {
    if (taskid > MASTER) {

    /* Receive my portion of array from the master task */
    source = MASTER;
    MPI_Recv(&offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, &status);
    MPI_Recv(&data[offset], chunksize, MPI_FLOAT, source, tag2,
      MPI_COMM_WORLD, &status);

    mysum = update(offset, chunksize, taskid);

    /* Send my results back to the master task */
    dest = MASTER;
    MPI_Send(&offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
    MPI_Send(&data[offset], chunksize, MPI_FLOAT, MASTER, tag2, MPI_COMM_WORLD);

    MPI_Reduce(&mysum, &sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);

    } /* end of non-master */
}

float update(int myoffset, int chunk, int myid) {
  int i;
  float mysum;
  /* Perform addition to each of my array elements and keep my sum */
  mysum = 0;
  for(i=myoffset; i < myoffset + chunk; i++) {
    data[i] = data[i] + i * 1.0;
    mysum = mysum + data[i];
    }
  printf("Task %d mysum = %e\n",myid,mysum);
  return(mysum);
}


int main (int argc, char *argv[])
{
    int size = atoi(argv[1]);
    
    // data array size dynamic
    data = (float *) malloc(size * sizeof(float));

    /**Initializations**/
    initialization(argc, argv, size);

    /**Master task only**/
    masterTask(argc);

    /**Non-master tasks only**/
    nonMasterTask();

    MPI_Finalize();

}   /* end of main */

