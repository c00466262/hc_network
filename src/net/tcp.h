#ifndef __TCP_H__
#define __TCP_H__

#include "net.h"
//-----------------------------------------------------------------------------
// TCP.H
//
//-----------------------------------------------------------------------------
#define TCP_WINDOW        1500

// TCP states
#define STATE_CLOSED				0
#define STATE_LISTEN				1
#define STATE_SYN_RCVD				2
#define STATE_ESTABLISHED			3
#define STATE_CLOSE_WAIT			4
#define STATE_LAST_ACK				5
#define STATE_FIN_WAIT_1			6
#define STATE_FIN_WAIT_2			7
#define STATE_CLOSING				8
#define STATE_TIME_WAIT				9


// TCP flag bits
#define FLG_FIN						0x0001
#define FLG_SYN						0x0002
#define FLG_RST						0x0004
#define FLG_PSH						0x0008
#define FLG_ACK						0x0010
#define FLG_URG						0x0020


// Miscellaneous
#define NO_CONNECTION  			MAX_OF_TCP_CONNS

#define TCP_TIMEOUT				4		// = 2 seconds,用于重传TCP,500ms检测一次
#define INACTIVITY_TIME			480		// = 2分钟没有任何数据，TCP连接直接关闭,250ms检测一次

extern TCP_CONNECTION conxn[];
extern UINT32 initial_sequence_nr;

void tcp_send(UCHAR* outbuf, UINT nr, sockaddr_in *remote_addr, sockaddr_in *local_addr, UINT16 flags, BOOL opt, UINT len);
void tcp_rcve(UCHAR * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len);
void tcp_retransmit(void);
void tcp_inactivity(void);
void tcp_init(void);

#endif
