/*
** simple error demonstration to demonstrate power of valgrind
** Julian M. Kunkel - 17.04.2008
*/

#include <stdio.h>
#include <stdlib.h>

int *mistakes1(void) {
  int buf[] = {1, 1, 2, 3, 4, 5};

  int *memory = malloc(sizeof(buf));  // Speicher reservieren

  // Inhalt des Arrays buf in den von memory gehaltenen Speicher laden
  int a = sizeof(buf) / sizeof(int);
  for (int n = 0; n < a; n ++) { memory[n] = buf[n]; }

  return memory; // entspricht: return &memory[0];
}

int *mistakes2(void) {
  int *buf = malloc(sizeof(int) * 4);
  buf[1] = 2;
  return buf;
}

int *mistakes3(void) {
  /* In dieser Funktion darf kein Speicher direkt d.h. explizit allokiert werden. */
  int mistakes2_ = 2;
  int *buf = mistakes2();
  buf[mistakes2_] = 3;
  return buf + mistakes2_; // alternativ: return &buf[mistakes2_]
}

int *mistakes4(void) {
  int *buf = malloc(sizeof(int) * 5);
  buf[4] = 4;
  return &buf[4]; // alternativ: return buf + 4;
}

int *mistakes5(void) {
  int *buf = malloc(sizeof(int) * 5);
  buf[4] = 5;
  return buf;
}

int main(void) {
  /* Diese Zeile darf NICHT verändert werden! */
  int *p[5] = {&mistakes1()[1], &mistakes2()[1], mistakes3(), mistakes4(), mistakes5()+4};

  /* Die printf-Aufrufe dürfen NICHT verändert werden*/
  printf("1: %d\n", *p[0]);
  printf("2: %d\n", *p[1]);
  printf("3: %d\n", *p[2]);
  printf("4: %d\n", *p[3]);
  printf("5: %d\n", *p[4]);

  /* TODO */
  /* Fügen sie hier die korrekten aufrufe von free() ein */
  free(p[0] - 1);
  free(p[1] - 1);
  free(p[2] - 2);
  free(p[3] - 4);
  free(p[4] - 4);

  return 0;
}
