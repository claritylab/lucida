/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * TODO:
 *
 * @author: Johann Hauswald
 * @contact: jahausw@umich.edu
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "timer.h"
struct timeval t1, t2;

/**
* @brief Start the timer
*/
void tic(void) { gettimeofday(&t1, NULL); }

/**
* @brief Stop the timer and return the time taken.
* values returned in ms.
* @return Time since last tic()
*/
double toc(void) {
  gettimeofday(&t2, NULL);
  double elapsedTime;
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;

  return elapsedTime;
}
