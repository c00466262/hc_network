/********************************************************************************************
* FILE:	ethEnd.c

* DESCRIPTION:
	��̫������Ӳ���޹ص��շ�,��eth��IP�Ľӿ�
MODIFY HISTORY:
	2013.4.8            lixingfu   create
********************************************************************************************/

#include "config.h"
#include "eth.h"
#include "ip.h"
#include "arp.h"
#include "hostEnd.h"

#ifdef HCTEL_3AH_OAM_SUPPORT
#include "hctel_3ah_oam.h"
#endif


/*
	HOST�ӿڲ�������
*/
LOCAL NET_FUNCS hostNetFuncs=
{
	hostEndRcv,
	hostEndSend,
};

/*
	HOST�ӿڶ���
*/
HOST_DEVICE hostDevice=
{
	{"host",
	0,
	IFF_RUNNING|IFF_UP,
	0,0,0,0,0,0,0,
	NULL,
	&hostNetFuncs},
	NULL,     		/*���⽻�����˿�*/
	&ethDevice		/*��̫���˿�*/
};

#if 0
/*
	������hostend�뽻����������˿�������
*/
STATUS hostSwvJoin()
{
	hostDevice.pSwvPort = &swvDevice;
	swvDevice.pHostEnd = &hostDevice;
	
	return OK;
}
#endif

/*
	����̫��֡ͷ���д���
	
	�ڽ���ģʽ�£���swvEnd.send����
	������ģʽ�£���eth_rcve����
*/
STATUS hostEndRcv(void *pCookie, UCHAR *inbuf, int len)
{
	ETH_HEADER * eth;
	UINT16  frame_type;

	eth = (ETH_HEADER *)inbuf;
	NET_LOG("hostEndRcv: len=%d\r\n",len,0,0,0,0,0);

	hostDevice.end.inPkts++;
	
	/*ֻ���ܹ㲥�����ǶԱ����ĵ������ݰ�������Ĳ�����*/
	if(!((eth->dest_hwaddr[0] & (UINT8)0x01) ||
		(eth->dest_hwaddr[0]==my_hwaddr[0] && eth->dest_hwaddr[1]==my_hwaddr[1] && 
		eth->dest_hwaddr[2]==my_hwaddr[2] && eth->dest_hwaddr[3]==my_hwaddr[3] && 
		eth->dest_hwaddr[4]==my_hwaddr[4] && eth->dest_hwaddr[5]==my_hwaddr[5])))
	{
		NET_LOG("hostEndRcv: not my mac address\r\n",0,0,0,0,0,0);

		return ERROR;
	}

#ifdef HCTEL_3AH_OAM_SUPPORT
	if(eth->dest_hwaddr[0]==slowPrtlDstAddr[0] && eth->dest_hwaddr[1]==slowPrtlDstAddr[1] && 
	   eth->dest_hwaddr[2]==slowPrtlDstAddr[2] && eth->dest_hwaddr[3]==slowPrtlDstAddr[3] && 
	   eth->dest_hwaddr[4]==slowPrtlDstAddr[4] && eth->dest_hwaddr[5]==slowPrtlDstAddr[5])
	{
		NET_LOG("hostEndRcv: slow protocol pkt!\r\n",0,0,0,0,0,0);
		hctel_oamRcvPkt(inbuf);

		return OK;
	}
#endif	
	
	// Reject frames in IEEE 802 format where Eth type field
	// is used for length.  Todo: Make it handle this format
	//  query_ethchip()����ʱ�Ѿ��ж�ʵ�ʽ��ճ��ȣ������ж���̫������ĳ��Ȼ�����
	if (ntohs(eth->frame_type) < MAX_ETH_LEN)
	{
		NET_LOG("hostEndRcv: error frame_type\r\n",0,0,0,0,0,0);
		return ERROR;
	}

	// Figure out what type of frame it is from Eth header
	// Call appropriate handler and supply address of buffer
	frame_type = ntohs(eth->frame_type);

	NET_LOG("hostEndRcv: frame_type:%x\r\n",frame_type,0,0,0,0,0);
	switch (frame_type)
	{
		case ETHERTYPE_ARP:
		{
			#ifdef ARP_SUPPORT
			
			NET_LOG("hostEndRcv: arp pkt\r\n",0,0,0,0,0,0);
			
			// del 14 byte Ethernet header
			arp_rcve(inbuf + LINK_HEAD_LEN);

			#endif
			
			break;
		}
		
		case ETHERTYPE_IP:
		{
			#ifdef IP_SUPPORT

			NET_LOG("hostEndRcv: ip pkt\r\n",0,0,0,0,0,0);
			
			// del 14 byte Ethernet header			
			ip_rcve(inbuf + LINK_HEAD_LEN);

			#endif

			break;
		}

	#ifdef ETH_ENMP_SUPPORT	
		case ETHERTYPE_VLAN:
		{
			/*���մ���ǩ����������*/
			NET_LOG("hostEndRcv: eth vlan pkt\r\n",0,0,0,0,0,0);
			ethEnmpTagRcv(inbuf + LINK_HEAD_LEN);
			break;
		}		

		case ENMPoETH_TPYE_POLL:
		{
			NET_LOG("hostEndRcv: ethEnmp pkt\r\n",0,0,0,0,0,0);
			
			// del 14 byte Ethernet header			
			ethEnmpRcv(inbuf + LINK_HEAD_LEN);
			break;
		}
	#endif

	#ifdef ETH_ENMP_ARP_REQ_SUPPORT
		case ENMPoETH_TYPE_ARP_RSP:
		{
			NET_LOG("hostEndRcv: ethEnmp arp rsp pkt\r\n",0,0,0,0,0,0);
			ethEnmpArpRspRcv(inbuf + LINK_HEAD_LEN);			
			break;
		}
	#endif

	
	#ifdef ETH_ENMP_ARP_RSP_SUPPORT
		case ENMPoETH_TYPE_ARP_REQ:
		{
			NET_LOG("hostEndRcv: ethEnmp arp req pkt\r\n",0,0,0,0,0,0);
			ethEnmpArpReqRcv(inbuf + LINK_HEAD_LEN);
			
			break;
		}		
	#endif

		default:
			break;
	}

	return OK;
}

/*
	�����̫����ͷ,��ip_send����:
	�ڽ���ģʽ�£�����swvEnd.receive,Ȼ����swvEnd�͸�switchģ�鴦��
	������ģʽ�£�����eth_sendͨ����̫���ڷ���
*/
STATUS hostEndSend(UCHAR * outbuf, UCHAR * dest_hwaddr, UINT16 ptype, int len)
{
	HOST_DEVICE *pHostDevice = &hostDevice;

	ETH_HEADER * eth;

	NET_LOG("hostEndSend: len %d\r\n", len, 0, 0,0,0,0);

	hostDevice.end.outPkts++;
	
	// Add 14 bytes Ethernet header
	outbuf -= LINK_HEAD_LEN;

	eth = (ETH_HEADER *)outbuf;

	memcpy(eth->dest_hwaddr, dest_hwaddr, 6);
	memcpy(eth->source_hwaddr, my_hwaddr, 6); 
	
	eth->frame_type = htons(ptype);

	len += LINK_HEAD_LEN; 
		
	/*��̫�����ĳ�����СΪ64���ֽڣ�����4���ֽ�FCS������˵��len��СΪ60�ֽ�*/
	if(len < ETHERSMALL)
	{
		memset(outbuf+len, 0, ETHERSMALL-len);
		len = ETHERSMALL;
	}

	/*�͸��������˿�swv��swv�����뽻����*/
	if(pHostDevice->pSwvPort)
	{
		/*�ݲ�֧������*/
		#if 0
		SWV_DEVICE *pSwvDevice = pHostDevice->pSwvPort;
		NET_LOG("hostEndSend: sent to swv, len %d\r\n", len, 0, 0,0,0,0);		
		pSwvDevice->end.pFuncTable->receive(pSwvDevice, outbuf, len);
		#endif
	}
	else
	if(pHostDevice->pEthEnd)
	{
		ETH_DEVICE *pEthDevice = pHostDevice->pEthEnd;
		NET_LOG("hostEndSend: sent to eth, len %d\r\n", len, 0, 0,0,0,0);		
		pEthDevice->end.pFuncTable->send(pEthDevice, outbuf, len);
	}

	return ERROR;
}


