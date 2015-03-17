/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <stdlib.h>
#include <stdio.h>

void* sirius_malloc(size_t size){
    void* ret = malloc(size);
    if (NULL == ret){
        fprintf(stderr, "malloc(%lu) failed\n", size);
        exit(1);
    }
    return ret;
}

void sirius_free(void* ptr){
    free(ptr);
}
