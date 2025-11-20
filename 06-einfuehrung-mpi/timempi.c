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

  printf("%s\n", output);
  printf("%d\n", micro_sec);

  MPI_Finalize(); //Beendet MPI
  
  return 0;
}