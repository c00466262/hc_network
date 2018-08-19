//-----------------------------------------------------------------------------
// UDP.H
//
//-----------------------------------------------------------------------------

void udp_send(UCHAR  * outbuf, sockaddr_in *remote_addr, sockaddr_in *local_addr, UINT len);
void udp_rcve(UCHAR  * inbuf, UINT32 remote_ip, UINT32 local_ip, UINT len);
void udp_init(void);

