#ifndef __HOSTEND_H__
#define __HOSTEND_H__

#include "end.h"

typedef struct
{
	END_OBJ	end;		/* The class we inherit from. */
	
	void* pSwvPort;		/*交换机端口*/
	void* pEthEnd;		/*ETH发送端口*/

}HOST_DEVICE;

extern HOST_DEVICE hostDevice;
//STATUS hostSwvJoin(void);
STATUS hostEndRcv(void *pCookie, UCHAR *inbuf, int len);
STATUS hostEndSend(UCHAR * outbuf, UCHAR * dest_hwaddr, UINT16 ptype, int len);

#endif
