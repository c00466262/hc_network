/****************************************************************************
* FILE:	malloc.c

* DESCRIPTION:
	memory库的malloc函数，从C51移植而来
* REVISION HISTORY:
	2012.03.18	lixingfu created it.
*/
/*****************************************************************************/
#include "mem.h"

/*-----------------------------------------------------------------------------
void  *malloc (
  unsigned int size);			number of bytes to allocate

Return Value
------------
    NULL	FAILURE:  No free blocks of size are available
  NON-NULL	SUCCESS:  Address of block returned
-----------------------------------------------------------------------------*/
void  *malloc (
  unsigned int size)
{
	__memp__ q;			/* ptr to free block */
	__memp__ p;			/* q->next */
	unsigned int k;			/* space remaining in the allocated block */

/*-----------------------------------------------
Initialization:  Q is the pointer to the next
available block.
-----------------------------------------------*/
	q = &AVAIL;

/*-----------------------------------------------
End-Of-List:  P points to the next block.  If
that block DNE (P==NULL), we are at the end of
the list.
-----------------------------------------------*/
	while (1)
	{
		if ((p = q->next) == NULL)
		{
			return (NULL);				/* FAILURE */
		}

/*-----------------------------------------------
Found Space:  If block is large enough, reserve
if.  Otherwise, copy P to Q and try the next
free block.
-----------------------------------------------*/
		if (p->len >= size)
			break;

	 	 q = p;
	}//end while

/*-----------------------------------------------
Reserve P:  Use at least part of the P block to
satisfy the allocation request.  At this time,
the following pointers are setup:

P points to the block from which we allocate
Q->next points to P
-----------------------------------------------*/
	k = p->len - size;		/* calc. remaining bytes in block */

	if (k < MIN_BLOCK)		/* rem. bytes too small for new block */
	{
		q->next = p->next;
		return (&p[1]);				/* SUCCESS */
	}

/*-----------------------------------------------
Split P Block:  If P is larger than we need, we
split P into two blocks:  the leftover space and
the allocated space.  That means, we need to
create a header in the allocated space.
-----------------------------------------------*/
	k -= HLEN;
	p->len = k;

	q = (__memp__ ) (((char  *) (&p [1])) + k);
	q->len = size;

	return (&q[1]);					/* SUCCESS */
}

