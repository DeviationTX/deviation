#ifndef _TX_H_
#define _TX_H_

/* note that MODULE_ENABLE is defined in protospi.h */
struct Transmitter {
    u32 txid;
    //u8 module_poweramp;
};

extern struct Transmitter Transmitter;

#endif
