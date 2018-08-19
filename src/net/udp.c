//-----------------------------------------------------------------------------
// Copyright (c) 2002 Jim Brady
// Do not use commercially without author's permission
// Last revised August 2002
// Net UDP.C
//
// This module handles UDP messages
// Refer to RFC 768, 1122
// Also RFC 862 echo, RFC 867 daytime, and RFC 868 time
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "net.h"
#include "ip.h"
#include "cksum.h"
#include "icmp.h"
#include "udp.h"

#ifdef SNMP_SUPPORT
#include "Snmp.h"
#endif

#ifdef TFTP_SUPPORT
#include "tftp.h"
#endif

#ifdef  UDP_SUPPORT 

//------------------------------------------------------------------------
//	UDP Echo service - see RFC 862
// This simply echos what it received back to the sender
//------------------------------------------------------------------------
void udp_echo_service(UCHAR * outbuf, sockaddr_in *remote_addr, sockaddr_in *local_addr, UINT len)
{
	udp_send(outbuf, remote_addr, local_addr, len);
}

//------------------------------------------------------------------------
//	This handles outgoing UDP messages
// See "TCP/IP Illustrated, Volume 1" Sect 11.1 - 11.3
//------------------------------------------------------------------------
void udp_send(UCHAR  * outbuf, sockaddr_in *remote_addr, sockaddr_in *local_addr, UINT len)
{
	UINT32  sum;
	UINT16  result;
	UDP_HEADER  * udp;
	IP_HEADER  * ip;
	UINT16 udp_len_h;/*主机格式的UDP LEN*/

	NET_LOG("udp_send: rport=%d, lport=%d\r\n",remote_addr->sin_port,local_addr->sin_port,0,0,0,0);

	/*add 8 bytes UDP header*/
	outbuf -= UDP_HEAD_LEN;
	/////////////////////////////////////////////////////////////////////////////////////////////////
	udp = (UDP_HEADER  *)(outbuf);
	ip = (IP_HEADER  *)(outbuf - IP_HEAD_LEN);

	// Direct message back to the senders port. 
	udp->dest_port = htons(remote_addr->sin_port);
	udp->src_port = htons(local_addr->sin_port);
	
	udp_len_h = UDP_HEAD_LEN + len;
	udp->length = htons(udp_len_h);
	udp->checksum = 0;

	// Compute checksum including 12 bytes of pseudoheader
	// Must pre-fill 2 items in outbuf to do this
	// Direct message back to senders ip address
	ip->dest_ipaddr = htonl(remote_addr->sin_addr);
	ip->src_ipaddr = htonl(local_addr->sin_addr);

	// Sum src_ipaddr, dest_ipaddr, and entire UDP message 
	sum = (UINT32)cksum(outbuf - 8, 8 + udp_len_h);

	// Add in the rest of pseudoheader which is
	// zero, protocol id, and UDP length
	sum += (UINT32)UDP_TYPE;
	sum += (UINT32)udp_len_h;

	// In case there was a carry, add it back around
	result = (UINT16)(sum + (sum >> 16));
	udp->checksum = htons(~result);

	ip_send(outbuf, remote_addr->sin_addr, local_addr->sin_addr, UDP_TYPE, udp_len_h);
}

//------------------------------------------------------------------------
// This handles incoming UDP messages
// See "TCP/IP Illustrated, Volume 1" Sect 11.1 - 11.3
//------------------------------------------------------------------------
void udp_rcve(UCHAR  * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len)
{
	UINT16  result;
	UDP_HEADER  * udp;
	UINT32  sum;
	
	sockaddr_in remote_addr;	/* remote transport address */
	sockaddr_in local_addr;		/* local transport address */
	udp = (UDP_HEADER  *)inbuf;

	// The IP length "len" should be the same as the redundant length
	// udp->length.  TCP/IP Illustrated, Vol 2, Sect 23.7 says to use the
	// UDP length, unless IP length < UDP length, in which case the frame
	// should be discarded.
	if (len < ntohs(udp->length))
		return;

	// If the checksum is zero it means that the sender did not compute
	// it and we should not try to check it.
	if (ntohs(udp->checksum) != 0)
	{
		// Compute UDP checksum including 12 byte pseudoheader
		// Sum src_ipaddr, dest_ipaddr, and entire UDP message 
		sum = (UINT32)cksum(inbuf - 8, 8 + ntohs(udp->length));

		// Add in the rest of pseudoheader which is
		// zero, protocol id, and UDP length
		sum += (UINT32)UDP_TYPE;
		sum += (UINT32)ntohs(udp->length);

		// In case there was a carry, add it back around
		result = (UINT16)(sum + (sum >> 16));

		if (result != 0xFFFF)
		{
			return;
		}
	}
	// Capture sender's port number and ip_addr
	// to send return message to
	remote_addr.sin_addr = remote_ip;
	remote_addr.sin_port = ntohs(udp->src_port);

	local_addr.sin_addr = local_ip;
	local_addr.sin_port = ntohs(udp->dest_port);

	NET_LOG("udp_rcve: rport=%d, lport=%d\r\n",remote_addr.sin_port,local_addr.sin_port,0,0,0,0);
	
	// See if any applications are on any ports
	switch (ntohs(udp->dest_port))
	{
		case ECHO_PORT:
		{
			// Pass it the payload length
			udp_echo_service(inbuf, &remote_addr, &local_addr, ntohs(udp->length) - 8);
			break;
		}

	#ifdef SNMP_SUPPORT
		case  SNMP_PORT:
		{	
			snmp_rcve(inbuf + UDP_HEAD_LEN, &remote_addr, &local_addr, ntohs(udp->length)-8);
			break;
		}
	#endif	//SNMP_SUPPORT

	#ifdef TFTP_SUPPORT
		case  TFTP_PORT:
		{	
			tftp_server(inbuf + UDP_HEAD_LEN, &remote_addr, &local_addr, ntohs(udp->length)-8);
			break;
		}
	#endif	//TFTP_SUPPORT

		default:
		{
			// If no application is registered to handle incoming
			// UDP message then send ICMP destination unreachable
			dest_unreach_send(inbuf, remote_ip, local_ip);
			break;
		}
	}
}

void udp_init()
{
	
}
#endif
