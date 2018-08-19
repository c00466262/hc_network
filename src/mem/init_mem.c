/****************************************************************************
* FILE:	init_mem.c

* DESCRIPTION:
	memory库的初始化函数，从C51移植而来
* REVISION HISTORY:
	2012.03.18	lixingfu created it.
*/
/*****************************************************************************/
#include "mem.h"

/*-----------------------------------------------
Memory pool headers.  AVAIL points to the first
available block or is NULL if there are no free
blocks.  ROVER is a roving header that points to
a block somewhere in the list.

Note that the list is maintained in address
order.  AVAIL points to the block with the
lowest address.  That block points to the block
with the next higher address and so on.
-----------------------------------------------*/
__memt__  __mem_avail__ [2] =
{ 
	{ NULL, 0 },	/* HEAD for the available block list */
	{ NULL, 0 },	/* UNUSED but necessary so free doesn't join HEAD or ROVER with the pool */
};

/*-----------------------------------------------------------------------------
void init_mempool (
  void  *pool,	address of the memory pool
  unsigned int size);           size of the pool in bytes

-----------------------------------------------------------------------------*/
void init_mempool (
  void  *pool,
  unsigned int size)
{

/*-----------------------------------------------
If the pool points to the beginning of a memory
area (NULL), change it to point to 1 and decrease
the pool size by 1 byte.
-----------------------------------------------*/
	if (pool == NULL)
	{
		pool = (void*)1;
		size--;
	}

/*-----------------------------------------------
Set the AVAIL header to point to the beginning
of the pool and set the pool size.
-----------------------------------------------*/
	AVAIL.next = pool;
	AVAIL.len  = size;

/*-----------------------------------------------
Set the link of the block in the pool to NULL
(since it's the only block) and initialize the
size of its data area.
-----------------------------------------------*/
	(AVAIL.next)->next = NULL;
	(AVAIL.next)->len  = size - HLEN;
}


