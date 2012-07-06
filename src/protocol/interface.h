#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#ifdef PROTO_HAS_A7105
#include "iface_a7105.h"
void FLYSKY_Initialize();
#endif

#ifdef PROTO_HAS_CYRF6936
#include "iface_cyrf6936.h"
void DEVO_Initialize();
void DSM2_Initialize();
void J6PRO_Initialize();
#endif

#endif //_INTERFACE_H_
