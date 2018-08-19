//-----------------------------------------------------------------------------
// IP.H
//
//-----------------------------------------------------------------------------
#ifndef __IP_H__
#define __IP_H__

#define IP_HEAD_TTL         32
void ip_send(UCHAR * outbuf, UINT32 dest_ipaddr, UINT32 src_ipaddr, UINT8 proto_id, int len);
void ip_rcve(UCHAR  *);

#endif

