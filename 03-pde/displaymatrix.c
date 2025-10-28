/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  	      	   TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Dr. Thomas Ludwig                                           **/
/**            Thomas A. Zochler                                           **/
/**            Andreas C. Schmidt                                          **/
/**                                                                        **/
/** File:      displaymatrix.c                                             **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/** Beschreibung der Funktion DisplayMatrix:                               **/
/**                                                                        **/
/** Die Funktion DisplayMatrix gibt eine Matrix ( double v[...] )          **/
/** in einer "ubersichtlichen Art und Weise auf die Standardausgabe aus.   **/
/**                                                                        **/
/** Die "Ubersichtlichkeit wird erreicht, indem nur ein Teil der Matrix    **/
/** ausgegeben wird. Aus der Matrix werden die Randzeilen/-spalten sowie   **/
/** sieben Zwischenzeilen ausgegeben.                                      **/
/**                                                                        **/
/** Die Funktion erwartet einen Zeiger auf ein lineares Feld, in dem       **/
/** die Matrixeintraege wie in einer zweidimensionalen Matrix abgelegt     **/
/** sind.                                                                  **/
/**                                                                        **/
/****************************************************************************/

/****************************************************************************/
/** Beschreibung der Funktion DisplayMatrixAddr:                           **/
/**                                                                        **/
/** Gleiche Funktionalitaet wie DisplayMatrix, mit dem einzigen Unter-	   **/
/** schied, dass ein Dreifachpointer auf die Matrix erwartet wird, und	   **/
/** somit mittels Addressrechnung auf die Matrix zugegriffen wird.         **/
/**                                                                        **/
/** Ein zusaetzlicher Parameter gibt an, auf welche der zweidimensionalen  **/
/** Matrizen zugegriffen werden soll.                                      **/
/****************************************************************************/

#include "partdiff-seq.h"
#include <stdio.h>

void DisplayMatrix(char* s, double* v, int interlines)
{
    int x, y;
    int lines = 8 * interlines + 9;

    printf("%s\n", s);
    for (y = 0; y < 9; y++) {
        for (x = 0; x < 9; x++) {
            printf("%11.8f", v[y * (interlines + 1) * lines + x * (interlines + 1)]);
        }
        printf("\n");
    }
    fflush(stdout);
}