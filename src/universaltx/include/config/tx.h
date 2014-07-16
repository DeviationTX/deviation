#ifndef _TX_H_
#define _TX_H_

struct mcu_pin {
    u32 port;
    u16 pin;
};


struct Transmitter {
    struct mcu_pin module_enable[TX_MODULE_LAST];
    u8 txid;
    //u8 module_poweramp;
};
extern struct Transmitter Transmitter;

#endif
