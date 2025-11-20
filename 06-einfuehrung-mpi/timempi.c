#define _DEFAULT_SOURCE
// mpirun -np=4 ~/Uebungen/6/timempi.x
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h> // Nutzung von MPI

int main(int argc, char *argv[]){

  //initialisiert die parallelen Prozesse und gibt jedem einen Rang(rank).
  int size, rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size); // Gesamtzahl der Prozesse
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Speicher den eigenen Rang

  struct timeval tv;
  time_t current_time;
  int micro_sec;
  char time_string[30];
  char output[80];
  char hostname[30];

  gettimeofday(&tv, NULL);
  gethostname(hostname, 30);

  current_time = tv.tv_sec;
  micro_sec = tv.tv_usec;

  strftime(time_string, 30, "%Y-%m-%d %T", localtime(&current_time));
  snprintf(output, 80, "[%d] %s // %s.%d", rank, hostname, time_string, micro_sec);

  // Send and Receive messages
  if(size == 1){
    printf("%s\n", output);
    printf("[%d] Kleinster MS-Anteil: %d\n", rank, micro_sec);
    printf("[%d] Größte Differenz:: 0\n", rank);
  }
  else if(rank < size-1){
    MPI_Send(&output, 80, MPI_CHAR, size-1, rank, MPI_COMM_WORLD); // value, amount, type, destination, tag, com
    MPI_Send(&micro_sec, 1, MPI_INT, size-1, rank+1000, MPI_COMM_WORLD);
  }
  else if (rank == size-1){
    char outP[80];
    int ms[size-1];

    for(int i = 0; i < size-1; i++){
      MPI_Recv(outP, 80, MPI_CHAR, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // var, amount, type, source, tag, com, status
      MPI_Recv(&ms[i], 1, MPI_INT, i, i+1000, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      printf("%s\n", outP);
    }

    // Berechene Zeitunterschiede
    int msMax = ms[0];
    int msMin = ms[0];
    for(int i = 1; i < size-1; i++){
      if(ms[i] < msMin){
        msMin = ms[i];
      }
      else if(ms[i] > msMax){
        msMax = ms[i];
      }
    }

    int diff = msMax - msMin;
    printf("[%d] Kleinster MS-Anteil: %d\n", rank, msMin);
    printf("[%d] Größte Differenz:: %d\n", rank, diff);
  }

  MPI_Barrier(MPI_COMM_WORLD);            // Warte auf alle Prozesse (Funktioniert nicht für printf?)
  printf("[%d] beendet jetzt!\n", rank);  // Schreibe Prozess-Ende

  MPI_Finalize();                         //Beendet MPI
  return 0;
}