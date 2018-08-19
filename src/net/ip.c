//-----------------------------------------------------------------------------
// Copyright (c) 2002 Jim Brady
// Do not use commercially without author's permission
// Last revised August 2002
// Net IP.C
//
// This module is the IP layer
// Refer to RFC 791, 1122, and RFC 815 (fragmentation)
//-----------------------------------------------------------------------------
#include "string.h"

#include "config.h"
#include "net.h"
#include "cksum.h"
#include "arp.h"
#include "hostEnd.h"
#include "ip.h"
#include "icmp.h"

#ifdef UDP_SUPPORT
#include "udp.h"
#endif

#ifdef TCP_SUPPORT
#include "tcp.h"
#endif

#ifdef IP_SUPPORT
//------------------------------------------------------------------------
// This handles outgoing IP datagrams.  It adds the 20 byte IP header
// and checksum then forwards the IP datagram to the Ethernet layer
// for sending. See "TCP/IP Illustrated, Volume 1" Sect 3.2
//------------------------------------------------------------------------

void ip_send(UCHAR * outbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr, UINT8 proto_id, int len)
{
	IP_HEADER  * ip;
	UCHAR  * dest_hwaddr;
	static UINT16  ip_ident = 0;

	NET_LOG("ip_send: to %s, len = %d\r\n",(int)inet_ntoa(dest_ipaddr),len,0,0,0,0);

	// add 20 bytes ip header
	outbuf -= IP_HEAD_LEN;

	ip = (IP_HEADER  *)outbuf;
	ip->ver_len = 0x45;              // IPv4 with 20 byte header
	ip->type_of_service = 0;
	ip->total_length = htons(20 + len);
	ip->identifier = htons(ip_ident++);     // sequential identifier
	ip->fragment_info = 0;           // not fragmented
	ip->time_to_live = IP_HEAD_TTL;  // max hops
	ip->protocol_id = proto_id;      // type of payload
	ip->header_cksum = 0;
	ip->src_ipaddr = htonl(src_ipaddr);

	// Outgoing IP address
	ip->dest_ipaddr = htonl(dest_ipaddr);

	// Compute and insert complement of checksum of ip header
	// Outgoing ip header length is always 20 bytes
	ip->header_cksum = htons(~cksum(outbuf, 20));

	// Use ARP to get hardware address to send this to
	//从arp_cache 获得硬件地址
	dest_hwaddr = arp_resolve(dest_ipaddr);

	//没有从arp_cache获得硬件地址,丢弃本包
	//arp去获得硬件地址后更新arp_cache
	// Null means that the ARP resolver did not find the IP address
	// in its cache so had to send an ARP request
	if((dest_hwaddr == NULL) ||
		
		(dest_hwaddr[0]== 0 && dest_hwaddr[1]== 0 && dest_hwaddr[2]== 0 &&
		dest_hwaddr[3]== 0 && dest_hwaddr[4]== 0 && dest_hwaddr[5]== 0) ||
		
		(dest_hwaddr[0]== 0xFF && dest_hwaddr[1]== 0xFF && dest_hwaddr[2]== 0xFF &&
		dest_hwaddr[3]== 0xFF && dest_hwaddr[4]== 0xFF && dest_hwaddr[5]== 0xFF))
		
		return;

	/*通过HOSTEND发送，HOSTEND根据判断发送到SWITCH或者是ETHEND*/
	hostEndSend(outbuf, dest_hwaddr, ETHERTYPE_IP, 20 + len);
}

  /*                                                                        
    0                   1                   2                   3           
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1         
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |Version|  IHL  |Type of Service|          Total Length         |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |         Identification        |Flags|      Fragment Offset    |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |  Time to Live |    Protocol   |         Header Checksum       |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                       Source Address                          |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                    Destination Address                        |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
   |                    Options                    |    Padding    |        
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+        
                                                                            
                    Example Internet Datagram Header                        
  */                                                                        


//------------------------------------------------------------------------
// This handles incoming IP datagrams from the Ethernet layer
// See "TCP/IP Illustrated, Volume 1" Sect 3.2
//------------------------------------------------------------------------
void ip_rcve(UCHAR  * inbuf)
{
	IP_HEADER  * ip;
	UINT  header_len, payload_len;
	UINT32 remote_ip, local_ip;
	UINT16 sum;

	NET_LOG("ip_rcve:\r\n",0,0,0,0,0,0);
		
	ip = (IP_HEADER  *)inbuf;
	
/*------1.判断地址---------------------------------------*/
	if (ntohl(ip->dest_ipaddr) != my_ipaddr)
	{
		NET_LOG("ip_rcve: not my ip, dest ip = %s\r\n",(int)inet_ntoa(ntohl(ip->dest_ipaddr)),0,0,0,0,0);
		return;
	}
	
//------2.Validate checksum of ip header------------------*/
	header_len = 4 * (0x0F & ip->ver_len);
	payload_len = ntohs(ip->total_length) - header_len;

	sum = cksum(inbuf, header_len);
	if(sum != 0xFFFF)
	{
		NET_LOG("ip_rcve: cksum error\r\n",0,0,0,0,0,0);	
		return; 
	}
	
//------3.判断版本------------------------------------------*/
	// Make sure incoming message is IP version 4
	if ((ip->ver_len >> 4) != 0x04)
	{
		return;
	}
	
//-------4.判断碎片，不支持ip分片---------------------------*/
	// Make sure incoming message is not fragmented because
	// we cannot handle fragmented messages
	if ((ntohs(ip->fragment_info) & 0x3FFF) != 0)
	{
		return; 
	}
	
//-------5.到目前已经收到合法ip报文---------------------------*/

	//------4.0   我们不使用  options, and do not forward messages,
	// 但是有可能收到带options的报文，要把options删除
	if (header_len > 20)
	{
		memmove(inbuf + IP_HEAD_LEN, inbuf + header_len, payload_len);
		// Adjust info to reflect the move
		header_len = 20;
		ip->ver_len = 0x45;
		ip->total_length = htons(20 + payload_len);
	}

	remote_ip = ntohl(ip->src_ipaddr);
	local_ip = ntohl(ip->dest_ipaddr);

	NET_LOG("ip_rcve: remote ip %s\r\n",(int)inet_ntoa(remote_ip),0,0,0,0,0);

//-------6. 检查 protocol ID  进行处理---------------------------*/
	// See "TCP/IP Illustrated, Volume 1" Sect 1.7
	// RFC 791 for values for various protocols
	switch (ip->protocol_id)
	{
	#ifdef ICMP_SUPPORT  
		case ICMP_TYPE:
		{
			NET_LOG("ip_rcve: icmp rcved\r\n",0,0,0,0,0,0);	

			icmp_rcve(inbuf + IP_HEAD_LEN, remote_ip, local_ip, payload_len);
			break;
		}
	#endif

	#ifdef UDP_SUPPORT  
		case UDP_TYPE:
		{
			NET_LOG("ip_rcve: udp rcved\r\n",0,0,0,0,0,0);	

			udp_rcve(inbuf + IP_HEAD_LEN, remote_ip, local_ip, payload_len);

			break;
		}
	#endif //UDP_SUPPORT

	#ifdef TCP_SUPPORT  
		case TCP_TYPE:
		{
			NET_LOG("ip_rcve: tcp rcved\r\n",0,0,0,0,0,0);
		
			tcp_rcve(inbuf + IP_HEAD_LEN, remote_ip, local_ip, payload_len);

			break;
		}
	#endif //TCP_SUPPORT

		default:
		break;
	}
}
#endif

