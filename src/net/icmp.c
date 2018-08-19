//-----------------------------------------------------------------------------
// Copyright (c) 2002 Jim Brady
// Do not use commercially without author's permission
// Last revised August 2002
// Net ICMP.C
//
// This module handles ICMP messages
// Refer to RFC 792, 896, 950, 1122, and 1191
//-----------------------------------------------------------------------------
#include "string.h"

#include "config.h"
#include "net.h"
#include "cksum.h"
#include "ip.h"
#include "icmp.h"

#ifdef ICMP_SUPPORT
//------------------------------------------------------------------------
// This builds a ping response message.  It allocates memory for the
// entire outgoing message, including Eth and IP headers.  See "TCP/IP
// Illustrated, Volume 1" Sect 7.2 for info on Ping messages
//------------------------------------------------------------------------
void ping_send(UCHAR  * inbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr, UINT len) 
{
	ICMP_HEADER  * ping_in;
	ICMP_HEADER  * ping_out;
	UCHAR  * outbuf;

	NET_LOG("ping_send:\r\n",0,0,0,0,0,0);

	outbuf = net_tx_buf;
	// Ping response message payload starts at offset 34	
	outbuf += IP_HEAD_LEN + LINK_HEAD_LEN;

	ping_out = (ICMP_HEADER  *)outbuf;
	ping_in = (ICMP_HEADER  *)inbuf;
	
	ping_out->msg_type = 0;  //echo reply
	ping_out->msg_code = 0;  // echo reply
	ping_out->checksum = 0;  //echorequest=echo reply=0
	ping_out->identifier = ping_in->identifier;    //规定rsp＝req
	ping_out->sequence = ping_in->sequence;  //规定rsp＝req
	
	wordcpy(&ping_out->echo_data, &ping_in->echo_data, len-PING_HEADER_LEN);

	// Compute checksum over the ICMP header plus
	// optional data and insert complement
	ping_out->checksum = htons(~cksum((UCHAR *)ping_out, len));

	ip_send(outbuf, dest_ipaddr, src_ipaddr, ICMP_TYPE, len);
}

//------------------------------------------------------------------------
// This handles incoming ICMP messages.  See "TCP/IP Illustrated,
// Volume 1" Sect 6.2 for discussion of the various ICMP messages
//------------------------------------------------------------------------
void icmp_rcve(UCHAR  * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len)
{
	ICMP_HEADER *icmp;

	icmp = (ICMP_HEADER *)inbuf;

	// IP header has been adjusted if necessary to always be 
	// 20 bytes so message starts at an offset of 34
	// Validate checksum of entire ICMP message incl data 
	if(cksum(inbuf, len) != 0xFFFF)
	{
		return; 
	}

	// Switch on the message type
	switch(icmp->msg_type)
	{
		case 3:
		break;

		case 8:  //echo request
		NET_LOG("icmp_rcve: ping rcved, len = %d\r\n",len,0,0,0,0,0);

		ping_send(inbuf, remote_ip, local_ip, len); 
		break;

		default:
		break;
	}
}

//=======================================================
#ifdef UDP_SUPPORT 
//------------------------------------------------------------------------
// This builds an outgoing ICMP destination port unreachable response
// message.  See See "TCP/IP Illustrated, Volume 1" Sect 6.5.  This
// message is typically sent in response to a UDP message directed
// to a port that has no corresponding application running. 
// Todo: The spec says we should return all options that were in
// the original incoming IP header.  Right now we cut off everything
// after the first 20 bytes. 
//   用于收到udp报文，但是没有相应的应用层程序与之对应，
//   就发送icmp-unreach
//------------------------------------------------------------------------
void dest_unreach_send(UCHAR  * inbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr)
{
	UCHAR  * outbuf;
	ICMP_ERR_HEADER  * icmp;

	// Allocate memory for entire outgoing message
	// including eth and IP haders.  Always 70 bytes

	outbuf = net_tx_buf;
	outbuf += IP_HEAD_LEN + LINK_HEAD_LEN;
		
	icmp = (ICMP_ERR_HEADER  *)outbuf;

	// Fill in ICMP error message header
	icmp->msg_type = 3;   // destination unreachable
	icmp->msg_code = 3;   // port unreachable
	icmp->checksum = 0;
	  
	// Fill in ICMP error message data
	icmp->msg_data = 0;
	       
	// Copy in 20 byte original incoming IP header
	// plus 8 bytes of data
	wordcpy(&icmp->echo_data, inbuf - IP_HEAD_LEN, 28);

	// Compute checksum over the 36 byte long ICMP
	// header plus data and insert complement
	icmp->checksum = ~cksum(outbuf, 36);
	  
	// Forward message to the IP layer
	ip_send(outbuf, dest_ipaddr, src_ipaddr, ICMP_TYPE, 36);
}
#endif

//=====UDP_SUPPORT===========================================
#endif
