/********************************************************************************************
* FILE:	ethEnd.c

* DESCRIPTION:
	以太网层与硬件无关的收发,是eth与IP的接口
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
	HOST接口操作函数
*/
LOCAL NET_FUNCS hostNetFuncs=
{
	hostEndRcv,
	hostEndSend,
};

/*
	HOST接口定义
*/
HOST_DEVICE hostDevice=
{
	{"host",
	0,
	IFF_RUNNING|IFF_UP,
	0,0,0,0,0,0,0,
	NULL,
	&hostNetFuncs},
	NULL,     		/*虚拟交换机端口*/
	&ethDevice		/*以太网端口*/
};

#if 0
/*
	将本机hostend与交换机的虚拟端口相连接
*/
STATUS hostSwvJoin()
{
	hostDevice.pSwvPort = &swvDevice;
	swvDevice.pHostEnd = &hostDevice;
	
	return OK;
}
#endif

/*
	对以太网帧头进行处理，
	
	在交换模式下，由swvEnd.send调用
	在主机模式下，由eth_rcve调用
*/
STATUS hostEndRcv(void *pCookie, UCHAR *inbuf, int len)
{
	ETH_HEADER * eth;
	UINT16  frame_type;

	eth = (ETH_HEADER *)inbuf;
	NET_LOG("hostEndRcv: len=%d\r\n",len,0,0,0,0,0);

	hostDevice.end.inPkts++;
	
	/*只接受广播或者是对本机的单播数据包，其余的不处理*/
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
	//  query_ethchip()接收时已经判断实际接收长度，这里判断以太网包里的长度或类型
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
			/*接收带标签的网管数据*/
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
	添加以太网包头,由ip_send调用:
	在交换模式下，调用swvEnd.receive,然后由swvEnd送给switch模块处理
	在主机模式下，调用eth_send通过以太网口发送
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
		
	/*以太网包的长度最小为64个字节，包括4个字节FCS，所以说，len最小为60字节*/
	if(len < ETHERSMALL)
	{
		memset(outbuf+len, 0, ETHERSMALL-len);
		len = ETHERSMALL;
	}

	/*送给交换机端口swv，swv再送入交换机*/
	if(pHostDevice->pSwvPort)
	{
		/*暂不支持软交换*/
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


