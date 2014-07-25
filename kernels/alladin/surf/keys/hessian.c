/**************************/
/***    UNCLASSIFIED    ***/
/**************************/

/***

  ALL SOURCE CODE PRESENT IN THIS FILE IS UNCLASSIFIED AND IS
  BEING PROVIDED IN SUPPORT OF THE DARPA PERFECT PROGRAM.

  THIS CODE IS PROVIDED AS-IS WITH NO WARRANTY, EXPRESSED, IMPLIED, 
  OR OTHERWISE INFERRED. USE AND SUITABILITY FOR ANY PARTICULAR
  APPLICATION IS SOLELY THE RESPONSIBILITY OF THE IMPLEMENTER. 
  NO CLAIM OF SUITABILITY FOR ANY APPLICATION IS MADE.
  USE OF THIS CODE FOR ANY APPLICATION RELEASES THE AUTHOR
  AND THE US GOVT OF ANY AND ALL LIABILITY.

  THE POINT OF CONTACT FOR QUESTIONS REGARDING THIS SOFTWARE IS:

  US ARMY RDECOM CERDEC NVESD, RDER-NVS-SI (JOHN HODAPP), 
  10221 BURBECK RD, FORT BELVOIR, VA 22060-5806

  THIS HEADER SHALL REMAIN PART OF ALL SOURCE CODE FILES.

 ***/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M 2000  /* columns */
#define N 2000  /* rows */

void
hessian (float *I_steepest, int nCols, int nRows, int np, float *H)
{
    int i, j;
    int x, y;

    for (y = 0; y < nRows; y++) {
        for (i = 0; i < np; i++) {
            for (j = 0; j < np; j++) {
                float total = 0.0;
                for (x = 0; x < nCols; x++) {
                    int index1 = (6 * y * nCols) + (nCols * i) + x;
                    int index2 = (6 * y * nCols) + (nCols * j) + x;
                    total += I_steepest[index1] * I_steepest[index2];
                }
                H[6*i + j] += total;
            }
        }
    }
}

int main (int argc, char * argv[])
{
    int hessianThreshold = 100;
    int nOctaves = 4;
    int nOctaveLayers = 3;
    /* Steepest descent */
    float * I_steepest;
    /* Hessian */
    float * H;
    int y;
    printf("Rows: %d Colums: %d\n", N, M);

    I_steepest = calloc (6 * M * N, sizeof(float));
    H = calloc (6 * 6, sizeof(float));

    for (y = 0; y < 6 * M *N; ++y) {
        I_steepest[y] = rand() % 10000;
    }

    /* Compute the Hessian matrix */
    hessian (I_steepest, M, N, 6, H);

    free (I_steepest);
    free (H);

    return 0;
}
