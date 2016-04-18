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
 * TODO: eh this isn't exactly mine, took it from another project and changed
 * it up. all the other files have a LICENSE header but this one didn't since
 * its just a timing utility
 *
 * @author: Johann Hauswald
 * @contact: jahausw@umich.edu
 */

#ifndef TIMER_H
#define TIMER_H

void init_timer(void);
double timer_getres(void);
void tic(void);
double toc(void);

#define STATS_INIT(NAME_, VALUE_) \
  printf("{\n");                  \
  printf("\t\"%s\":\"%s\"", NAME_, VALUE_);

#define STATS_END()  \
  do {               \
    printf("\n}\n"); \
  } while (0)

#define PRINT_STAT_INT(NAME_, VALUE_)         \
  do {                                        \
    printf(",\n\t\"%s\": %d", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_INT64(NAME_, VALUE_)        \
  do {                                         \
    printf(",\n\t\"%s\": %ld", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_INT64_ARRAY_AS_PAIR(NAME_, ARY_, LEN_)  \
  do {                                                     \
    printf(",\n\t\"%s\": [", NAME_);                       \
    for (uint64_t i = 0; i < LEN_ - 1; i++) {              \
      printf("\n\t\t[ %ld, %ld],", i, ARY_[i]);            \
    }                                                      \
    printf("\n\t\t[ %ld, %ld]", LEN_ - 1, ARY_[LEN_ - 1]); \
    printf("\n\t]");                                       \
  } while (0)

#define PRINT_STAT_DOUBLE(NAME_, VALUE_)      \
  do {                                        \
    printf(",\n\t\"%s\": %f", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_HEX64(NAME_, VALUE_)             \
  do {                                              \
    printf(",\n\t\"%s\":\"0x%lx\"", NAME_, VALUE_); \
  } while (0)

#define PRINT_STAT_STRING(NAME_, VALUE_)         \
  do {                                           \
    printf(",\n\t\"%s\":\"%s\"", NAME_, VALUE_); \
  } while (0)

#endif /* TIMER_H */
