#ifndef _INTERFACE_H_
#define _INTERFACE_H_

enum ProtoCmds {
    PROTOCMD_INIT,
    PROTOCMD_DEINIT,
    PROTOCMD_BIND,
    PROTOCMD_CHECK_AUTOBIND,
    PROTOCMD_NUMCHAN,
    PROTOCMD_DEFAULT_NUMCHAN,
    PROTOCMD_CURRENT_ID,
    PROTOCMD_GETOPTIONS,
    PROTOCMD_SETOPTIONS,
    PROTOCMD_TELEMETRYSTATE,
    PROTOCMD_RESET,
};

enum TXRX_State {
    TXRX_OFF,
    TX_EN,
    RX_EN,
};

enum PinConfigState {
    CSN_PIN,
    ENABLED_PIN,
    DISABLED_PIN,
    RESET_PIN,
};

enum MultimodCmd {
    //TXRX_OFF,
    //TX_EN,
    //RX_EN,
    CHANGE_MODULE = 3,
    CLEAR_PIN_ENABLE,
    SET_PIN_ENABLE,
};

int MULTIMOD_SwitchCommand(int module, int command);
void MODULE_CSN(int module, int set);

#ifndef MODULAR
#define PROTODEF(proto, module, map, cmd, name) extern const void * cmd(enum ProtoCmds);
#include "protocol.h"
#undef PROTODEF
#endif

#ifdef PROTO_HAS_A7105
#include "iface_a7105.h"
#endif

#ifdef PROTO_HAS_CYRF6936
#include "iface_cyrf6936.h"
#endif

#ifdef PROTO_HAS_CC2500
#include "iface_cc2500.h"
#endif

#ifdef PROTO_HAS_NRF24L01
#include "iface_nrf24l01.h"
#endif

#endif //_INTERFACE_H_
