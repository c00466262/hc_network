#ifndef __MEM_H__
#define __MEM_H__

#include "stdlib.h"

/*-----------------------------------------------
Memory pool block structure and typedefs.
Memory is laid out as follows:

{[NXT|LEN][BLK (LEN bytes)]}{[NXT|LEN][BLK]}...

Note that the size of a node is:
          __mem__.len + sizeof (__mem__)
-----------------------------------------------*/
struct __mem__
{
	struct __mem__  *next;	/* single-linked list */
	unsigned int       len;	/* length of following block */
};

typedef struct __mem__  __memt__;
typedef __memt__  *__memp__;

#define	HLEN	(sizeof(__memt__))

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
extern __memt__  __mem_avail__ [];

#define AVAIL	(__mem_avail__[0])

#define MIN_BLOCK	(HLEN * 4)


// void init_mempool (void  *pool,unsigned int size);


#endif
