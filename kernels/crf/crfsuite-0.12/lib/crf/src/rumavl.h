/*
 * RumAVL - Threaded AVL Tree Implementation
 *
 * Copyright (c) 2005-2007 Jesse Long <jpl@unknown.za.net>
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *   1. The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *   2. The origin of the Software must not be misrepresented; you must not
 *	claim that you wrote the original Software.
 *   3. Altered source versions of the Software must be plainly marked as
 *	such, and must not be misrepresented as being the original Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Please see the `README' file, the documentation in the `doc' directory and
 * the `rumavl.c' source file for more information.
 */

#ifndef RUMAVL_H
#define RUMAVL_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>	/* size_t */



    
/*----------------------------------------------------------------------------
 * DATA TYPES
 *--------------------------------------------------------------------------*/

/* Opaque context handle for the tree */
typedef struct rumavl RUMAVL;

/* Node type - used for iterating */
typedef struct rumavl_node RUMAVL_NODE;




/*----------------------------------------------------------------------------
 * FUNDEMENTAL FUNCTIONS
 *--------------------------------------------------------------------------*/

/* Create a new RumAVL tree */
RUMAVL *rumavl_new (size_t reclen, 
		    int (*cmp)(const void *, const void *, size_t, void *),
		    void *(*alloc)(void *, size_t, void *),
		    void *udata);

/* Destroy a RumAVL tree */
void rumavl_destroy (RUMAVL *tree);

/* This function returns the size of each record in a tree */
size_t rumavl_record_size (RUMAVL *tree);

/* Get a pointer to the udata pointer */
void **rumavl_udata  (RUMAVL *tree);

/* Insert a record into a tree, overwriting an existing record necessary */
int rumavl_set (RUMAVL *tree, const void *record);
/* Insert a record into a tree, never overwrites an existing record */
int rumavl_insert (RUMAVL *tree, const void *record);

/* Retrieve record from tree, or NULL */
void *rumavl_find (RUMAVL *tree, const void *find);

/* Remove record from tree */
int rumavl_delete (RUMAVL *tree, const void *record);




/*----------------------------------------------------------------------------
 * ITERATOR FUNCTIONS
 *--------------------------------------------------------------------------*/

/* Get a pointer to the node containing a specific record */
RUMAVL_NODE *rumavl_node_find (RUMAVL *tree, const void *find, void **record);

/* Get the next node in sequence after a specific node, in a specific
 * direction, or get the first node on either end of a tree */
RUMAVL_NODE *rumavl_node_next (RUMAVL *tree, RUMAVL_NODE *node, int dir,
				    void **record);
/* Possible directions */
#define RUMAVL_DESC (-1)
#define RUMAVL_ASC  (+1)

/* Get a record held by a specific node */
void *rumavl_node_record (RUMAVL_NODE *node);

/* Pass each record in a tree to a user defined callback function */
extern int rumavl_foreach (RUMAVL *tree, int dir,
	    int (*cbfn)(RUMAVL *, void *, void *), void *udata);




/*----------------------------------------------------------------------------
 * CALLBACK FUNCTIONS
 *
 * Functions giving you more control over the actions of this library.
 *--------------------------------------------------------------------------*/

int (**rumavl_owcb(RUMAVL *tree))(RUMAVL *, RUMAVL_NODE *, void *, 
	const void *, void *);
int (**rumavl_delcb(RUMAVL *tree))(RUMAVL *, RUMAVL_NODE *, void *, void *);




/*----------------------------------------------------------------------------
 * MEMORY MANAGEMENT
 *
 * The rumavl_mem struct is used to define how a RUMAVL object allocates
 * and frees memory.
 *--------------------------------------------------------------------------*/
void *(**rumavl_alloc(RUMAVL *tree))(void *ptr, size_t size, void *udata);



/*----------------------------------------------------------------------------
 * ERROR CODES
 *
 * The functions returning int's will return these errors
 *--------------------------------------------------------------------------*/

#define RUMAVL_ERR_INVAL  (-1)	/* Invalid argument */
#define RUMAVL_ERR_NOMEM  (-2)	/* Insufficient memory */
#define RUMAVL_ERR_NOENT  (-3)	/* Entry does not exist */
#define RUMAVL_ERR_EORNG  (-5)	/* No nodes left in range */
#define RUMAVL_ERR_EXIST  (-6)	/* Entry already exists */

/* returns static string describing error number */
extern const char *rumavl_strerror (int errno);

#ifdef __cplusplus
}
#endif

#endif /* ifndef RUMAVL_H */
