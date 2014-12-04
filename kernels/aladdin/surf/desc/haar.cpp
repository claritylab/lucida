#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#define M 128 /* columns */
#define N 128 /* rows */

/** A Modified version of 1D Haar Transform, used by the 2D Haar Transform function **/
void haar1d(float *vec, int n, int w)
{
	float *vecp = new float[n];
	for(int i = 0; i < n; ++i)
		vecp[i] = 0;

		w/=2;
		for(int i=0;i<w;i++)
		{
			vecp[i] = (vec[2*i] + vec[2*i+1])/sqrt(2.0);
			vecp[i+w] = (vec[2*i] - vec[2*i+1])/sqrt(2.0);
		}
		
		for(int i=0;i<(w*2);i++)
			vec[i] = vecp[i];

		delete [] vecp;
}

/** The 2D Haar Transform **/
void haar2d(float **matrix, int rows, int cols)
{
	float *temp_row = new float[cols];
	float *temp_col = new float[rows];

	int i=0,j=0;
	int w = cols, h=rows;
	while(w>1 || h>1)
	{
		if(w>1)
		{
			for(i=0;i<h;i++)
			{
				for(j=0;j<cols;j++)
					temp_row[j] = matrix[i][j];

				haar1d(temp_row,cols,w);
				
				for(j=0;j<cols;j++)
					matrix[i][j] = temp_row[j];
			}
		}

		if(h>1)
		{
			for(i=0;i<w;i++)
			{
				for(j=0;j<rows;j++)
					temp_col[j] = matrix[j][i];
				haar1d(temp_col, rows, h);
				for(j=0;j<rows;j++)
					matrix[j][i] = temp_col[j];
			}
		}

		if(w>1)
			w/=2;
		if(h>1)
			h/=2;
	}

	delete [] temp_row;
	delete [] temp_col;
}

/** Here's an example on how to use these functions **/
int main(int argc, char **argv)
{
	float **frame = new float*[N];
	for(int i = 0; i < N; ++i)
		frame[i] = new float[M];

    for (int i = 0; i < N; ++i){
        for (int j = 0; j < M; ++j)
            frame[i][j] = rand() % 255;
    }

    printf("Rows: %d Colums: %d\n", N, M);
	haar2d(frame, N, M);

	for(int i = 0; i < N; ++i)
		delete [] frame[i];

	delete [] frame;

	return 0;
}
