/*----------------------------------------------------------------------------
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
 *--------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Although not required by the license, I would appreciate it if you would
 * send me a mail notifying me of bugfixes and enhancements you make to this
 * code. My email address is <jpl@unknown.za.net>
 *--------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *			     DEVELOPEMENT NOTES
 *
 * Links
 *    Each node has two links, link[0] is the left child, and link[1] is the
 *    right child. When a link points to a node that is actually below it in
 *    the BST, the respective thread flag is marked 0. When the link is a
 *    thread, the respective thread flag is marked 1, or 2 if the thread is
 *    to the opposite edge of the BST.
 *    
 * Direction
 *    In RumAVL we use the numbers -1 (RUMAVL_DESC) and +1 (RUMAVL_ASC) to 
 *    indicate direction, where -1 (RUMAVL_DESC) means left or descending in
 *    value, and +1 (RUMAVL_ASC) means right or ascending in value.
 *
 * Threads
 *    In RumAVL, the threads (non-bst links of leaves) are implemented in a
 *    sort of circular list. It is important to note that you cannot go
 *    through the entire list by following the same link, as you would when
 *    going through a linked list. Draw an example threaded AVL tree on paper
 *    and see why.
 *
 *--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include "rumavl.h"

/* For memory allocation debugging
#ifdef USE_MEMBUG
  #define MEMBUG_DEFINES
  #include <membug.h>
#endif */




/*****************************************************************************
 * 
 * MACROS - to make readability better
 * 
 ****************************************************************************/

/* Link numbers */
#define LEFT		(0)
#define RIGHT		(1)

/* Direction to link no, expects RUMAVL_DESC or RUMAVL_ASC */
#define LINK_NO(i)	(((i) + 1) / 2) /* -1 => 0; 1 => 1 */
/* Get opposite link number, expects LEFT or RIGHT */
#define OTHER_LINK(i)	((i) ^ 1)	/* 1 => 0; 0 => 1 */

/* link no to direction, expects LEFT or RIGHT */
#define DIR_NO(i)	(((i) * 2) - 1) /* 0 => -1; 1 => 1 */
/* opposite direction, expects RUMAVL_DESC or RUMAVL_ASC */
#define OTHER_DIR(i)	((i) * -1)	/* -1 => 1; 1 => -1 */

/* Memory allocation functions */
#define mem_alloc(tree, bytes)		mem_mgr((tree), NULL, (bytes))
#define mem_free(tree, ptr)		mem_mgr((tree), (ptr), 0)
#define mem_relloc(tree, ptr, bytes)	mem_mgr((tree), (ptr), (bytes))




/*****************************************************************************
 * 
 * DATA TYPES
 * 
 ****************************************************************************/

/* 
 * RUMAVL - the handle on the tree
 *
 * All settings for a tree are in the RUMAVL object, including memory
 * management, delete and overwrite callback functions, and the record
 * comparison function pointer.
 */
struct rumavl {
    RUMAVL_NODE *root;		    /* root node in tree */
    size_t reclen;		    /* length of records */
    int (*cmp)(const void *,	    /* function to compare records */
	       const void *, 
	       size_t,
	       void *);
    int (*owcb)(RUMAVL *, RUMAVL_NODE *, void *, const void *, void *);
    int (*delcb)(RUMAVL *, RUMAVL_NODE *, void *, void *);
    void *(*alloc)(void *, size_t, void *);
    void *udata;		    /* user data for callbacks */
};

/*
 * RUMAVL_NODE - the node structure
 *
 * RUMAVL_NODE's contain all information about a specific node, including
 * links to the right and left children of the node, and flags (thread) 
 * indicating whether or not the links are threads or not, and the balance
 * factor of the node.
 *
 * The record associated with each node is allocated along with the node,
 * and can be found directly after the node, by using the NODE_REC() macro.
 */
struct rumavl_node {
    RUMAVL_NODE	   *link[2];	/* links to child nodes */
    char	    thread[2];	/* flags for links, normal link or thread? */
    signed char	    balance;	/* balance factor for node */
    void	   *rec;
    #define NODE_REC(node) ((node)->rec)
};

/*
 * RUMAVL_STACK - a stack of nodes forming a path to a node
 *
 * RUMAVL_STACK's are used while deleting and inserting nodes, where effects
 * could be felt by all parents of the node. RUMAVL_STACK's are implemented
 * in a singly linked list. This is a change from the method used by most AVL
 * trees, where a static array node pointers are allocated. Linked lists allow
 * fo an unlimited height in the AVL tree.
 *
 * node is a pointer to the parent node's pointer to the node in question.
 * dir is the direction of the descent from this node.
 */
typedef struct rumavl_stack RUMAVL_STACK;
struct rumavl_stack {
    RUMAVL_STACK *next;
    RUMAVL_NODE **node;
    int dir;
};

/* various other RumAVL specific structs defined in rumavl.h */




/*****************************************************************************
 * 
 * FORWARD DECLERATIONS
 * 
 ****************************************************************************/

static RUMAVL_NODE *seq_next (RUMAVL_NODE *node, int dir);
static RUMAVL_NODE *node_new(RUMAVL *tree, const void *record);
static void node_destroy (RUMAVL *tree, RUMAVL_NODE *node);
static int stack_push (RUMAVL *tree, RUMAVL_STACK **stack, RUMAVL_NODE **node,
						int dir);
static void stack_destroy(RUMAVL *tree, RUMAVL_STACK *stack);
static void stack_update(RUMAVL *tree, RUMAVL_STACK *stack, signed char diff);

static signed char balance (RUMAVL_NODE **node, int dir);
static signed char rotate (RUMAVL_NODE **node, int dir);

static void *mem_mgr (RUMAVL *tree, void *ptr, size_t size);

static int rec_cmp (RUMAVL *tree, const void *reca, const void *recb);
static int my_cmp (const void *a, const void *b, size_t n, void *udata);

static int insert_cb (RUMAVL *t, RUMAVL_NODE *n, void *r1, const void *r2, 
	void *udata);



/*****************************************************************************
 * 
 * PUBLIC FUNCTIONS
 * 
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * rumavl_new - allocates a new RUMAVL object, and initialises it. This is the
 * only time the user gets to set the record length and record comparison
 * function, to avoid data loss.
 *--------------------------------------------------------------------------*/
RUMAVL *rumavl_new (size_t reclen, 
		    int (*cmp)(const void *, const void *, size_t, void *),
		    void *(*alloc)(void *, size_t, void *),
		    void *udata)
{
    RUMAVL *tree;

    if (reclen < 1)
	return NULL;

    if (alloc == NULL)
	tree = malloc(sizeof(RUMAVL));
    else
	tree = alloc(NULL, sizeof(RUMAVL), udata);

    if (tree == NULL)
	return NULL;
	
    tree->root = NULL;
    
    tree->owcb = NULL;
    tree->delcb = NULL;

    tree->alloc = alloc;

    tree->reclen = reclen;
    tree->udata = udata;

    if (cmp == NULL)
	tree->cmp = my_cmp;
    else
	tree->cmp = cmp;
    
    return tree;
}

/*----------------------------------------------------------------------------
 * rumavl_destroy - cleanly frees all memory used by the RUMAVL, as well as 
 * all nodes. All nodes are passed to the delete callback function in case the
 * user has a special way of destroying nodes. The return value of the delete
 * callback function is ignored, because once we start destroying we cant
 * simply undestroy half the nodes.
 *--------------------------------------------------------------------------*/
void rumavl_destroy (RUMAVL *tree)
{
    RUMAVL_NODE *node, *tmp;
    
    if (tree->root != NULL){
	/* walk through tree deleting all */
	node = tree->root;
	while (node->thread[LEFT] == 0) /* move to bottom left most node */
	    node = node->link[LEFT];
	while (node != NULL){
	    tmp = seq_next(node, RUMAVL_ASC);
	    if (tree->delcb != NULL){
		tree->delcb(tree, node, NODE_REC(node), tree->udata);
	    }
	    node_destroy(tree, node);
	    node = tmp;
	}
    }

    if (tree->alloc == NULL)
	free(tree);
    else
	tree->alloc(tree, 0, tree->udata);
}

/*---------------------------------------------------------------------------
 * rumavl_udata - get a pointer to the tree's user pointer
 *-------------------------------------------------------------------------*/
void **rumavl_udata (RUMAVL *tree)
{
    return &tree->udata;
}

int (**rumavl_owcb(RUMAVL *tree))(RUMAVL *, RUMAVL_NODE *, void *, 
	const void *, void *)
{
    return &tree->owcb;
}

int (**rumavl_delcb(RUMAVL *tree))(RUMAVL *, RUMAVL_NODE *, void *, void *)
{
    return &tree->delcb;
}

/*----------------------------------------------------------------------------
 * rumavl_set - set a node, overwriting if necessary, or creating if the node
 * does not exist
 *--------------------------------------------------------------------------*/
int rumavl_set (RUMAVL *tree, const void *record)
{
    RUMAVL_NODE **node, *tmp;
    RUMAVL_STACK *stack;
    int ln;
    
    if (tree->root == NULL){
	/* This is the first node in the tree */
	if ((tree->root = node_new(tree, record)) == NULL)
	    return RUMAVL_ERR_NOMEM;
	tree->root->link[LEFT] = tree->root;
	tree->root->link[RIGHT] = tree->root;
	tree->root->thread[LEFT] = 2;
	tree->root->thread[RIGHT] = 2;
	return 0;
    }

    /* Since the tree is not empty, we must descend towards the nodes ideal
     * possition, and we may even find an existing node with the same record.
     * We keep a list parents for the eventual node position, because these
     * parents may become inbalanced by a new insertion. */

    stack = NULL;
    node = &tree->root;
    for (;;){
	if ((ln = rec_cmp(tree, record, NODE_REC(*node))) == 0){
	    /* OK, we found the exact node we wish to set, and we now
	     * overwrite it. No change happens to the tree structure */
	    stack_destroy(tree, stack);
	    
	    if (tree->owcb != NULL &&
		    (ln = tree->owcb(tree, *node, NODE_REC(*node), 
				      record, tree->udata)) != 0){
		return ln;
	    }
	    
	    memcpy(NODE_REC(*node), record, tree->reclen);
	    return 0;
	}
	
	/* *node is not the node we seek */
	
	if (stack_push(tree, &stack, node, ln)){
	    stack_destroy(tree, stack);
	    return RUMAVL_ERR_NOMEM;
	}
	
	ln = LINK_NO(ln);
	if ((*node)->thread[ln] > 0){
	    /* This is as close to the correct node as we can get. We will
	     * now break and add the new node as a leaf */
	    break;
	}
	
	node = &(*node)->link[ln];
    }
	    
    /* we have reached a leaf, add new node here */
    if ((tmp = node_new(tree, record)) == NULL){
	stack_destroy(tree, stack);
	return RUMAVL_ERR_NOMEM;
    }
    /* new child inherits parent thread */
    tmp->link[ln] = (*node)->link[ln];
    tmp->thread[ln] = (*node)->thread[ln];
    if (tmp->thread[ln] == 2)
	tmp->link[ln]->link[OTHER_LINK(ln)] = tmp;
    
    tmp->link[OTHER_LINK(ln)] = *node;
    tmp->thread[OTHER_LINK(ln)] = 1;
    (*node)->link[ln] = tmp;
    (*node)->thread[ln] = 0;

    /* all parentage is now one level heavier - balance where necessary */
    stack_update(tree, stack, +1);
    
    return 0;
}


/*----------------------------------------------------------------------------
 * rumavl_insert - like rumavl_set, but only works if the node does not
 * exist. Temporarily replaces overwrite callback with a function that
 * always prevents overwrite, and calls rumavl_set()
 *--------------------------------------------------------------------------*/
int rumavl_insert (RUMAVL *tree, const void *record)
{
    int retv;
    int (*tmp)(RUMAVL *, RUMAVL_NODE *, void *, const void *, void *);
    
    tmp = tree->owcb;
    tree->owcb = insert_cb;
    retv = rumavl_set(tree, record);
    tree->owcb = tmp;
    return retv;
}

/*----------------------------------------------------------------------------
 * rumavl_delete - deletes a node. Beware! this function is the worst part of
 * the library. Think (and draw pictures) when you edit this function.
 *--------------------------------------------------------------------------*/
int rumavl_delete (RUMAVL *tree, const void *record)
{
    RUMAVL_NODE **node, *tmpnode;
    RUMAVL_STACK *stack;
    int dir, ln;

    if (tree->root == NULL)	/* tree is empty */
	return RUMAVL_ERR_NOENT;

    stack = NULL;
    node = &tree->root;

    /* Find desired node */
    while ((dir = rec_cmp(tree, record, NODE_REC(*node))) != 0){
	if (stack_push(tree, &stack, node, dir) != 0)
	    goto nomemout;

	if ((*node)->thread[LINK_NO(dir)] > 0){
	    /* desired node does not exist */
	    stack_destroy(tree, stack);
	    return RUMAVL_ERR_NOENT;
	}
	node = &(*node)->link[LINK_NO(dir)];
    }

    /* OK, we got the node to be deleted, now get confirmation from user */
    if (tree->delcb != NULL &&
	    (ln = tree->delcb(tree, *node, NODE_REC(*node), tree->udata)) 
	    != 0){
	stack_destroy(tree, stack);
	return ln;
    }

    if ((*node)->thread[LEFT] > 0){
	if ((*node)->thread[RIGHT] > 0){
	    /* ooh look, we're a leaf */
	    tmpnode = *node;
	    if (stack != NULL){
		/* This node has a parent, which will need to take over a
		 * thread from the node being deleted. First we work out
		 * which (left/right) child we are of parent, then give
		 * parent the respective thread. If the thread destination
		 * points back to us (edge of tree thread), update it to
		 * point to our parent. */
		ln = LINK_NO(stack->dir);
		(*stack->node)->link[ln] = tmpnode->link[ln];
		(*stack->node)->thread[ln] = tmpnode->thread[ln];
		if ((*stack->node)->thread[ln] == 2)
		    (*stack->node)->link[ln]->link[OTHER_LINK(ln)] =
			*stack->node;
	    }else{
		/* 
		 * the only time stack will == NULL is when we are
		 * deleting the root of the tree. We already know that
		 * this is a leaf, so we will be leaving the tree empty.
		 */
		tree->root = NULL;
	    }
	    node_destroy(tree, tmpnode);
	}else{
	    /* *node has only one child, and can be pruned by replacing
	     * *node with its only child. This block of code and the next
	     * should be identical, except that all directions and link
	     * numbers are opposite.
	     *
	     * Let node being deleted = DELNODE for this comment.
	     * DELNODE only has one child (the right child). The left
	     * most descendant of DELNODE will have a thread (left thread)
	     * pointing to DELNODE. This thread must be updated to point
	     * to the node currently pointed to by DELNODE's left thread.
	     *
	     * DELNODE's left thread may point to the opposite edge of the
	     * BST. In this case, the destination of the thread will have
	     * a thread back to DELNODE. This will need to be updated to
	     * point back to the leftmost descendant of DELNODE.
	     */
	    tmpnode = *node;		    /* node being deleted */
	    *node = (*node)->link[RIGHT];   /* right child */
	    /* find left most descendant */
	    while ((*node)->thread[LEFT] == 0) 
		node = &(*node)->link[LEFT];
	    /* inherit thread from node being deleted */
	    (*node)->link[LEFT] = tmpnode->link[LEFT];
	    (*node)->thread[LEFT] = tmpnode->thread[LEFT];
	    /* update reverse thread if necessary */
	    if ((*node)->thread[LEFT] == 2)
		(*node)->link[LEFT]->link[RIGHT] = *node;
	    node_destroy(tree, tmpnode);
	}
    }else if ((*node)->thread[RIGHT] > 0){
	/* see above */
	tmpnode = *node;
	*node = (*node)->link[LEFT];
	while ((*node)->thread[RIGHT] == 0)
	    node = &(*node)->link[RIGHT];
	(*node)->link[RIGHT] = tmpnode->link[RIGHT];
	(*node)->thread[RIGHT] = tmpnode->thread[RIGHT];
	if ((*node)->thread[RIGHT] == 2)
	    (*node)->link[RIGHT]->link[LEFT] = *node;
	node_destroy(tree, tmpnode);
    }else{
	/* Delete a node with children on both sides. We do this by replacing
	 * the node to be deleted (delnode) with its inner most child
	 * on the heavier side (repnode). This in place replacement is quicker
	 * than the previously used method of rotating delnode until it is a
	 * (semi) leaf.
	 *
	 * At this point node points to delnode's parent's link to delnode. */
	RUMAVL_NODE *repnode, *parent;
	int outdir, outln;

	/* find heaviest subtree */
	if ((*node)->balance > 0){
	    outdir = +1;    /* outter direction */
	    dir = -1;	    /* inner direction */
	    outln = 1;	    /* outer link number */
	    ln = 0;	    /* inner link number */
	}else{
	    outdir = -1;    /* same as above, but opposite subtree */
	    dir = +1;
	    outln = 0;
	    ln = 1;
	}
	
	/* Add node to be deleted to the list of nodes to be rebalanced.
	 * Rememer that the replacement node will actually be acted apon,
	 * and that the replacement node should feel the effect of its own
	 * move */
	if (stack_push(tree, &stack, node, outdir) != 0)
	    goto nomemout;
	
	parent = *node;
	repnode = parent->link[outln];

	if (repnode->thread[ln] != 0){
	    /* repnode inherits delnode's lighter tree, and balance, and gets
	     * balance readjusted below */
	    repnode->link[ln] = (*node)->link[ln];
	    repnode->thread[ln] = (*node)->thread[ln];
	    repnode->balance = (*node)->balance;
	}else{
	    /* Now we add delnodes direct child to the list of "to update".
	     * We pass a pointer to delnode's link to its direct child to 
	     * stack_push(), but that pointer is invalid, because when
	     * stack_update() tries to access the link, delnode would have
	     * been destroyed. So, we remember the stack position at which
	     * we passed the faulty pointer to stack_push, and update its
	     * node pointer when we find repnode to point to repnodes 
	     * link on the same side */
	    RUMAVL_STACK *tmpstack;

	    if (stack_push(tree, &stack, &parent->link[outln], dir) != 0)
		goto nomemout;

	    tmpstack = stack;

	    parent = repnode;
	    repnode = repnode->link[ln];

	    /* move towards the innermost child of delnode */		
	    while (repnode->thread[ln] == 0){
		if (stack_push(tree, &stack, &parent->link[ln], dir) != 0)
		    goto nomemout;
		parent = repnode;
		repnode = repnode->link[ln];
	    }

	    if (repnode->thread[outln] == 0){
		/* repnode's parent inherits repnodes only child */
		parent->link[ln] = repnode->link[outln];
	    }else{
		/* parent already has a link to repnode, but it must now be
		 * marked as a thread */
		parent->thread[ln] = 1;
	    }

	    repnode->link[0] = (*node)->link[0];
	    repnode->thread[0] = (*node)->thread[0];
	    repnode->link[1] = (*node)->link[1];
	    repnode->thread[1] = (*node)->thread[1];
	    repnode->balance = (*node)->balance;

	    /* see comment above */
	    tmpstack->node = &repnode->link[outln];
	}
	node_destroy(tree, *node);
	*node = repnode;

	/* innermost child in lighter tree has an invalid thread to delnode,
	 * update it to point to repnode */
	repnode = seq_next(repnode, dir);
	repnode->link[outln] = *node;
    }

    /* update parents' balances */
    stack_update(tree, stack, -1);
    return 0;

nomemout:
    stack_destroy(tree, stack);
    return RUMAVL_ERR_NOMEM;
}

/*----------------------------------------------------------------------------
 * rumavl_find
 *
 * Returns a pointer to the record that matches "record".
 *--------------------------------------------------------------------------*/
void *rumavl_find (RUMAVL *tree, const void *find)
{
    void *record;
    rumavl_node_find(tree, find, &record);
    return record;
}

void *(**rumavl_alloc(RUMAVL *tree))(void *ptr, size_t size, void *udata)
{
    return &tree->alloc;
}

/*----------------------------------------------------------------------------
 * rumavl_record_size - returns size of all records in a tree
 *--------------------------------------------------------------------------*/
size_t rumavl_record_size (RUMAVL *tree)
{
    return tree->reclen;
}

/*----------------------------------------------------------------------------
 * rumavl_node_find
 *
 * Returns a pointer to the node that matches "record".
 *--------------------------------------------------------------------------*/
RUMAVL_NODE *rumavl_node_find (RUMAVL *tree, const void *find, void **record)
{
    RUMAVL_NODE *node;
    int ln;
    
    if (find == NULL || tree->root == NULL)
	goto fail;

    node = tree->root;
    for (;;){
	if ((ln = rec_cmp(tree, find, NODE_REC(node))) == 0){
	    if (record != NULL)
		*record = NODE_REC(node);
	    return node;
	}

	ln = LINK_NO(ln);
	if (node->thread[ln] > 0)
	    break;

	node = node->link[ln];
    }
    /* we didn't find the desired node */

fail:
    if (record != NULL)
	*record = NULL;
    
    return NULL; 
}

/*----------------------------------------------------------------------------
 * rumavl_node_next - find next node 
 *--------------------------------------------------------------------------*/
RUMAVL_NODE *rumavl_node_next (RUMAVL *tree, RUMAVL_NODE *node, int dir,
				    void **record)
{
    /* make sure `dir' is either RUMAVL_ASC or RUMAVL_DESC */
    if (dir == 0)
	goto fail;
    else if (dir > 0)
	dir = RUMAVL_ASC;
    else
	dir = RUMAVL_DESC;

    /* if node is uninitialised, start with first possible node in `dir'
     * direction */
    if (node == NULL){
	/* unless the tree is empty of course */
	if (tree->root == NULL)
	    goto fail;

	dir = OTHER_LINK(LINK_NO(dir));
	node = tree->root;
	while (node->thread[dir] == 0){
	    node = node->link[dir];
	}
	goto found;
    }

    if ((node = seq_next(node, dir)) == NULL)
	goto fail;

    /* fall through */

found:
    if (record != NULL)
	*record = NODE_REC(node);
    return node;

fail:
    if (record != NULL)
	*record = NULL;
    return NULL;
}

/*----------------------------------------------------------------------------
 * rumavl_node_record - returns a pointer to the record stored in a node
 *--------------------------------------------------------------------------*/
void *rumavl_node_record (RUMAVL_NODE *node)
{
    return NODE_REC(node);
}

/*----------------------------------------------------------------------------
 * rumavl_foreach - loop through entire tree, using temporary iterator
 *--------------------------------------------------------------------------*/
extern int rumavl_foreach (RUMAVL *tree, int dir,
	    int (*cbfn)(RUMAVL *, void *, void *), void *udata)
{
    RUMAVL_NODE *node;
    int retv;
    void *record;

    if (cbfn == NULL)
	return RUMAVL_ERR_INVAL;
    
    retv = RUMAVL_ERR_NOENT;
    node = NULL;
    while ((node = rumavl_node_next(tree, node, dir, &record)) != NULL){
	if ((retv = cbfn(tree, record, udata)) != 0)
	    break;
    }

    return retv;
}

/*----------------------------------------------------------------------------
 * rumavl_strerror - return string description of RumAVL error code
 *--------------------------------------------------------------------------*/
const char *rumavl_strerror (int errno)
{
    switch (errno){
	case 0:
	    return "Operation successful";
	case RUMAVL_ERR_INVAL:
	    return "Invalid argument to function";
	case RUMAVL_ERR_NOMEM:
	    return "Insufficient memory to complete operation";
	case RUMAVL_ERR_NOENT:
	    return "Entry does not exist";
	case RUMAVL_ERR_EORNG:
	    return "No more entries in range";
	case RUMAVL_ERR_EXIST:
	    return "Entry already exists";
    }
    return "UNKNOWN ERROR";
}




/*****************************************************************************
 * 
 * PRIVATE FUNCTIONS
 * 
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * insert_cb - used by rumavl_insert() to disallow any overwriting by
 * rumavl_set()
 *--------------------------------------------------------------------------*/
static int insert_cb (RUMAVL *t, RUMAVL_NODE *n, void *r1, const void *r2, 
	void *udata)
{
    (void) t; (void) r1; (void) r2; (void) udata; (void) n;
    return RUMAVL_ERR_EXIST;
}

/*----------------------------------------------------------------------------
 * seq_next - return a pointer to the next node in sequence
 *--------------------------------------------------------------------------*/
static RUMAVL_NODE *seq_next (RUMAVL_NODE *node, int dir)
{
    int ln;
    
    ln = LINK_NO(dir);
    if (node->thread[ln] == 2){
	return NULL;
    }else if (node->thread[ln] == 1){
	return node->link[ln];
    }
    node = node->link[ln];
    ln = OTHER_LINK(ln);
    while (node->thread[ln] == 0){
	node = node->link[ln];
    }
    return node;
}

/*----------------------------------------------------------------------------
 * node_new - create a new node. MUST update link[] and thread[] after calling
 * this function
 *--------------------------------------------------------------------------*/
static RUMAVL_NODE *node_new(RUMAVL *tree, const void *record)
{
    RUMAVL_NODE *node;

    if ((node = mem_alloc(tree, sizeof(RUMAVL_NODE))) == NULL)
	return NULL;

    if ((node->rec = mem_alloc(tree, tree->reclen)) == NULL){
	mem_free(tree, node);
	return NULL;
    }

    memcpy(node->rec, record, tree->reclen);
    node->balance = 0;
    node->link[0] = NULL;
    node->link[1] = NULL;
    node->thread[0] = 0;
    node->thread[1] = 0;
    return node;
}

/*----------------------------------------------------------------------------
 * node_destroy - cleanly destroy node
 *--------------------------------------------------------------------------*/
static void node_destroy (RUMAVL *tree, RUMAVL_NODE *node)
{
    mem_free(tree, node);
}

/*----------------------------------------------------------------------------
 * stack_push - push a node entry onto stack, for rumavl_set() and 
 * rumavl_delete(). If this is the first entry, *stack should == NULL
 *--------------------------------------------------------------------------*/
static int stack_push(RUMAVL *tree, RUMAVL_STACK **stack, RUMAVL_NODE **node, 
			int dir)
{
    RUMAVL_STACK *tmp;
    
    if ((tmp = mem_alloc(tree, sizeof(RUMAVL_STACK))) == NULL)
	return -1;
    
    tmp->next = *stack;
    *stack = tmp;
    tmp->node = node;
    tmp->dir = dir;

    return 0;
}

/*----------------------------------------------------------------------------
 * stack_destroy - free up a stack
 *--------------------------------------------------------------------------*/
static void stack_destroy(RUMAVL *tree, RUMAVL_STACK *stack)
{
    RUMAVL_STACK *tmp;
    while (stack != NULL){
	tmp = stack;
	stack = stack->next;
	mem_free(tree, tmp);
    }
}

/*----------------------------------------------------------------------------
 * stack_update - goes up stack readjusting balance as needed. This function
 * serves as a testiment to the philosophy of commenting while you code, 'cos
 * hell if I can remember how I got to this. I think is has something to do
 * with the varying effects on tree height, depending on exactly which sub 
 * tree, or sub-sub tree was modified. TODO study and comment
 *--------------------------------------------------------------------------*/
static void stack_update(RUMAVL *tree, RUMAVL_STACK *stack, signed char diff)
{
    RUMAVL_STACK *tmpstack;
    
    /* if diff becomes 0, we quit, because no further change to ancestors
     * can be made */
    while (stack != NULL && diff != 0){
	signed char ob, nb;
	ob = (*stack->node)->balance;
	(*stack->node)->balance += diff * (signed char)stack->dir;
	nb = (*stack->node)->balance;
	if (diff < 0){
	    if (stack->dir == -1 && ob < 0){
		if (nb > 0)
		    nb = 0;
		diff = (nb - ob) * -1;
	    }else if (stack->dir == 1 && ob > 0){
		if (nb < 0)
		    nb = 0;
		diff = nb - ob;
	    }else{
		diff = 0;
	    }
	}else{
	    if (stack->dir == -1 && nb < 0){
		if (ob > 0)
		    ob = 0;
		diff = (nb - ob) * -1;
	    }else if (stack->dir == 1 && nb > 0){
		if (ob < 0)
		    ob = 0;
		diff = nb - ob;
	    }else{
		diff = 0;
	    }
	}
	while ((*stack->node)->balance > 1){
	    diff += balance(stack->node, -1);
	}
	while ((*stack->node)->balance < -1){
	    diff += balance(stack->node, 1);
	}
	tmpstack = stack;
	stack = stack->next;
	mem_free(tree, tmpstack);
    }

    /* we may exit early if diff becomes 0. We still need to free all stack
     * entries */
    while (stack != NULL){
	tmpstack = stack;
	stack = stack->next;
	mem_free(tree, tmpstack);
    }
}

/*----------------------------------------------------------------------------
 * my_cmp - a wrapper around memcmp() for default record comparison function.
 *--------------------------------------------------------------------------*/
static int my_cmp (const void *a, const void *b, size_t n, void *udata)
{
    (void) udata;
    return memcmp(a, b, n);
}

/*----------------------------------------------------------------------------
 * rec_cmp - a wrapper around the record comparison function, that only
 * returns 0, RUMAVL_ASC or RUMAVL_DESC.
 *--------------------------------------------------------------------------*/
static int rec_cmp (RUMAVL *tree, const void *reca, const void *recb)
{
    int retv;
    retv = tree->cmp(reca, recb, tree->reclen, tree->udata);
    if (retv < 0)
	return RUMAVL_DESC;
    if (retv > 0)
	return RUMAVL_ASC;
    return 0;
}

/*----------------------------------------------------------------------------
 * Balance - rotate or double rotate as needed. Sometimes simply rotating a
 * tree is inefficient, as it leaves the tree as inbalanced as it was before
 * the rotate. To rectify this, we first rotate the heavier child so that the
 * heavier grandchild is on the outside, then rotate as per normal.
 *
 * TODO Check all callers, and make sure that they call this function sanely,
 *	and then remove unnecessary checks.
 *--------------------------------------------------------------------------*/
static signed char balance (RUMAVL_NODE **node, int dir)
{
    int ln;
    signed char retv;
    
    if (node == NULL || *node == NULL || (dir * dir) != 1)
	return 0;

    ln = OTHER_LINK(LINK_NO(dir)); /* link number of new root */
    
    /* new root must exist */
    if ((*node)->thread[ln] > 0)
	return 0;

    retv = 0;
    if ((*node)->link[ln]->balance == (char) dir &&
	    (*node)->link[ln]->thread[OTHER_LINK(ln)] == 0){
	/* double rotate if inner grandchild is heaviest */
	retv = rotate (&((*node)->link[ln]), OTHER_DIR(dir));
    }
    
    return retv + rotate (node, dir);
}

/*----------------------------------------------------------------------------
 * rotate
 *
 * rotates a tree rooted at *node. dir determines the direction of the rotate,
 * dir < 0 -> left rotate; dir >= 0 -> right rotate
 *
 * TODO How sure are we that all callers pass decent `dir' values?
 * TODO Restudy the tree height modification and balance factor algorithms,
 *	and document them.
 *--------------------------------------------------------------------------*/
static signed char rotate (RUMAVL_NODE **node, int dir)
{
    RUMAVL_NODE *tmp;
    signed char a, b, ad, bd, retv;
    int ln;

    /* force |dir| to be either -1 or +1 */
    if (node == NULL || *node == NULL || (dir * dir) != 1)
	return 0;

    ln = LINK_NO(dir);
    ln = OTHER_LINK(ln); /* link number of new root */

    /* new root must exist */
    if ((*node)->thread[ln] > 0)
	return 0;

    /* calculate effect on tree height */
    if ((dir == 1 && (*node)->balance < 0 && (*node)->link[0]->balance >= 0)||
       (dir == -1 && (*node)->balance > 0 && (*node)->link[1]->balance <= 0)){
	retv = 0;
    }else{
	if (dir == 1){
	    if ((*node)->balance < -1)
		retv = -1;
	    else if ((*node)->balance == -1)
		retv = 0;
	    else
		retv = +1;
	}else{
	    if ((*node)->balance > 1)
		retv = -1;
	    else if ((*node)->balance == 1)
		retv = 0;
	    else
		retv = +1;
	}
    }
		    

    /* rotate tree */
    tmp = *node;
    *node = tmp->link[ln];
    if ((*node)->thread[OTHER_LINK(ln)] > 0){
	tmp->thread[ln] = 1;
    }else{
	tmp->link[ln] = (*node)->link[OTHER_LINK(ln)];
	tmp->thread[ln] = 0;
    }
    (*node)->link[OTHER_LINK(ln)] = tmp;
    (*node)->thread[OTHER_LINK(ln)] = 0;
    


    /* rebalance factors after rotate matrix */
    a = tmp->balance;
    b = (*node)->balance;

    if (a > 0)
	ad = 1;
    else if (a < 0)
	ad = -1;
    else
	ad = 0;

    if (b > 0)
	bd = 1;
    else if (b < 0)
	bd = -1;
    else
	bd = 0;
    
    if (ad == OTHER_DIR(dir)){
	if (bd == OTHER_DIR(dir)){
	    tmp->balance += (b * -1) + dir;
	    if (tmp->balance * dir > 0)
		(*node)->balance = (tmp->balance - (b * -1)) + dir;
	    else
		(*node)->balance += dir;
	}else{
	    tmp->balance += dir;
	    (*node)->balance += dir;
	}
    }else{
	if (bd == OTHER_DIR(dir)){
	    tmp->balance += (b * -1) + dir;
	    (*node)->balance += dir + tmp->balance; 
	}else{
	    tmp->balance += dir;
	    (*node)->balance += dir + tmp->balance;
	}
    }
    
    return retv;
}

/*----------------------------------------------------------------------------
 * mem_alloc
 *
 * default memory allocation function (malloc wrapper)
 *--------------------------------------------------------------------------*/
static void *mem_mgr (RUMAVL *tree, void *ptr, size_t size)
{
    if (tree->alloc != NULL)
	return tree->alloc(ptr, size, tree->udata);
 
    return realloc(ptr, size);
}
