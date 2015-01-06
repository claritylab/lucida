/*
 *      Logging utility.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

#include <os.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <crfsuite.h>
#include "logging.h"

void logging(logging_t* lg, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if (lg != NULL && lg->func != NULL) {
        lg->func(lg->instance, format, args);
    }
}

void logging_timestamp(logging_t* lg, const char *format)
{
    time_t ts;
    char timestamp[80];

    time(&ts);
    strftime(
        timestamp, sizeof(timestamp),
        "%Y-%m-%dT%H:%M:%SZ",
        gmtime(&ts)
        );
    logging(lg, format, timestamp);
}

void logging_progress_start(logging_t* lg)
{
    lg->percent = 0;
    logging(lg, "0");
}

void logging_progress(logging_t* lg, int percent)
{
    while (lg->percent < percent) {
        ++lg->percent;
        if (lg->percent % 2 == 0) {
            if (lg->percent % 10 == 0) {
                logging(lg, "%d", lg->percent / 10);
            } else {
                logging(lg, ".");
            }
        }
    }
}

void logging_progress_end(logging_t* lg)
{
    logging_progress(lg, 100);
    logging(lg, "\n");
}
