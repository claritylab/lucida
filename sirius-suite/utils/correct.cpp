#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>

#include "correct.h"

void write_out(char *fname, float *arr, int arr_len) {
  std::ofstream f;
  f.open(fname, std::ios::out);
  f.precision(6);

  for(int i = 0; i < arr_len; ++i)
    f << arr[i] << "\n";

  f.close();
}
