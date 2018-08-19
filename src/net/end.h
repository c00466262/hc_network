#ifndef __END_H__
#define __END_H__

#define END_NAME_MAX 8          /* The maximum length of a device name */

typedef struct net_funcs
{
	STATUS (*receive)();	/*�ӿڵĽ��պ���*/
	STATUS (*send) ();		/*�ӿڵķ��ͺ���*/
}NET_FUNCS; 


typedef struct end_object
{
	char name[END_NAME_MAX];
	int unit;
	int flags;
	UINT32 inPkts;	/*�����ӿڽ��յ���ȷ����*/
	UINT32 outPkts;	/*�����ӿڷ��͵İ���*/

	UINT32 inPktsCrc;	/*CRC�����*/
	UINT32 inPktsOv;	/*overflow�����*/
	UINT32 inPktsSize;	/*���ȴ����*/
	UINT32 inPktsFifo;	/*FIFO�����*/

	UINT32 outPktsErr;	/*���ʹ����*/
	
	STATUS (*receiveRtn) ();	/*�ӿڵ��ϲ�Э����պ���*/
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
