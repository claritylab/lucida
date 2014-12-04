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

#define M 512 /* columns */
#define N 512 /* rows */

int
conv2d (int *in, int *out, int nRows, int nCols, float *filter, float normFactor, int nFilterRows, int nFilterCols)
{
    float sum = 0.0;
    int m = 0, n = 0;
    int i = 0, j = 0;
    int row = 0, col = 0;
    int rowOffset = nFilterRows / 2;
    int colOffset = nFilterCols / 2;
    int rowBegIndex = rowOffset;
    int colBegIndex = colOffset;
    int pxlPos = 0;
    int fltPos = 0;

    int *tmpBuf = (int *)malloc((nRows + nFilterRows) * (nCols + nFilterCols) * sizeof(int));

    for (row = 0; row < nRows; row++)
    {
        {
            memcpy((void *)(tmpBuf + (row + rowOffset) * (nCols + nFilterCols) + colOffset), 
                    (void *)(in + row * nCols), 
                    nCols * sizeof(int));
        }
    }

    for (row = rowBegIndex; row < nRows + rowOffset; row++)
    {
        for (col = colBegIndex; col < nCols + colOffset; col++)
        {
            sum = 0;
            m = 0;
            for (i = row - rowOffset; i <= row + rowOffset; i++)
            {
                n = 0;
                for (j = col - colOffset; j <= col + colOffset; j++)
                {
                    pxlPos = i * (nCols + nFilterCols) + j;
                    fltPos = m * nFilterCols + n;
                    sum += ((float) tmpBuf[pxlPos] * filter[fltPos]);
                    n++;
                }
                m++;
            }
            out[(row - rowBegIndex) * nCols + (col - colBegIndex)] = (int) (sum / normFactor);
        }
    }

    free((void *)tmpBuf);

    return 0;
}

int main (int argc, char * argv[])
{
    int * frame;
    int * output;
    int i;

    int nFilterRowsFD = 9; 
    int nFilterColsFD = 9;

    float FD[] =  {
        1,   3,   4,   5,   6,   5,  4,    3,  1,
        3,   9,  12,  15,  18,  15,  12,   9,  3,
        4,  12,  16,  20,  24,  20,  16,  12,  4,
        5,  15,  20,  25,  30,  25,  20,  15,  5,
        6,  18,  24,  30,  36,  30,  24,  18,  6,
        5,  15,  20,  25,  30,  25,  20,  15,  5,
        4,  12,  16,  20,  24,  20,  16,  12,  4,
        3,   9,  12,  15,  18,  15,  12,   9,  3,
        1,   3,   4,   5,   6,   5,   4,   3,  1
    };

    for (i = 0; i < nFilterRowsFD * nFilterColsFD; i++)
    {
        FD[i] /= (1024.0);
    }

    srand (time (NULL));

    frame = calloc (M * N, sizeof(int));
    output = calloc (M * N, sizeof(int));

    for (i = 0; i < M*N; i++) {
        frame[i] = rand() % 255;
    }

    /* Perform the 2D convolution */
    printf("Rows: %d Colums: %d\n", N, M);
    conv2d (frame, output, N, M, FD, 1.0, nFilterRowsFD, nFilterColsFD);

    free (output);
    free (frame);
    return 0;
}
