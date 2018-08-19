/****************************************************************************
* FILE:	free.c

* DESCRIPTION:
	memory库的free函数，从C51移植而来
* REVISION HISTORY:
	2012.03.18	lixingfu created it.
*/
/*****************************************************************************/

#include "mem.h"


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void free (void  *memp)
{
/*-----------------------------------------------
FREE attempts to organize Q, P0, and P so that
Q < P0 < P.  Then, P0 is inserted into the free
list so that the list is maintained in address
order.

FREE also attempts to consolidate small blocks
into the largest block possible.  So, after
allocating all memory and freeing all memory,
you will have a single block that is the size
of the memory pool.  The overhead for the merge
is very minimal.
-----------------------------------------------*/
	__memp__ q;		/* ptr to free block */
	__memp__ p;		/* q->next */
	__memp__ p0;	/* block to free */

/*-----------------------------------------------
If the user tried to free NULL, get out now.
Otherwise, get the address of the header of the
memp block (P0).  Then, try to locate Q and P
such that Q < P0 < P.
-----------------------------------------------*/
	if ((memp == NULL) || (AVAIL.len == 0))
		return;

	p0 = memp;
	p0 = &p0[-1];		/* get address of header */

/*-----------------------------------------------
Initialize.
Q = Location of first available block.
-----------------------------------------------*/
	q = &AVAIL;

/*-----------------------------------------------
B2. Advance P.
Hop through the list until we find a free block
that is located in memory AFTER the block we're
trying to free.
-----------------------------------------------*/
	while (1)
	{
		p = q->next;

		if ((p == NULL) || (p > memp))
			break;

		q = p;
	}

/*-----------------------------------------------
B3. Check upper bound.
If P0 and P are contiguous, merge block P into
block P0.
-----------------------------------------------*/
	if ((p != NULL) && ((unsigned int)(((char *)memp) + p0->len) == (unsigned int)p))
	{
		p0->len += p->len + HLEN;
		p0->next = p->next;
	}
	else
	{
		p0->next = p;
	}

/*-----------------------------------------------
B4. Check lower bound.
If Q and P0 are contiguous, merge P0 into Q.
-----------------------------------------------*/
	if ((unsigned int)(((char  *)q) + q->len + HLEN) == (unsigned int)p0)
	{
		q->len += p0->len + HLEN;
		q->next = p0->next;
	}
	else
	{
		q->next = p0;
	}
}

