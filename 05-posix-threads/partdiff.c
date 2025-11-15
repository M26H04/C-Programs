/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**                 TU München - Institut für Informatik                   **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Andreas C. Schmidt                                          **/
/**                                                                        **/
/** File:      partdiff.c                                                  **/
/**                                                                        **/
/** Purpose:   Partial differential equation solver for Gauß-Seidel and    **/
/**            Jacobi method.                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* ************************************************************************ */
/* Include standard header file.                                            */
/* ************************************************************************ */
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "partdiff.h"

struct calculation_arguments {
  uint64_t N;            /* number of spaces between lines (lines=N+1)     */
  uint64_t num_matrices; /* number of matrices                             */
  double h;              /* length of a space between two lines            */
  double ***Matrix;      /* index matrix used for addressing M             */
  double *M;             /* two matrices with real values                  */
};

struct calculation_results {
  uint64_t m;
  uint64_t stat_iteration; /* number of current iteration                    */
  double stat_precision;   /* actual precision of all slaves in iteration    */
};

/* ************************************************************************ */
/* Thread data structure                                                    */
/* ************************************************************************ */
struct thread_data {
  int thread_id;
  uint64_t start_row;
  uint64_t end_row;
  struct calculation_arguments const *arguments;
  struct calculation_results *results;
  struct options const *options;
  double *maxResiduum_ptr;
  pthread_barrier_t *barrier;
  pthread_mutex_t *mutex;
  int *term_iteration_ptr;
  int *m1_ptr;
  int *m2_ptr;
};

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time; /* time when program started                      */
struct timeval comp_time;  /* time when calculation completed                */

/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static void initVariables(struct calculation_arguments *arguments,
                          struct calculation_results *results,
                          struct options const *options) {
  arguments->N = (options->interlines * 8) + 9 - 1;
  arguments->num_matrices = (options->method == METH_JACOBI) ? 2 : 1;
  arguments->h = 1.0 / arguments->N;

  results->m = 0;
  results->stat_iteration = 0;
  results->stat_precision = 0;
}

/* ************************************************************************ */
/* freeMatrices: frees memory for matrices                                  */
/* ************************************************************************ */
static void freeMatrices(struct calculation_arguments *arguments) {
  uint64_t i;

  for (i = 0; i < arguments->num_matrices; i++) {
    free(arguments->Matrix[i]);
  }

  free(arguments->Matrix);
  free(arguments->M);
}

/* ************************************************************************ */
/* allocateMemory ()                                                        */
/* allocates memory and quits if there was a memory allocation problem      */
/* ************************************************************************ */
static void *allocateMemory(size_t size) {
  void *p;

  if ((p = malloc(size)) == NULL) {
    printf("Speicherprobleme! (%" PRIu64 " Bytes angefordert)\n", (uint64_t)size);
    exit(1);
  }

  return p;
}

/* ************************************************************************ */
/* allocateMatrices: allocates memory for matrices                          */
/* ************************************************************************ */
static void allocateMatrices(struct calculation_arguments *arguments) {
  uint64_t i, j;

  uint64_t const N = arguments->N;

  arguments->M = allocateMemory(arguments->num_matrices * (N + 1) * (N + 1) *
                                sizeof(double));
  arguments->Matrix =
      allocateMemory(arguments->num_matrices * sizeof(double **));

  for (i = 0; i < arguments->num_matrices; i++) {
    arguments->Matrix[i] = allocateMemory((N + 1) * sizeof(double *));

    for (j = 0; j <= N; j++) {
      arguments->Matrix[i][j] =
          arguments->M + (i * (N + 1) * (N + 1)) + (j * (N + 1));
    }
  }
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
static void initMatrices(struct calculation_arguments *arguments,
                         struct options const *options) {
  uint64_t g, i, j; /* local variables for loops */

  uint64_t const N = arguments->N;
  double const h = arguments->h;
  double ***Matrix = arguments->Matrix;

  /* initialize matrix/matrices with zeros */
  for (g = 0; g < arguments->num_matrices; g++) {
    for (i = 0; i <= N; i++) {
      for (j = 0; j <= N; j++) {
        Matrix[g][i][j] = 0.0;
      }
    }
  }

  /* initialize borders, depending on function (function 2: nothing to do) */
  if (options->inf_func == FUNC_F0) {
    for (g = 0; g < arguments->num_matrices; g++) {
      for (i = 0; i <= N; i++) {
        Matrix[g][i][0] = 3 + (1 - (h * i)); // Linke Kante
        Matrix[g][N][i] = 3 - (h * i);       // Untere Kante
        Matrix[g][N - i][N] = 2 + h * i;     // Rechte Kante
        Matrix[g][0][N - i] = 3 + h * i;     // Obere Kante
      }
    }
  }
}

/* ************************************************************************ */
/* calculate_row_distribution: Calculate start and end row for each thread  */
/* ************************************************************************ */
static void calculate_row_distribution(uint64_t N, uint64_t num_threads,
                                       uint64_t thread_id, uint64_t *start_row,
                                       uint64_t *end_row) {
  uint64_t rows_to_compute = N - 1;
  uint64_t rows_per_thread = rows_to_compute / num_threads;
  uint64_t remainder = rows_to_compute % num_threads;

  *start_row = 1 + thread_id * rows_per_thread + (thread_id < remainder ? thread_id : remainder);
  uint64_t num_rows = rows_per_thread + (thread_id < remainder ? 1 : 0);
  *end_row = *start_row + num_rows - 1;
}

/* ************************************************************************ */
/* thread_calculate: Thread worker function for calculation                 */
/* ************************************************************************ */
static void *thread_calculate(void *arg) {
  struct thread_data *data = (struct thread_data *)arg;

  int const N = data->arguments->N;
  double const h = data->arguments->h;

  double pih = 0.0;
  double fpisin = 0.0;

  if (data->options->inf_func == FUNC_FPISIN) {
    pih = PI * h;
    fpisin = 0.25 * TWO_PI_SQUARE * h * h;
  }

  while (*(data->term_iteration_ptr) > 0) {
    double **Matrix_Out = data->arguments->Matrix[*(data->m1_ptr)];
    double **Matrix_In = data->arguments->Matrix[*(data->m2_ptr)];

    double maxResiduum = 0.0;

    /* over assigned rows */
    for (uint64_t i = data->start_row; i <= data->end_row; i++) {
      double fpisin_i = 0.0;

      if (data->options->inf_func == FUNC_FPISIN) {
        fpisin_i = fpisin * sin(pih * (double)i);
      }

      /* over all columns */
      for (int j = 1; j < N; j++) {
        double star = 0.25 * (Matrix_In[i - 1][j] + Matrix_In[i][j - 1] +
                              Matrix_In[i][j + 1] + Matrix_In[i + 1][j]);

        if (data->options->inf_func == FUNC_FPISIN) {
          star += fpisin_i * sin(pih * (double)j);
        }

        if (data->options->termination == TERM_PREC ||
            *(data->term_iteration_ptr) == 1) {
          double residuum = Matrix_In[i][j] - star;
          residuum = (residuum < 0) ? -residuum : residuum;
          maxResiduum = (residuum < maxResiduum) ? maxResiduum : residuum;
        }

        Matrix_Out[i][j] = star;
      }
    }

    /* Update global maximum residuum */
    pthread_mutex_lock(data->mutex);
    if (maxResiduum > *(data->maxResiduum_ptr)) {
      *(data->maxResiduum_ptr) = maxResiduum;
    }
    pthread_mutex_unlock(data->mutex);

    /* Wait for all threads to finish computation */
    pthread_barrier_wait(data->barrier);

    /* Only thread 0 updates global state */
    if (data->thread_id == 0) {
      data->results->stat_iteration++;
      data->results->stat_precision = *(data->maxResiduum_ptr);

      /* exchange m1 and m2 */
      int tmp = *(data->m1_ptr);
      *(data->m1_ptr) = *(data->m2_ptr);
      *(data->m2_ptr) = tmp;

      /* check for stopping calculation depending on termination method */
      if (data->options->termination == TERM_PREC) {
        if (*(data->maxResiduum_ptr) < data->options->term_precision) {
          *(data->term_iteration_ptr) = 0;
        }
      } else if (data->options->termination == TERM_ITER) {
        (*(data->term_iteration_ptr))--;
      }

      /* Reset maxResiduum for next iteration */
      *(data->maxResiduum_ptr) = 0.0;
    }

    /* Wait for thread 0 to update global state */
    pthread_barrier_wait(data->barrier);
  }

  return NULL;
}

/* ************************************************************************ */
/* calculate: solves the equation                                           */
/* ************************************************************************ */
static void calculate(struct calculation_arguments const *arguments,
                      struct calculation_results *results,
                      struct options const *options) {
  /* For Gauss-Seidel: use sequential version */
  if (options->method == METH_GAUSS_SEIDEL) {
    int i, j;           /* local variables for loops */
    int m1, m2;         /* used as indices for old and new matrices */
    double star;        /* four times center value minus 4 neigh.b values */
    double residuum;    /* residuum of current iteration */
    double maxResiduum; /* maximum residuum value of a slave in iteration */

    int const N = arguments->N;
    double const h = arguments->h;

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    m1 = 0;
    m2 = 0;

    if (options->inf_func == FUNC_FPISIN) {
      pih = PI * h;
      fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
      double **Matrix_Out = arguments->Matrix[m1];
      double **Matrix_In = arguments->Matrix[m2];

      maxResiduum = 0;

      /* over all rows */
      for (i = 1; i < N; i++) {
        double fpisin_i = 0.0;

        if (options->inf_func == FUNC_FPISIN) {
          fpisin_i = fpisin * sin(pih * (double)i);
        }

        /* over all columns */
        for (j = 1; j < N; j++) {
          star = 0.25 * (Matrix_In[i - 1][j] + Matrix_In[i][j - 1] +
                         Matrix_In[i][j + 1] + Matrix_In[i + 1][j]);

          if (options->inf_func == FUNC_FPISIN) {
            star += fpisin_i * sin(pih * (double)j);
          }

          if (options->termination == TERM_PREC || term_iteration == 1) {
            residuum = Matrix_In[i][j] - star;
            residuum = (residuum < 0) ? -residuum : residuum;
            maxResiduum = (residuum < maxResiduum) ? maxResiduum : residuum;
          }

          Matrix_Out[i][j] = star;
        }
      }

      results->stat_iteration++;
      results->stat_precision = maxResiduum;

      /* exchange m1 and m2 */
      i = m1;
      m1 = m2;
      m2 = i;

      /* check for stopping calculation depending on termination method */
      if (options->termination == TERM_PREC) {
        if (maxResiduum < options->term_precision) {
          term_iteration = 0;
        }
      } else if (options->termination == TERM_ITER) {
        term_iteration--;
      }
    }

    results->m = m2;
  }
  else{

    /* Parallel Jacobi method with POSIX threads */
    uint64_t num_threads = options->number;
    pthread_t *threads = allocateMemory(num_threads * sizeof(pthread_t));
    struct thread_data *thread_data_array =
        allocateMemory(num_threads * sizeof(struct thread_data));

    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    pthread_barrier_init(&barrier, NULL, num_threads);
    pthread_mutex_init(&mutex, NULL);

    int m1 = 0;
    int m2 = 1;
    int term_iteration = options->term_iteration;
    double maxResiduum = 0.0;

    /* Create threads */
    for (uint64_t t = 0; t < num_threads; t++) {
      thread_data_array[t].thread_id = t;
      calculate_row_distribution(arguments->N, num_threads, t,
                                  &thread_data_array[t].start_row,
                                  &thread_data_array[t].end_row);
      thread_data_array[t].arguments = arguments;
      thread_data_array[t].results = results;
      thread_data_array[t].options = options;
      thread_data_array[t].maxResiduum_ptr = &maxResiduum;
      thread_data_array[t].barrier = &barrier;
      thread_data_array[t].mutex = &mutex;
      thread_data_array[t].term_iteration_ptr = &term_iteration;
      thread_data_array[t].m1_ptr = &m1;
      thread_data_array[t].m2_ptr = &m2;

      pthread_create(&threads[t], NULL, thread_calculate, &thread_data_array[t]);
    }

    /* Wait for all threads to complete */
    for (uint64_t t = 0; t < num_threads; t++) {
      pthread_join(threads[t], NULL);
    }

    results->m = m2;

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    free(threads);
    free(thread_data_array);
  }
  return;
}

/* ************************************************************************ */
/*  displayStatistics: displays some statistics about the calculation       */
/* ************************************************************************ */
static void displayStatistics(struct calculation_arguments const *arguments,
                              struct calculation_results const *results,
                              struct options const *options) {
  int N = arguments->N;
  double time = (comp_time.tv_sec - start_time.tv_sec) +
                (comp_time.tv_usec - start_time.tv_usec) * 1e-6;

  printf("Berechnungszeit:    %f s \n", time);
  printf("Speicherbedarf:     %f MiB\n", (N + 1) * (N + 1) * sizeof(double) *
                                             arguments->num_matrices / 1024.0 /
                                             1024.0);
  printf("Berechnungsmethode: ");

  if (options->method == METH_GAUSS_SEIDEL) {
    printf("Gauß-Seidel");
  } else if (options->method == METH_JACOBI) {
    printf("Jacobi");
  }

  printf("\n");
  printf("Interlines:         %" PRIu64 "\n", options->interlines);
  printf("Stoerfunktion:      ");

  if (options->inf_func == FUNC_F0) {
    printf("f(x,y) = 0");
  } else if (options->inf_func == FUNC_FPISIN) {
    printf("f(x,y) = 2pi^2*sin(pi*x)sin(pi*y)");
  }

  printf("\n");
  printf("Terminierung:       ");

  if (options->termination == TERM_PREC) {
    printf("Hinreichende Genaugkeit");
  } else if (options->termination == TERM_ITER) {
    printf("Anzahl der Iterationen");
  }

  printf("\n");
  printf("Anzahl Iterationen: %" PRIu64 "\n", results->stat_iteration);
  printf("Norm des Fehlers:   %.11e\n", results->stat_precision);
  printf("\n");
}

/****************************************************************************/
/** Beschreibung der Funktion displayMatrix:                               **/
/**                                                                        **/
/** Die Funktion displayMatrix gibt eine Matrix                            **/
/** in einer "ubersichtlichen Art und Weise auf die Standardausgabe aus.   **/
/**                                                                        **/
/** Die "Ubersichtlichkeit wird erreicht, indem nur ein Teil der Matrix    **/
/** ausgegeben wird. Aus der Matrix werden die Randzeilen/-spalten sowie   **/
/** sieben Zwischenzeilen ausgegeben.                                      **/
/****************************************************************************/
static void displayMatrix(struct calculation_arguments *arguments,
                          struct calculation_results *results,
                          struct options *options) {
  int x, y;

  double **Matrix = arguments->Matrix[results->m];

  int const interlines = options->interlines;

  printf("Matrix:\n");

  for (y = 0; y < 9; y++) {
    for (x = 0; x < 9; x++) {
      printf("%11.8f", Matrix[y * (interlines + 1)][x * (interlines + 1)]);
    }

    printf("\n");
  }

  fflush(stdout);
}

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int main(int argc, char **argv) {
  struct options options;
  struct calculation_arguments arguments;
  struct calculation_results results;

  askParams(&options, argc, argv);

  initVariables(&arguments, &results, &options);

  allocateMatrices(&arguments);
  initMatrices(&arguments, &options);

  gettimeofday(&start_time, NULL);
  calculate(&arguments, &results, &options);
  gettimeofday(&comp_time, NULL);

  displayStatistics(&arguments, &results, &options);
  displayMatrix(&arguments, &results, &options);

  freeMatrices(&arguments);

  return 0;
}
