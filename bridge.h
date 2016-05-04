#ifndef BRIDGE_H
#define BRIDGE_H
#include "ether.h"
#include <time.h>
typedef struct _bridgeentry
{
 MacAddr host;
 int iface;
 time_t last_accesstime;
}BridgeEntry;
#endif
