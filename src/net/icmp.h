//-----------------------------------------------------------------------------
// ICMP.H
//
//-----------------------------------------------------------------------------
#ifndef __ICMP_H__
#define __ICMP_H__

void ping_send(UCHAR  * inbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr, UINT len) ;
void dest_unreach_send(UCHAR  * inbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr);
void icmp_rcve(UCHAR  * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len);

#endif
