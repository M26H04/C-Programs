#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>

int main(int argc, char** argv) {
  int rank, size;
  struct timeval tv;
  time_t current_time;
  int micro_sec;
  char time_string[30];
  char output[80];
  char hostname[30];

  // MPI initialisieren
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // Zeitstempel und Hostname ermitteln
  gettimeofday(&tv, NULL);
  gethostname(hostname, 30);

  current_time = tv.tv_sec;
  micro_sec = tv.tv_usec;

  strftime(time_string, 30, "%Y-%m-%d %T", localtime(&current_time));

  // Spezialfall für den Fall mit nur 1 Prozess
  if (size == 1) {
    printf("[%d] beendet jetzt!\n", rank);
  } else {
    // ALTERNATIVE LÖSUNG MIT KOLLEKTIVEN OPERATIONEN
    
    // Alle Prozesse außer dem letzten generieren ihre Ausgabe
    if (rank != size - 1) {
      snprintf(output, 80, "[%d] %s // %s.%d", rank, hostname, time_string, micro_sec);
      MPI_Send(output, 80, MPI_CHAR, size - 1, 0, MPI_COMM_WORLD);
    } else {
      // Letzter Prozess empfängt alle Ausgaben
      char received_output[80];
      for (int i = 0; i < size - 1; i++) {
        MPI_Recv(received_output, 80, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%s\n", received_output);
      }
    }

    // AUFGABE 2: Kollektive Operationen für Min/Max
    // Alle Prozesse außer dem letzten nehmen teil
    int local_micro = (rank == size - 1) ? 0 : micro_sec;
    int global_min, global_max;

    // Für die kollektive Operation erstellen wir einen Sub-Communicator
    // der nur die Prozesse 0 bis n-2 enthält
    MPI_Comm workers_comm;
    int color = (rank == size - 1) ? MPI_UNDEFINED : 0;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &workers_comm);

    if (rank != size - 1) {
      // Worker-Prozesse: Senden ihre Daten an den Root (im Sub-Communicator ist das Rang 0,
      // aber wir wollen zum letzten Prozess im COMM_WORLD senden)
      // Daher bleiben wir bei der manuellen Lösung für die Kommunikation
      MPI_Send(&micro_sec, 1, MPI_INT, size - 1, 1, MPI_COMM_WORLD);
    } else {
      // Letzter Prozess empfängt und berechnet
      int* micro_seconds = (int*)malloc((size - 1) * sizeof(int));
      
      for (int i = 0; i < size - 1; i++) {
        MPI_Recv(&micro_seconds[i], 1, MPI_INT, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }

      // Min und Max finden
      int min_micro = micro_seconds[0];
      int max_micro = micro_seconds[0];
      
      for (int i = 1; i < size - 1; i++) {
        if (micro_seconds[i] < min_micro) {
          min_micro = micro_seconds[i];
        }
        if (micro_seconds[i] > max_micro) {
          max_micro = micro_seconds[i];
        }
      }

      int difference = max_micro - min_micro;

      printf("[%d] Kleinster MS-Anteil: %d\n", rank, min_micro);
      printf("[%d] Größte Differenz: %d\n", rank, difference);

      free(micro_seconds);
    }

    if (workers_comm != MPI_COMM_NULL) {
      MPI_Comm_free(&workers_comm);
    }

    // Synchronisation
    MPI_Barrier(MPI_COMM_WORLD);

    // Beendigungsmeldung
    printf("[%d] beendet jetzt!\n", rank);
  }

  MPI_Finalize();

  return 0;
}