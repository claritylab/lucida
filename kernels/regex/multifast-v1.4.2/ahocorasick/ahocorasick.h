/*
 * ahocorasick.h: the main ahocorasick header file.
 * This file is part of multifast.
 *
    Copyright 2010-2013 Kamiar Kanani <kamiar.kanani@gmail.com>

    multifast is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    multifast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with multifast.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AUTOMATA_H_
#define _AUTOMATA_H_

#include "actypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AC_NODE;

typedef struct AC_AUTOMATA
{
    /* The root of the Aho-Corasick trie */
    struct AC_NODE * root;

    /* maintain all nodes pointers. it will be used to access or release
    * all nodes. */
    struct AC_NODE ** all_nodes;

    unsigned int all_nodes_num; /* Number of all nodes in the automata */
    unsigned int all_nodes_max; /* Current max allocated memory for *all_nodes */

    /* this flag indicates that if automata is finalized by
     * ac_automata_finalize() or not. 1 means finalized and 0
     * means not finalized (is open). after finalizing automata you can not
     * add pattern to automata anymore. */
    unsigned short automata_open;

    /* It is possible to feed a large input to the automata chunk by chunk to
     * be searched using ac_automata_search(). in fact by default automata
     * thinks that all chunks are related unless you do ac_automata_reset().
     * followings are variables that keep track of searching state. */
    struct AC_NODE * current_node; /* Pointer to current node while searching */
    unsigned long base_position; /* Represents the position of current chunk
                                  * related to whole input text */

    /* The input text.
     * used only when it is working in settext/findnext mode */
    AC_TEXT_t * text;
    
    /* The lase searched position in the chunk. 
     * used only when it is working in settext/findnext mode */
    unsigned long position;
    
    /* Statistic Variables */
    
    /* Total patterns in the automata */
    unsigned long total_patterns;
    
} AC_AUTOMATA_t;


AC_AUTOMATA_t * ac_automata_init     (void);
AC_STATUS_t     ac_automata_add      (AC_AUTOMATA_t * thiz, AC_PATTERN_t * str);
void            ac_automata_finalize (AC_AUTOMATA_t * thiz);
int             ac_automata_search   (AC_AUTOMATA_t * thiz, AC_TEXT_t * text, int keep, AC_MATCH_CALBACK_f callback, void * param);

void            ac_automata_settext  (AC_AUTOMATA_t * thiz, AC_TEXT_t * text, int keep);
AC_MATCH_t *    ac_automata_findnext (AC_AUTOMATA_t * thiz);

void            ac_automata_release  (AC_AUTOMATA_t * thiz);
void            ac_automata_display  (AC_AUTOMATA_t * thiz, char repcast);


#ifdef __cplusplus
}
#endif

#endif
