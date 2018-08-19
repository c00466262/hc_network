#ifndef __END_H__
#define __END_H__

#define END_NAME_MAX 8          /* The maximum length of a device name */

typedef struct net_funcs
{
	STATUS (*receive)();	/*接口的接收函数*/
	STATUS (*send) ();		/*接口的发送函数*/
}NET_FUNCS; 


typedef struct end_object
{
	char name[END_NAME_MAX];
	int unit;
	int flags;
	UINT32 inPkts;	/*经过接口接收的正确包数*/
	UINT32 outPkts;	/*经过接口发送的包数*/

	UINT32 inPktsCrc;	/*CRC错误包*/
	UINT32 inPktsOv;	/*overflow错误包*/
	UINT32 inPktsSize;	/*长度错误包*/
	UINT32 inPktsFifo;	/*FIFO错误包*/

	UINT32 outPktsErr;	/*发送错误包*/
	
	STATUS (*receiveRtn) ();	/*接口的上层协议接收函数*/
	NET_FUNCS *pFuncTable;	/* Function table. */
}END_OBJ;

#define	IFF_UP		0x01	/* interface is up */
#define	IFF_RUNNING	0x40	/* resources allocated */

#define	END_FLAGS_CLR(pEnd,clrBits) \
            ((pEnd)->flags &= ~(clrBits))

#define	END_FLAGS_SET(pEnd,setBits) \
            ((pEnd)->flags |= (setBits))

#define	END_FLAGS_GET(pEnd) \
	    ((pEnd)->flags)

#define END_RCV_RTN_CALL(pEnd,inbuf,len) \
	    if ((pEnd)->receiveRtn) \
		{ \
		(pEnd)->receiveRtn ((pEnd), inbuf,len); \
		}

#endif	
