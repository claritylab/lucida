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

#if !defined(BATCH_SIZE)
#define BATCH_SIZE (30)
#endif

#define M 2000  /* columns */
#define N 2000  /* rows */
#define THRESHOLD 100.0

int
thresh (int *in, int threshold)
{
    int i;

    for (i = 0; i < N*M; ++i) {
        if(in[i] < threshold)
            in[i] = 0;
    }

    return 0;
}

int main (int argc, char * argv[])
{
    int * frame;
    int i;


    srand (time (NULL));

    frame = calloc (M * N, sizeof(int));

    for (i = 0; i < M*N; i++) {
        frame[i] = rand() % 255;
    }

    /* Perform the 2D convolution */
    thresh (&frame[M * N], THRESHOLD);

    free (frame);
    return 0;
}
