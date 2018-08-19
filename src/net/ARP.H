//-----------------------------------------------------------------------------
// ARP.H
//
//-----------------------------------------------------------------------------
#ifndef _ARP_H__
#define _ARP_H__

#include "net.h"

// Allow up to 10 entries in ARP cache
#define CACHESIZE		10
extern ARP_CACHE arp_cache[CACHESIZE];

void arp_init(void);
UCHAR * arp_resolve(UINT32);
void arp_rcve(UCHAR *);

#endif
