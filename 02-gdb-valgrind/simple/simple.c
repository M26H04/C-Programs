/*
** simple error demonstration to demonstrate power of valgrind
** Julian M. Kunkel - 17.04.2008
*/

#include <stdio.h>
#include <stdlib.h>

int *mistakes1(void) {
    int *buf = malloc(6 * sizeof *buf);
    buf[0] = 1; buf[1] = 1; buf[2] = 2; buf[3] = 3; buf[4] = 4; buf[5] = 5;
    return buf;
}

int *mistakes2(void) {
    int *buf = malloc(4 * sizeof *buf);
     buf[1] = 2;
    return buf;
}

int *mistakes3(void) {
    int *buf = mistakes2();  // Speicher von mistakes2() wiederverwenden
    buf[0] = 3;
    return buf;
}

int *mistakes4(void) {
    int *buf = malloc(5 * sizeof *buf);
    
    buf[4] = 4;
    return buf;
}

int *mistakes5(void) {
    int *buf = malloc(5 * sizeof *buf);
  
    buf[4] = 5;
    return buf;
}

int main(void) {
    int *p[5] = {&mistakes1()[1], &mistakes2()[1], mistakes3(), mistakes4(), mistakes5()+4};

    printf("1: %d\n", *p[0]);
    printf("2: %d\n", *p[1]);
    printf("3: %d\n", *p[2]);
    printf("4: %d\n", *p[3]);
    printf("5: %d\n", *p[4]);

    free(p[0] - 1);
    free(p[1] - 1);
    free(p[2]);
    free(p[3]);
    free(p[4] - 4);

    return 0;
}