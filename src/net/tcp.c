//-----------------------------------------------------------------------------
// Copyright (c) 2002 Jim Brady
// Do not use commercially without author's permission
// Last revised August 2002
// Net TCP.C
//
// This module handles TCP segments
// Refer to RFC 793, 896, 1122, 1323, 2018, 2581
//
// A "connection" is a unique combination of 4 items:  His IP address,
// his port number, my IP address, and my port number.
//
// Note that a SYN and a FIN count as a byte of data, but a RST does
// not count. Neither do any of the other flags.
// See "TCP/IP Illustrated, Volume 1" Sect 17.3 for info on flags

/*
	20121115 李幸福增加:tcp_send如果len大于以太网包的最大长度，则分多个TCP包发送
*/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>		// toupper
#include "config.h"
#include "net.h"
#include "cksum.h"
#include "ip.h"
#include "tcp.h"
#include "http.h"


#ifdef TCP_SUPPORT

UINT32 initial_sequence_nr;

// 当前的连接信息These structures keep track of connection information
TCP_CONNECTION conxn[MAX_OF_TCP_CONNS];

/*
	Options: MSS (4 bytes), NOPS (2 bytes), Selective ACK (2 bytes) 
	在首次协商时使用	
*/
static UCHAR tcp_opt[] = 
{
	0x02,        //option功能代号2:最大报文长度//
	0x04,        //mss总长度：4bytes
	0x05, 0xB4,  //mss=1460
	0x01, 0x01,  //
	0x04, 0x02
}; //sack-permitted:4   sack-permitted长度：2byte

//------------------------------------------------------------------------
//  Initialize variables declared in this module
//
//------------------------------------------------------------------------
void tcp_init(void)
{
   memset(&conxn, 0, sizeof(conxn));
   initial_sequence_nr = 1;
}

/*
	TCP发送
	如果outbuf为空，则从net_tx_buf取,否则,TCP向outbuf前填充
	如果nr有效，则remote_socket和local_socket无效
	如果nr无效，则remote_socket和local_socket有效
	opt表示是否需要option头，否则只是20个字节
	len表示数据的长度

	
	如果len大于以太网包的最大长度，则分多个TCP数据流发送
	以太网包最大长度为1514,以太网头部长度为14,IP层头部长度为20,TCP为20
	所以TCP数据最大长度为1514-14-20-20=1460
*/
#define MAX_TCP_DATA_LEN	(MAX_ETH_LEN-LINK_HEAD_LEN-IP_HEAD_LEN-TCP_HEAD_LEN)

void tcp_send(UCHAR* outbuf, 
				UINT nr, 
				sockaddr_in *remote_socket, 
				sockaddr_in *local_socket, 
				UINT16 flags, 
				BOOL opt, 
				UINT len)
{
	UINT32 sum;
	UINT result;
	UINT hdr_len;
	
	IP_HEADER * ip;
	TCP_HEADER * tcp;

	/*TCP分片使用*/
	UINT frag_len;		/*分片长度*/
	UCHAR* frag_outbuf;	/*分片后的下一个outbuf*/


	/*判断TCP头的长度*/
	if(opt)
		hdr_len = TCP_HEAD_LEN+sizeof(tcp_opt);
	else
		hdr_len = TCP_HEAD_LEN;
	
	// Allocate memory for entire outgoing message including
	// eth & IP headers 

	/*如果没有发送数据，说明只是发送TCP的包头信息，包括opt等*/
	if(outbuf == NULL)
	{
		outbuf = net_tx_buf + LINK_HEAD_LEN + IP_HEAD_LEN + TCP_HEAD_LEN;
	}		

	/*分片发送超长的TCP数据流,每次发送len长度*/
	frag_len = len;
	frag_outbuf = outbuf;

	do	
	{
		if(frag_len > MAX_TCP_DATA_LEN)
		{
			outbuf = frag_outbuf;
			
			frag_outbuf += MAX_TCP_DATA_LEN;/*后移到下一个要发送的缓冲区*/
			frag_len -= MAX_TCP_DATA_LEN;
			len = MAX_TCP_DATA_LEN;
		}
		else
		{
			outbuf = frag_outbuf;		
			len = frag_len;
			frag_len = 0;			
		}

		outbuf -= hdr_len;
			
		tcp = (TCP_HEADER  *)(outbuf);
		ip = (IP_HEADER  *)(outbuf - IP_HEAD_LEN);
		
		// If no connection, then message is probably a reset
		// message which goes back to the sender
		// Otherwise, use information from the connection.
		if (nr == NO_CONNECTION)
		{
			NET_LOG("tcp_send: remote addr=%s, port=%d, local port=%d, hdrlen=%d, flag=%04x\r\n",
			(int)inet_ntoa(remote_socket->sin_addr),remote_socket->sin_port,local_socket->sin_port,hdr_len, flags,0);

			tcp->src_port = htons(local_socket->sin_port);
			tcp->dest_port = htons(remote_socket->sin_port);
			tcp->sequence = 0;
			tcp->ack_number = 0;
		}
		else
		if (nr < MAX_OF_TCP_CONNS)
		{
			NET_LOG("tcp_send: nr=%d, hdrlen=%d, flag=%04x, len=%d\r\n",
				nr,hdr_len, flags, len,0,0);

			remote_socket = &conxn[nr].his_addr;
			local_socket = &conxn[nr].my_addr;
			
			// This message is to connected port
			tcp->src_port = htons(conxn[nr].my_addr.sin_port);
			tcp->dest_port = htons(conxn[nr].his_addr.sin_port);
			tcp->sequence = htonl(conxn[nr].my_sequence);
			tcp->ack_number = htonl(conxn[nr].his_sequence);
		}

		// Total segment length = header length

		// Insert header len
		tcp->flags = htons(hdr_len << 10) | htons(flags);
		tcp->window = TCP_WINDOW;
		tcp->checksum = 0;
		tcp->urgent_ptr = 0;

		// Sending SYN with header options
		if (opt)
		{
			memcpy(&tcp->options, tcp_opt, sizeof(tcp_opt));
		}

		// Compute checksum including 12 bytes of pseudoheader
		// Must pre-fill 2 items in ip header to do this
		ip->dest_ipaddr = htonl(remote_socket->sin_addr);
		ip->src_ipaddr = htonl(local_socket->sin_addr);

		// Sum source_ipaddr, dest_ipaddr, and entire TCP message 
		sum = (UINT32)cksum((UCHAR *)tcp - 8, 8 + hdr_len + len);

		// Add in the rest of pseudoheader which is
		// protocol id and TCP segment length
		sum += (UINT32)TCP_TYPE;
		sum += (UINT32)(hdr_len+len);

		// In case there was a carry, add it back around
		result = (UINT16)(sum + (sum >> 16));
		tcp->checksum = htons(~result);

		/*增加 my_sequence*/
		conxn[nr].my_sequence += len;

		ip_send(outbuf, remote_socket->sin_addr, local_socket->sin_addr, TCP_TYPE, hdr_len+len);

		// (Re)start TCP retransmit timer
		conxn[nr].timer = TCP_TIMEOUT;
	}while(frag_len > 0);
}

/*
    0                   1                   2                   3   
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Source Port          |       Destination Port        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Acknowledgment Number                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Data |           |U|A|P|R|S|F|                               |
   | Offset| Reserved  |R|C|S|S|Y|I|            Window             |
   |       |           |G|K|H|T|N|N|                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Checksum            |         Urgent Pointer        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             data                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                            TCP Header Format

                                    
                              +---------+ ---------\      active OPEN  
                              |  CLOSED |            \    -----------  
                              +---------+<---------\   \   create TCB  
                                |     ^              \   \  snd SYN    
                   passive OPEN |     |   CLOSE        \   \           
                   ------------ |     | ----------       \   \         
                    create TCB  |     | delete TCB         \   \       
                                V     |                      \   \     
                              +---------+            CLOSE    |    \   
                              |  LISTEN |          ---------- |     |  
                              +---------+          delete TCB |     |  
                   rcv SYN      |     |     SEND              |     |  
                  -----------   |     |    -------            |     V  
 +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
 |         |<-----------------           ------------------>|         |
 |   SYN   |                    rcv SYN                     |   SYN   |
 |   RCVD  |<-----------------------------------------------|   SENT  |
 |         |                    snd ACK                     |         |
 |         |------------------           -------------------|         |
 +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
   |           --------------   |     |   -----------                  
   |                  x         |     |     snd ACK                    
   |                            V     V                                
   |  CLOSE                   +---------+                              
   | -------                  |  ESTAB  |                              
   | snd FIN                  +---------+                              
   |                   CLOSE    |     |    rcv FIN                     
   V                  -------   |     |    -------                     
 +---------+          snd FIN  /       \   snd ACK          +---------+
 |  FIN    |<-----------------           ------------------>|  CLOSE  |
 | WAIT-1  |------------------                              |   WAIT  |
 +---------+          rcv FIN  \                            +---------+
   | rcv ACK of FIN   -------   |                            CLOSE  |  
   | --------------   snd ACK   |                           ------- |  
   V        x                   V                           snd FIN V  
 +---------+                  +---------+                   +---------+
 |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
 +---------+                  +---------+                   +---------+
   |                rcv ACK of FIN |                 rcv ACK of FIN |  
   |  rcv FIN       -------------- |    Timeout=2MSL -------------- |  
   |  -------              x       V    ------------        x       V  
    \ snd ACK                 +---------+delete TCB         +---------+
     ------------------------>|TIME WAIT|------------------>| CLOSED  |
                              +---------+                   +---------+

                      TCP Connection State Diagram
                               Figure 6.
*/



//------------------------------------------------------------------------
// This handles incoming TCP messages and manages the TCP state machine
// Note - both the SYN and FIN flags consume a sequence number.
// See "TCP/IP Illustrated, Volume 1" Sect 18.6 for info on TCP states
// See "TCP/IP Illustrated, Volume 1" Sect 17.3 for info on flags
//------------------------------------------------------------------------
void tcp_rcve(UCHAR * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len)
{
	UINT8 i, j;
	UINT nr=0;  //  当前的连接号:1-MAX_OF_TCP_CONNS，第nr个连接
	UINT result, header_len, data_len;  //当前连接TCP头
	TCP_HEADER * tcp;   //当前连接TCP头
	UINT32 sum;          //代码和

	sockaddr_in remote_socket;	/* remote transport address */
	sockaddr_in local_socket;		/* local transport address */

	NET_LOG("tcp_rcve:\r\n",0,0,0,0,0,0);
	
	//=================  0               ============================
	// IP header is always 20 bytes so message starts at index 34      
	tcp = (TCP_HEADER *)inbuf;

	//===============================================================
	//             1   计算代码和			   
	// Compute TCP checksum including 12 byte pseudoheader
	// Sum source_ipaddr, dest_ipaddr, and entire TCP message 
	sum = (UINT32)cksum(inbuf - 8, 8 + len);

	// Add in the rest of pseudoheader which is
	// protocol id and TCP segment length
	sum += (UINT32)0x0006;     
	sum += (UINT32)len;

	// In case there was a carry, add it back around
	result = (UINT16)(sum + (sum >> 16));

	if (result != 0xFFFF)
	{
		NET_LOG("tcp_rcve: cksum error\r\n",0,0,0,0,0,0);	
		return;
   	}
   
	//===============================================================
	//                 2     检查端口
	//      目前，只支持HTTP, 非HTTP端口，复位,没有连接
	// Capture sender's IP address and port number

	remote_socket.sin_addr = remote_ip;
	remote_socket.sin_port = ntohs(tcp->src_port);

	local_socket.sin_addr = local_ip;
	local_socket.sin_port = ntohs(tcp->dest_port);

	NET_LOG("tcp_rcve: remote port %d, my port %d\r\n",remote_socket.sin_port,local_socket.sin_port,0,0,0,0);

	if (ntohs(tcp->dest_port) != HTTP_PORT)	
  	{
    	tcp_send(NULL, NO_CONNECTION, &remote_socket, &local_socket, FLG_RST, FALSE, 0);
    	return;
   	}
	
	//----------------------------------------------------------------
	//           2.1  如果 是已经建立的连接中的一个，nr = i 当前的连接号////////
	// See if the TCP segment is from someone we are already connected to. 
	for (i=0; i < MAX_OF_TCP_CONNS; i++)    //最大MAX_OF_TCP_CONNS个连接
   	{
   		if(!conxn[i].valid)
   		{
			continue;
   		}
		
    	if ((remote_socket.sin_addr == conxn[i].his_addr.sin_addr) && 
			(remote_socket.sin_port == conxn[i].his_addr.sin_port))
    	{
      		nr = i;

			NET_LOG("tcp_rcve: find exist conn[%d]\r\n",nr,0,0,0,0,0);

      		break;
      	}
   	}

	//----------------------------------------------------------------
  	//   2.2  如果不是已经建立的连接中的一个,如果是SYN就分配一个暂时的连接号
  	//   包括一些非法的报文，在后面判断如果非法，再释放////////////////////////
	//----------------------------------------------------------------
  	if (i == MAX_OF_TCP_CONNS)
	{
		if (ntohs(tcp->flags) & FLG_SYN)//如果是请求连接
      	{
 			//-------  SYN 找第一个没有用的连接进入STATE_LISTEN
        	// Find first unused connection (one with IP = 0) 
        	for (j=0; j < MAX_OF_TCP_CONNS; j++)
        	{
          		if (!conxn[j].valid)
          		{
            		nr = j;
					memset(&conxn[nr], 0, sizeof(TCP_CONNECTION));
					conxn[nr].valid = TRUE;
            		// Initialize new connection
            		conxn[nr].state = STATE_LISTEN;
					
					NET_LOG("tcp_rcve: SYN create new conn %d\r\n",nr,0,0,0,0,0);
            		break;
          		}
        	}
			
        //-----------如果连接占满了，丢弃SYN
        	if (j == MAX_OF_TCP_CONNS) 
        	{
				NET_LOG("tcp_rcve: no idle conn\r\n",0,0,0,0,0,0);        	
        		return;
        	}
      	}
   	// else 不是SYN不予理睬
   	}

	//===============================================================
  	//             3    防护措施
  	// By now we should have a connection number in range of 0-4
  	// Do a check to avoid any chance of exceeding size of struct

	//----------------------------------------------------------------
	//            3.1    防止连接号（nr）超出范围
	//----------------------------------------------------------------
  	if (nr >= MAX_OF_TCP_CONNS)
   	{
    	return;
   	}
	NET_LOG("tcp_rcve: nr = %d\r\n",nr,0,0,0,0,0); 

	/*置连接超时计数器*/
	conxn[nr].inactivity = INACTIVITY_TIME;
	
	//----------------------------------------------------------------
	//         3.2  防止序列号（tcp->sequence）溢出
	//----------------------------------------------------------------
	if (ntohl(tcp->sequence) > 0xFFFFFF00L) 
  	{
		conxn[nr].valid = 0;
		NET_LOG("tcp_rcve: sequence overflow\r\n",0,0,0,0,0,0);

		tcp_send(NULL, NO_CONNECTION, &remote_socket, &local_socket, FLG_RST, FALSE, 0);
		return;
  	}

	//===============================================================
	//                4     预处理:异常状态处理
	// 收到RST, SYN, 数据报ACK没有置位，不用考虑目前的TCP状态机
  
 	//----------------------------------------------------------------
  	//         4.1  如果收到RST，不依赖任何状态，
  	//         没有收到数据,因此不ACK，而是直接关闭连接
	//----------------------------------------------------------------

  	if (ntohs(tcp->flags) & FLG_RST)
  	{
    //An RST does not depend on state at all.  And it does
    // not count as data so do not send an ACK here.  Close connection
		NET_LOG("tcp_rcve: RST\r\n",0,0,0,0,0,0);    
    	conxn[nr].valid = 0;
    	return;
   	}
	//----------------------------------------------------------------
 	//      4.2  收到SYN，只对STATE_LISTEN,STATE_CLOSED,否则发reset	
	//----------------------------------------------------------------
	else 
	if (ntohs(tcp->flags) & FLG_SYN)
	{
	 	if ((conxn[nr].state != STATE_LISTEN) &&(conxn[nr].state != STATE_CLOSED))
		{
			NET_LOG("tcp_rcve: SYN ,but conn %d not LISTEN & CLOSED\r\n",0,0,0,0,0,0); 

	    	conxn[nr].valid = 0;
			
			NET_LOG("tcp_rcve: send RST\r\n",0,0,0,0,0,0); 			
		  	tcp_send(NULL, NO_CONNECTION, &remote_socket, &local_socket, FLG_RST, FALSE, 0);
		  	return;		
		}
	}
	//----------------------------------------------------------------
	//       4.3  如果收到报文，ACK没有置位，丢弃但不reset
	//           如果不是SYN或RST，一定ACK==1,否则不对，
	//----------------------------------------------------------------
	else 
	if ((ntohs(tcp->flags) & FLG_ACK) == 0)
	{
		// Incoming segments except SYN or RST must have ACK bit set
	 	// See TCP/IP Illustrated, Vol 2, Page 965
 		// Drop segment but do not send a reset
		NET_LOG("tcp_rcve: ACK, return\r\n",0,0,0,0,0,0); 
		
		return;
	}
	//----------------------------------------------------------------
	//          4.4 没有异常   
  	// Compute length of header including options, and from that
  	// compute length of actual data
	//----------------------------------------------------------------
	header_len =  (ntohs(tcp->flags) & 0xF000) >> 10;   //应该==固定值20
  	data_len = len - header_len;

	NET_LOG("tcp_rcve: head len = %d, data len = %d\r\n",header_len,data_len,0,0,0,0); 
	
	//===============================================================
  	//                5  信息处理:根据状态机
  	// 
	//===============================================================
  	switch (conxn[nr].state)
  	{
			//----------------------------------------------------------------
			//        5. 1    STATE_LISTEN/STATE_CLOSED 
			//----------------------------------------------------------------
	    case STATE_CLOSED:
	    case STATE_LISTEN:
   			NET_LOG("tcp_rcve: conn[%d] CLOSED or LISTEN\r\n",nr,0,0,0,0,0); 
			
	   	//--------5.1.1    是FLG_SYN且FLG_ACK== 0 :说明刚收到主叫的SYN
	    	if ((ntohs(tcp->flags) & FLG_SYN) && ((ntohs(tcp->flags) & FLG_ACK) == 0))
	    	{
				NET_LOG("tcp_rcve: SYN, not ACK\r\n",0,0,0,0,0,0); 

			//-----5.1.1.1 获取这个TCP的连接信息，//////////
				//若是STATE_CLOSED状态，则转移到STATE_LISTEN状态
				conxn[nr].valid = TRUE;

				conxn[nr].his_addr.sin_addr = remote_socket.sin_addr;
				conxn[nr].his_addr.sin_port = remote_socket.sin_port;

				conxn[nr].my_addr.sin_addr = local_socket.sin_addr;
				conxn[nr].my_addr.sin_port = local_socket.sin_port;

				conxn[nr].state = STATE_LISTEN;
				conxn[nr].his_sequence = 1 + ntohl(tcp->sequence);
				conxn[nr].his_ack = ntohl(tcp->ack_number);

			//-----5.1.1.2  使用系统时钟－随机－产生自己的sequence 
				conxn[nr].my_sequence = initial_sequence_nr;
				initial_sequence_nr += 64000L;

			//-----5.1.1.3  响应：SYN ACK
				//此后，client将连续发送两个报文：ACK和HTTP GET
				NET_LOG("tcp_rcve: send SYN & ACK\r\n",0,0,0,0,0,0);
	
				tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_SYN | FLG_ACK, TRUE, 0);

				// My SYN flag increments my sequence number
				// My sequence 永远指向下一个将要发送的字节，因此收到的ack号
				// 应该等于 my sequence 
				conxn[nr].my_sequence++;

			//----5.1.1.4  状态再转移到STATE_SYN_RCVD
				conxn[nr].state = STATE_SYN_RCVD;
	      	}
	    //--------5.1.2    非FLG_SYN且FLG_ACK== 0:复位，释放
			else 
	      	{
			// Sender is out of sync so send reset
				NET_LOG("tcp_rcve: Sender is out of sync so send reset\r\n",0,0,0,0,0,0); 
				conxn[nr].valid = FALSE;
				
				NET_LOG("tcp_rcve: send RST\r\n",0,0,0,0,0,0);
			
				tcp_send(NULL, NO_CONNECTION, &remote_socket, &local_socket, FLG_RST, FALSE, 0);   
	      	} 
			break;


			//------------------------------------------------------------------
		 	//             5. 2  STATE_SYN_RCVD  
		 	//------------------------------------------------------------------
	    case STATE_SYN_RCVD:
   			NET_LOG("tcp_rcve: conn[%d] CLOSED or LISTEN\r\n",nr,0,0,0,0,0); 

	    	// He may already be sending me data - should process it
	    	conxn[nr].his_sequence += data_len;
	    	conxn[nr].his_ack = ntohl(tcp->ack_number);

	    	//.......................................
	    	//     5.2.1 收到FLG_FIN，
	    	//     转移到STATE_CLOSE_WAIT  
	    	//     状态转移STATE_LAST_ACK
	    	//.......................................
	    	if (ntohs(tcp->flags) & FLG_FIN)
			{
	     		// His FIN counts as a byte of data
			 	//----5.2.1.1  发ACK, 转移到STATE_CLOSE_WAIT 
	      		conxn[nr].his_sequence++;
				
				NET_LOG("tcp_rcve: FIN, send ACK ,to CLOSE_WAIT\r\n",0,0,0,0,0,0);

	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_ACK, FALSE, 0);
	      		conxn[nr].state = STATE_CLOSE_WAIT;
	            
			 	//----5.2.1.2  按照标准：应该等待上层应用程序关闭后，再关闭TCP
			 	// 实际实现：简化处理，直接发FIN+ACK,状态转移STATE_LAST_ACK
				NET_LOG("tcp_rcve: send FIN and ACK,to LAST_ACK\r\n",0,0,0,0,0,0);
	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_FIN | FLG_ACK, FALSE, 0);
	      		conxn[nr].my_sequence++;   // For my FIN
	      		conxn[nr].state = STATE_LAST_ACK;
	      	}
			//.................................................
	    	//    5.2.2 对方收到我的SYN, 返回ACK
	    	//     状态转移到STATE_ESTABLISHED
			//..................................................
			else 
			if (ntohl(tcp->ack_number) == conxn[nr].my_sequence)
	      	{
	      		conxn[nr].state = STATE_ESTABLISHED;
				NET_LOG("tcp_rcve: ACK,to ESTABLISHED\r\n",0,0,0,0,0,0);
	      		// If sender sent data ignore it and he will resend
	     		 // 不要发送ACK，因为我们没有收到数据 ...... 
	      		// 等待客户端给我们发送数据
	      	}
	    	break;   //-----end of STATE_SYN_RCVD
			//----------------------------------------------------
	    	//        5.3     STATE_ESTABLISHED
	    	//----------------------------------------------------
		case STATE_ESTABLISHED:
   			NET_LOG("tcp_rcve: conn[%d] ESTABLISHED\r\n",nr,0,0,0,0,0); 

	    	conxn[nr].his_ack = ntohl(tcp->ack_number);

			//..................................................................
	    	//      5.3.1 收到FLG_FIN   
	    	//    状态转移STATE_CLOSE_WAI
	    	//    状态转移STATE_LAST_ACK
			//..................................................................
	    	if (ntohs(tcp->flags) & FLG_FIN)
			{
			  	//---- 5.3.1.1  状态转移STATE_CLOSE_WAI 
			  	conxn[nr].his_sequence++;
				
	      		NET_LOG("tcp_rcve: FIN, sendd ACK,toCLOSE_WAIT\r\n",0,0,0,0,0,0);
				
	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_ACK, FALSE, 0);
	      		conxn[nr].state = STATE_CLOSE_WAIT;
	      
			 	//----5.3.1.2  按照标准：应该等待上层应用程序关闭后，再关闭TCP
			 	// 实际实现：简化处理，直接发FIN+ACK,状态转移STATE_LAST_ACK
	      		NET_LOG("tcp_rcve: FIN, send FIN and ACK,toLAST_ACK\r\n",0,0,0,0,0,0);
	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_FIN | FLG_ACK, FALSE, 0);
	      		conxn[nr].my_sequence++;   // For my FIN
	      		conxn[nr].state = STATE_LAST_ACK;
	      	}
			//..................................................................
			//      5.3.2 收到TCP   发ACK,立即将数据发送到应用层
			//..................................................................
			else
			if (data_len != 0)
	      	{
				conxn[nr].his_sequence += data_len;

		    	NET_LOG("tcp_rcve: ht, send ACK and to APP\r\n",0,0,0,0,0,0);

	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_ACK, FALSE, 0); 		// Send ACK

				// 将指针送给上层，HTTP_SERVR 负责增加 sequence
	      		// sending so don't worry about it here
			#ifdef HTTP_SUPPORT
				result = http_rcve(inbuf+header_len, nr, data_len);
			#endif
			}
			//疑问:收到的ack没有通知应用层，应用层无法利用此信息
			//应用层处理方法:多次连续发送，不管ack与否///////
			//引起的限制:每次索取的网页比较少，否则会丢失//////
		  	else
		  	{
				if (ntohl(tcp->ack_number) < conxn[nr].my_sequence)
				{
			    	NET_LOG("tcp_rcve: low_ack\r\n",0,0,0,0,0,0);		
				}
		  	}
		  	break;

			//------------------------------------------------------------------
	    	//        5. 4 STATE_CLOSE_WAIT(被动关闭端)
	    	//   由于已经合并到STATE_SYN_RCVD和STATE_ESTABLISHED处理，
	    	//   不应该走到这里!!!!
			//------------------------------------------------------------------
		case STATE_CLOSE_WAIT:
   			NET_LOG("tcp_rcve: conn[%d] CLOSE_WAIT\r\n",nr,0,0,0,0,0); 

	    	// With this code, should not get here
	    	NET_LOG("tcp_rcve: Oops! Rcvd unexpected message\r\n",0,0,0,0,0,0);
				
	    	break;
	      
			//------------------------------------------------------------------
	    	//        5.5   STATE_LAST_ACK(被动关闭端)
			//------------------------------------------------------------------
	    case STATE_LAST_ACK:
   			NET_LOG("tcp_rcve: conn[%d] LAST_ACK\r\n",nr,0,0,0,0,0); 
			
	    	conxn[nr].his_ack = ntohl(tcp->ack_number);
		  	// If he ACK's my FIN then close
	    	// 如果收到我的FIN，STATE_CLOSED
	    	if (ntohl(tcp->ack_number) == conxn[nr].my_sequence)
	     	{
	        	conxn[nr].state = STATE_CLOSED;
	        	conxn[nr].valid = FALSE;  // Free up struct area
	     	}
	     	break;

			//------------------------------------------------------------------
	    	//        5.6   STATE_FIN_WAIT_1(主动关闭)
	    	//     由 tcp_inactivity()发起,已经发送了 FIN
			//------------------------------------------------------------------
	    case STATE_FIN_WAIT_1:
   			NET_LOG("tcp_rcve: conn[%d] FIN_WAIT_1\r\n",nr,0,0,0,0,0); 
			
	    	// He may still be sending me data - should process it
			conxn[nr].his_sequence += data_len;
	    	conxn[nr].his_ack = ntohl(tcp->ack_number);
			//......................................................
	    	//    5.6.1  发FIN后，收到对端FIN，说明同时发送FIN!
	   		//......................................................
	    	if (tcp->flags & FLG_FIN)
	      	{
	      		// His FIN counts as a byte of data
	      		conxn[nr].his_sequence++;
	     		NET_LOG("tcp_rcve: conn[%d] R_FIN+,S_ack\r\n",nr,0,0,0,0,0);
	      		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_ACK, FALSE, 0);

	      		// ---5.6.1.1  如果对端也ACK我的FIN:
	      		//转移到STATE_TIME_WAIT，再转移到STATE_CLOSED，释放
	      		if (ntohl(tcp->ack_number) == conxn[nr].my_sequence)
				{
	      			conxn[nr].state = STATE_TIME_WAIT;
	      			NET_LOG("tcp_rcve: conn[%d] acp,F_W_1toTIME_WAITtoCLOSED\r\n", nr,0,0,0,0,0);
	      			conxn[nr].state = STATE_CLOSED;
	      			conxn[nr].valid = FALSE;  // Free up connection
	      		}
	      		//    5.6.1.2  如果对端没有ACK我的FIN
	      		//      转移到STATE_CLOSING
				else
				{
					// He has not ACK'd my FIN.  This happens when there is a simultaneous
					// close - I got his FIN but he has not yet ACK'd my FIN
					conxn[nr].state = STATE_CLOSING;
					NET_LOG("tcp_rcve: conn[%d] nAck,F_W_1toCLOSING\r\n", nr ,0,0,0,0,0);
				}
			} // ---5.6.1 tcp->flags & FLG_FIN          
				
			//................................................................
	      	//----5.6.2  发FIN后， 没有收到对端FIN,进入STATE_FIN_WAIT_2
	      	//................................................................
			else 
		  	if (ntohl(tcp->ack_number) == conxn[nr].my_sequence)
	      	{
	        	// He has ACK'd my FIN but has not sent a FIN yet himself
	        	conxn[nr].state = STATE_FIN_WAIT_2;
	        	NET_LOG("tcp_rcve: conn[%d] R_ack,F_W_1toF_W_2",nr,0,0,0,0,0);
	      	}
	      	break;  //----end of 5.6 case STATE_FIN_WAIT_1:

				//----------------------------------------------------
	      //   5.7  STATE_FIN_WAIT_2，如果收FIN,发ACK，
	      //   进入STATE_TIME_WAIT，进入STATE_CLOSED，释放
	      //-----------------------------------------------------
	      case STATE_FIN_WAIT_2:
   				NET_LOG("tcp_rcve: conn[%d] FIN_WAIT_2\r\n",nr,0,0,0,0,0); 
		  	
	      		// He may still be sending me data - should process it
				conxn[nr].his_sequence += data_len;
	      		conxn[nr].his_ack = ntohl(tcp->ack_number);
	      
	      		if (tcp->flags & FLG_FIN)
	      		{
	        		conxn[nr].his_sequence++; // For his FIN flag
	        		NET_LOG("tcp_rcve: conn[%d] R_fin,S_ack,F_W_2toT_WtoCLOSED\r\n", nr,0,0,0,0,0);
	        		tcp_send(NULL, nr, &remote_socket, &local_socket, FLG_ACK, FALSE, 0);

	       			conxn[nr].state = STATE_TIME_WAIT;
	        		conxn[nr].state = STATE_CLOSED;
	        		conxn[nr].valid = FALSE;  // Free up struct area
	      		}
	      		else
	      		{
	        		NET_LOG("tcp_rcve: conn[%d] :n_fin!,err\r\n", nr ,0,0,0,0,0);        
	      		}
	      		break;

	      //--------------------------------------------------------      
	      //   5.8       STATE_TIME_WAIT
	      //--------------------------------------------------------
		case STATE_TIME_WAIT:
			NET_LOG("tcp_rcve: conn[%d] TIME_WAIT\r\n",nr,0,0,0,0,0); 
			
	     	//    由于已经合并到STATE_FIN_WAIT_1和STATE_FIN_WAIT_2,STATE_CLOSING处理，
	     	//   不应该走到这里!!!!
	      	// With this code, should not get here
	      	NET_LOG("tcp_rcve: conxn[nr] T:Oops! In TIME_WAIT state\r\n", nr,0,0,0,0,0);
			break;

				//----------------------------------------------------------------
	      // 5.9  STATE_CLOSING :出线同时关闭现象
	      //    不再接收数据，
	      //   如果收到对方响应我的fin的ack，
	      //   进入STATE_TIME_WAIT，进入STATE_CLOSED，释放
	      //  否则等待......
	      //----------------------------------------------------------------
	      case STATE_CLOSING:
			NET_LOG("tcp_rcve: conn[%d] CLOSING\r\n",nr,0,0,0,0,0); 
		  	
	      // Simultaneous close has happened. I have received his FIN
	      // but he has not yet ACK'd my FIN.  Waiting for ACK.
				// Will not receive data in this state
				conxn[nr].his_ack = ntohl(tcp->ack_number);
	      		
				if (ntohl(tcp->ack_number) == conxn[nr].my_sequence)
	      		{
			   		conxn[nr].state = STATE_TIME_WAIT;

	        		NET_LOG("tcp_rcve: conxn[nr] R_ack,CLOSINGtoT_WtoCLOSED\r\n",nr,0,0,0,0,0);
	        
	        		// Do not send any response to his ACK
	        		conxn[nr].state = STATE_CLOSED;
	        		conxn[nr].valid = FALSE;  // Free up struct area
	      		}
	     	 break;

	      //-----------------------------------------
	      //    5.10  
	      //-----------------------------------------
	      default:
		      break;
	}  //end of 5  信息处理:根据状态机  switch (conxn[nr].state)
}  //------end of tcp_recv

#if 0
//------------------------------------------------------------------------
//    1  每 0.5 运行一次，
//    2  如果另外一端对于本端的send不ACK'd，重发,为了节省
//               内存，我重新产生报文
//	3  当这个程序运行时，一个连接在opening / closing 发送reset
//	4  当这个程序运行时，一个连接在ESTABLISHED，
//             we have just sent a web page so re-send the page
//--------------------------------------------------------------
// This runs every 0.5 seconds.  If the other end has not ACK'd
// everyting we have sent, it re-sends it.  To save RAM space, we 
// regenerate a segment rather than keeping a bunch of segments 
// hanging around eating up RAM.  A connection should not be in an
// opening or closing state when this timer expires, so we simply
// send a reset.
//
//	If a connection is in the ESTABLISHED state when the timer expires
// then we have just sent a web page so re-send the page
//------------------------------------------------------------------------
void tcp_retransmit(void)//等待应答超时，需要重传
{
	static UCHAR idata ack_low = 0,retries = 0;
  	UCHAR idata nr; 									//当前的连接号：1-5

  	// Scan through all active connections 
	for (nr = 0; nr < MAX_OF_TCP_CONNS; nr++)
  	{
  		if ((conxn[nr].ipaddr != 0) && (conxn[nr].timer))//如果有连接并且没有超时
    	{
		  //连接计时器递减 Decrement the timer and see if it hit 0
			conxn[nr].timer--;
      		if (conxn[nr].timer == 0)
      		{
        		//-----1  如果不在 STATE_ESTABLISHED状态，send  RST//
        		//Socket just timed out. If we are not in ESTABLISHED state
        		// something is amiss so send reset and close connection
        		if (conxn[nr].state != STATE_ESTABLISHED)
        		{
          			// Send reset and close connection
          			#ifdef TCPDEBUG 
          			serial_send("\r\nT:timeOut,S_reset");
          			#endif
          			tcp_send(NULL, FLG_RST, 20, nr);
          			conxn[nr].ipaddr = 0;
          			return;//为什么不是break?????
        		}
        		//-----2  如果在 STATE_ESTABLISHED状态，
        		else
        		{
          			//--- 2.1 首先对方ack不应该> my_sequence，否则不合理
	        		if (conxn[nr].his_ack > conxn[nr].my_sequence)
          			{
            			// Send reset and close connection
           	 			#ifdef TCPDEBUG 
            			serial_send("T:E_ack,S_reset\r\n");
            			#endif
            			tcp_send(NULL, FLG_RST, 20, nr);
            			conxn[nr].ipaddr = 0;
            			return;
            		}
               
          //----2.2  对方ack号<my_sequence，说明有丢失////
          // 我们总是在发送结束后立即增加sequence 号，//////
          // 因此当前的sequence号应该等于对端发来的ack号///
          // 对方ack号<my_sequence，说明有丢失//////////
          
          //可能1:没收到,说明有丢失:可以重发//////
          //可能2：因为发送大的文件时，没有处理ack，而是连续发送，tcp_retransmit()计时到也会出现此情况///
          //而此时，正在运行http_send()被中断，此时http_send()可能发送了n个连续报文的m个，////////////
          //此时运行进入tcp_retransmit()->http_server（）->http_send()没有任何用途，会重入、////////
          //    所以：此时不应该重发，连续计时i次，i/2妙后复位
          			if (conxn[nr].his_ack < conxn[nr].my_sequence)
					{
            			ack_low++;
            			if(ack_low >= 10)
            			{
           					ack_low=0;
           					retries++;
							if (retries <= 2)
							{
	              // ----2.2.1    重发,最多2次
        	      // The only thing we send is a web page, and it looks
            	  // like other end did not get it, so resend  but do not increase my sequence number
	              				#ifdef TCPDEBUG 
	  	          				serial_send("T:Timeout, resending data\r\n");
 	        	    			#endif

								#ifdef HTTP_SUPPORT
              					http_server(conxn[nr].query, 0, nr, 1);
								#endif

				        		conxn[nr].inactivity = INACTIVITY_TIME;
        	      			}
							else
							//----2.2.2  重发也收不到，send  RST，释放连接
							{
								#ifdef TCPDEBUG 
								serial_send("T:ReTran2,S_reset\r\n");
								#endif
								// Send reset and close connection
        	      				tcp_send(NULL, FLG_RST, 20, nr);
              					conxn[nr].ipaddr = 0;
              					retries=0;
              				}
              			}
            		} //----end of 2.2 if (conxn[nr].his_ack < conxn[nr].my_sequence)

          		}   //---end of 2
        }	 //---end of if (conxn[nr].timer == 0)
      }  //----end of if ((conxn[nr].ipaddr != 0) && (conxn[nr].timer))
   } 	//---end of for (nr = 0; nr < MAX_OF_TCP_CONNS; nr++)
}

#endif

//------------------------------------------------------------------------
// This runs every 0.5 seconds.  If the connection has had no activity
// it initiates closing the connection.
//
//------------------------------------------------------------------------
void tcp_inactivity(void)
{
	UCHAR nr;

	// Look for active connections in the established state
	for (nr = 0; nr < MAX_OF_TCP_CONNS; nr++)
  	{
    	if (conxn[nr].valid)
    	{
      		// Decrement the timer and see if it hit 0
      		if ((conxn[nr].inactivity--) == 0)
      		{
      			NET_LOG("tcp_inactivity: nr=%d\r\n",nr,0,0,0,0,0);

        		// Inactivity timer has just timed out.
        		// Initiate close of connection
        		tcp_send(NULL, nr, NULL, NULL, (FLG_ACK | FLG_FIN), FALSE, 0);

				//关闭此连接
				memset(&conxn[nr], 0, sizeof(TCP_CONNECTION));
      		}//end if (conxn[nr].inactivity == 0)
    	}//end if
	}//end for
}
#endif

