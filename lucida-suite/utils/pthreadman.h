/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef PTHREADMAN_H
#define PTHREADMAN_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

inline void sirius_pthread_create(pthread_t* thread,
                                  const pthread_attr_t* attr,
                                  void* (*start_routine) (void *),
                                  void* arg) {
  if (pthread_create(thread, attr, start_routine, arg)) {
    fprintf(stderr, "pthread_create() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_attr_init(pthread_attr_t* attr) {
  if (pthread_attr_init(attr)) {
    fprintf(stderr, "pthread_init() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_attr_getdetachstate(const pthread_attr_t* attr,
                                               int* detachstate) {
  if (pthread_attr_getdetachstate(attr, detachstate)) {
    fprintf(stderr, "pthread_getdetachstate() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_attr_setdetachstate(pthread_attr_t* attr,
                                               int detachstate) {
  if (pthread_attr_setdetachstate(attr, detachstate)) {
    fprintf(stderr, "pthread_setdetachstate() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_attr_destroy(pthread_attr_t* attr) {
  if (pthread_attr_destroy(attr)) {
    fprintf(stderr, "pthread_attr_detroy() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_join(pthread_t thread, void** retval) {
  if (pthread_join(thread, retval)) {
    fprintf(stderr, "pthread_join() failed.\n");
    exit(1);
  }
}

inline void sirius_pthread_exit(void* retval) { return pthread_exit(retval); }

#endif /* PTHREADMAN_H */
