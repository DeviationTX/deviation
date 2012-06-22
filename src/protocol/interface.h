#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#ifdef PROTO_HAS_A7105
enum A7105_State {
    A7105_SLEEP     = 0x80,
    A7105_IDLE      = 0x90,
    A7105_STANDBY   = 0xA0,
    A7105_PLL       = 0xB0,
    A7105_RX        = 0xC0,
    A7105_TX        = 0xD0,
    A7105_RST_WRPTR = 0xE0,
    A7105_RST_RDPTR = 0xF0,
};

enum A7105_MASK {
    A7105_MASK_FBCF = 1 << 4,
    A7105_MASK_VBCF = 1 << 3,
};

void A7105_WriteReg(u8 addr, u8 value);
u8 A7105_ReadReg(u8 addr);
void A7105_Reset();
void A7105_WriteID(u32 id);
void A7105_Strobe(enum A7105_State);

#endif //A7105
#endif
