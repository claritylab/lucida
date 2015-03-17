/*
 *  Copyright (c) 2015, University of Michigan.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#if !defined(MEMORYMAN_H_)
#define MEMORYMAN_H_

void* sirius_malloc(size_t size);
void sirius_free(void* ptr);

#endif /* MEMORYMAN_H_ */
