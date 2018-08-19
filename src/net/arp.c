/*-----------------------------------------------------------------------------
*  FILE:config.h

*  DESCRIPTION:
	
	This module handles ARP messages and ARP resolution and manages
    the ARP cache.But this isn't standard.
	原则:这次用了这个arp表项,下一次我还以为会用它
	
* REVISION HISTORY:
	2010.7.7	lixing	created it.
-----------------------------------------------------------------------------*/

#include "string.h"

#include "config.h"
#include "net.h"
#include "eth.h"
#include "ip.h"
#include "arp.h"
#include "eth.h"
#include "cli.h"
#include "hostEnd.h"

#ifdef ARP_SUPPORT

//============================================================
ARP_CACHE arp_cache[CACHESIZE];
static UCHAR arp_cache_currptr = CACHESIZE - 1;//指到刚刚被替换的arp_cache表相


void arp_init(void)
{
  	memset(arp_cache, 0, sizeof(arp_cache)); 
	arp_cache_currptr = CACHESIZE - 1;//指向最后一个表项
}

//--------------------------------------------------------------------
// This allocates memory for the entire outgoing message,
// including eth and ip headers, then builds an outgoing
// ARP response message
// See "TCP/IP Illustrated, Volume 1" Sect 4.4
//--------------------------------------------------------------------
void arp_send(UCHAR * dest_hwaddr, UINT32 dest_ipaddr, UINT32 src_ipaddr, UINT16 msg_type)
{
	UCHAR * outbuf;
	
	ARP_HEADER * arp;

	NET_LOG("arp_send:\r\n",0,0,0,0,0,0);
	
	outbuf = net_tx_buf;
	
	// Allow 14 bytes for the ethernet header
	outbuf += LINK_HEAD_LEN;

	arp = (ARP_HEADER *)outbuf;
	arp->hardware_type = htons(1);//DIX_ETHERNET; 
	arp->protocol_type = htons(ETHERTYPE_IP);
	arp->hwaddr_len = 6;
	arp->ipaddr_len = 4;               
	arp->message_type = htons(msg_type);

	// My hardware address and IP addresses 
	memcpy(arp->source_hwaddr, my_hwaddr, 6);
	arp->src_ipaddr = htonl(src_ipaddr);
	
	// Destination hwaddr and dest IP addr
	if (msg_type == ARP_REQUEST)
  		memset(arp->dest_hwaddr, 0, 6);
  	else 
		memcpy(arp->dest_hwaddr, dest_hwaddr, 6);
	
	arp->dest_ipaddr = htonl(dest_ipaddr);

	// If request then the message is a brodcast, if a response then
	// send to specified hwaddr ARP payload size is always 28 bytes
	if (msg_type == ARP_REQUEST) 
		hostEndSend(outbuf, broadcast_hwaddr, ETHERTYPE_ARP, 28);
	else
		hostEndSend(outbuf, dest_hwaddr, ETHERTYPE_ARP, 28);
}

//------------------------------------------------------------------------
// Find the ethernet hardware address for the given ip address
// If destination IP is on my subnet then we want the eth
// address	of destination, otherwise we want eth addr of gateway. 
// Look in ARP cache first.  If not found there, send ARP request.
// Return pointer to the hardware address or 

//broadcast_hwaddr if not found//add by lixingfu
//------------------------------------------------------------------------
UCHAR * arp_resolve(UINT32 dest_ipaddr) 
{
	UCHAR i;

	NET_LOG("arp_resolve:\r\n",0,0,0,0,0,0);
	
	//--------------- 1 如果目的IP 不在本机的网段内，取网管的物理地址
	if((dest_ipaddr & my_subnetmask) != (my_ipaddr & my_subnetmask))
	{/*等价与if(dest_ipaddr ^ my_ipaddr) & my_subnetmask))*/
		if (gateway_ipaddr == 0)
		{
			return (NULL);	
		}
		else 
			dest_ipaddr = gateway_ipaddr;
	}
	
	//----------2   See if IP addr of interest is in ARP cache
	//----------从刚刚被替换的arp_cache表项向前找,以便找到最新的表项
	for (i=0; i < CACHESIZE; i++)
	{
		if (arp_cache[arp_cache_currptr].ipaddr == dest_ipaddr)
		{
			NET_LOG("arp_resolve: find dst mac\r\n",0,0,0,0,0,0);

			return (arp_cache[arp_cache_currptr].hwaddr);
		}
		
		//从后向前循环
		if(arp_cache_currptr == 0)
			arp_cache_currptr = CACHESIZE - 1;
		else
			arp_cache_currptr--;
	}
	
	//----------  3 没找到：ARP_REQUEST；waiting_for_arp
	// Not in cache so broadcast ARP request
	NET_LOG("arp_resolve: not found dest mac, send arp request\r\n",0,0,0,0,0,0);
	
	arp_send(broadcast_hwaddr, dest_ipaddr, my_ipaddr, ARP_REQUEST);

	// 没有找到对应的arp时，由于是单线程，所以并不再让需要此arp的数据包等待arpresponse
	return (NULL); 
}
 /*                                                                        
    0                   1                   2                   3           
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |      Hardware  Type           |        Protocol Type          |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |HardwareAddLen |ProtocolAddLen |         Operation             |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                    Sender Hardware Address                    |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |  Sender Hardware Addres       |  Sender Protocol Address      |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |  Sender Protocol Address      |  Target Hardware Address      |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                    Target Hardware Address                    |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                    Target Protocol Address                    |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
                                                                           
    */                                                                        
//------------------------------------------------------------------------
// This handles incoming ARP messages
// 轮转替换arpcache/
// Todo:  Resolve problem of trying to add to a full cache
//------------------------------------------------------------------------
void arp_rcve(UCHAR * inbuf)
{
	UCHAR i; 
	BOOL cached = FALSE;          //已经刷新
	ARP_HEADER * arp;

	NET_LOG("arp_rcve:\r\n",0,0,0,0,0,0);

	arp = (ARP_HEADER *)inbuf;

	//------- 1     检测硬件类型、协议类型
	if ((ntohs(arp->hardware_type) != DIX_ETHERNET) ||//只能识别DIX_ETHERNET
	(ntohs(arp->protocol_type) != ETHERTYPE_IP))
	  return;

	//------ 2   无论问谁，检测ARP报文内源IP，如果arp-cache已有
	//-----------刷新mac地址,并不替换其他的arp表项
	for (i=0; i < CACHESIZE; i++)
	{
		if (arp_cache[arp_cache_currptr].ipaddr == ntohl(arp->src_ipaddr))
		{
			memcpy(&arp_cache[arp_cache_currptr].hwaddr[0], arp->source_hwaddr, 6);
			cached = TRUE;//已经刷新了,就是更改了cache
			break;  
    	}
		//指向前一个被替换的arp_cache表项
		if(arp_cache_currptr == 0)
			arp_cache_currptr = CACHESIZE - 1;
		else
			arp_cache_currptr--;
	}
	//-------  3    检测arp目的IP，如果不是问我 ，不用应答，返回
	// 因此只有问我的人才能进入arp cache
	if (ntohl(arp->dest_ipaddr) != my_ipaddr)
	{
		NET_LOG("arp_rcve: not my ip, dest ip = %s\r\n",(int)inet_ntoa(ntohl(arp->dest_ipaddr)),0,0,0,0,0);
		return;
	}
	//---------4   如果问我，并且没有刷新cache
	//(当然，因为目前cache没有此ip地址)，增加入cache
	if (cached == FALSE)
	{
		//指向将要被替换的arp_cache项
		arp_cache_currptr = (++arp_cache_currptr)%CACHESIZE;
		arp_cache[arp_cache_currptr].ipaddr = ntohl(arp->src_ipaddr);
		memcpy(&arp_cache[arp_cache_currptr].hwaddr[0], &arp->source_hwaddr[0], 6);
	}//end if (cached == FALSE)
	
	//-------- 6   其它机器的arp请求
	if (arp->message_type == htons(ARP_REQUEST))
	{
		// Send ARP response 
		arp_send(arp->source_hwaddr, ntohl(arp->src_ipaddr), ntohl(arp->dest_ipaddr), ARP_RESPONSE);
	}
}
#endif

