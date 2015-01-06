/*
 *        A parser for command-line options.
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
#include <stdlib.h>
#include <string.h>
#include "option.h"

int option_parse(char * const argv[], int num_argv, option_handler_t handler, void *instance)
{
    int i;

    for (i = 0;i < num_argv;++i) {
        const char *token = argv[i];
        if (*token++ == '-') {
            int ret = 0;
            const char *next_token = (i+1 < num_argv) ? argv[i+1] : "";
            if (!*token) {
                break;    /* Only '-' was found. */
            } else if (*token == '-') {
                const char *arg = strchr(++token, '=');
                if (arg) {
                    arg++;
                } else {
                    arg = next_token;
                }

                ret = handler(instance, 0, token, arg);
                if (ret < 0) {
                    return -1;
                }
                if (arg == next_token) {
                    i += ret;
                }
            } else {
                char c;
                while ((c = *token++) != '\0') {
                    const char *arg = *token ? token : next_token;
                    ret = handler(instance, c, token, arg);
                    if (ret < 0) {
                        return -1;
                    }
                    if (ret > 0) {
                        if (arg == token) {
                            token = "";
                        } else {
                            i++;
                        }
                    }
                } /* while */
            } /* else (*token == '-') */
        } else {
            break;    /* a non-option argument was fonud. */
        } 
    } /* for (i) */

    return i;
}

int option_strcmp(const char *option, const char *longname)
{
    const char *p = strchr(option, '=');
    return p ? strncmp(option, longname, p-option) : strcmp(option, longname);
}
