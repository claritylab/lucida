#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M 256  /* columns */
#define N 256  /* rows */
#define THRESHOLD 100.0

int
thresh (int *in)
{
    int i;

    for (i = 0; i < N*M; i++) {
        if(in[i] > THRESHOLD)
            in[i] = 0;
    }

    return 0;
}

int main (int argc, char * argv[])
{
    int * frame;
    int i;

    printf("Rows: %d Colums: %d\n", N, M);

    srand (time (NULL));

    frame = calloc (M * N, sizeof(int));

    for (i = 0; i < N*M; i++)
        frame[i] = rand() % 255;

    thresh(frame);

    free (frame);
    return 0;
}
