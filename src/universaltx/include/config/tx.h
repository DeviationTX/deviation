#ifndef _TX_H_
#define _TX_H_

enum {
    CYRF6936_DEVO       = 0x00,
    CYRF6936_AWA24S     = 0x01,
    CYRF6936_BUYCHINA   = 0x02,
    CC2500_REVERSE_GD02 = 0x01,
};

/* note that MODULE_ENABLE is defined in protospi.h */
struct Transmitter {
    u8 module_config[TX_MODULE_LAST];
    u32 txid;
    //u8 module_poweramp;
};

extern struct Transmitter Transmitter;

#endif
