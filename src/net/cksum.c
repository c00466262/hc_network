/********************************************************************************************
* FILE:	cksum.c

* DESCRIPTION:
	网络校验和算法
MODIFY HISTORY:
	2010.7.13            lixingfu   create
********************************************************************************************/

#include "config.h"
#include "cksum.h"

/*
	Return sum in host byte order. 
*/
UINT16 cksum(UINT8 *buf, int count)
{
    UINT32 sum = 0;
	int pos = 0;
	
	while(count > 1)
	{
		sum += (buf[pos]<<8) + buf[pos+1];
		pos += 2;
		count -= 2;
	}
	
	/* Add left-over byte, if any */
	if( count == 1)
	{
		sum += (buf[pos]<<8) + 0;
	}

	/* Fold 32-bit sum to 16 bits */
    sum = (sum>>16) + (sum&0xffff);

    sum = (sum>>16) + (sum&0xffff);
    //sum += sum&0xffff0000;

    return sum;
}

