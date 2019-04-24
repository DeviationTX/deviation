#ifndef _IFACE_NRF24L01_H_
#define _IFACE_NRF24L01_H_

// Register map
enum {
    NRF24L01_00_CONFIG      = 0x00,
    NRF24L01_01_EN_AA       = 0x01,
    NRF24L01_02_EN_RXADDR   = 0x02,
    NRF24L01_03_SETUP_AW    = 0x03,
    NRF24L01_04_SETUP_RETR  = 0x04,
    NRF24L01_05_RF_CH       = 0x05,
    NRF24L01_06_RF_SETUP    = 0x06,
    NRF24L01_07_STATUS      = 0x07,
    NRF24L01_08_OBSERVE_TX  = 0x08,
    NRF24L01_09_CD          = 0x09,
    NRF24L01_0A_RX_ADDR_P0  = 0x0A,
    NRF24L01_0B_RX_ADDR_P1  = 0x0B,
    NRF24L01_0C_RX_ADDR_P2  = 0x0C,
    NRF24L01_0D_RX_ADDR_P3  = 0x0D,
    NRF24L01_0E_RX_ADDR_P4  = 0x0E,
    NRF24L01_0F_RX_ADDR_P5  = 0x0F,
    NRF24L01_10_TX_ADDR     = 0x10,
    NRF24L01_11_RX_PW_P0    = 0x11,
    NRF24L01_12_RX_PW_P1    = 0x12,
    NRF24L01_13_RX_PW_P2    = 0x13,
    NRF24L01_14_RX_PW_P3    = 0x14,
    NRF24L01_15_RX_PW_P4    = 0x15,
    NRF24L01_16_RX_PW_P5    = 0x16,
    NRF24L01_17_FIFO_STATUS = 0x17,
    NRF24L01_1C_DYNPD       = 0x1C,
    NRF24L01_1D_FEATURE     = 0x1D,
    //Instructions
    NRF24L01_61_RX_PAYLOAD  = 0x61,
    NRF24L01_A0_TX_PAYLOAD  = 0xA0,
    NRF24L01_E1_FLUSH_TX    = 0xE1,
    NRF24L01_E2_FLUSH_RX    = 0xE2,
    NRF24L01_E3_REUSE_TX_PL = 0xE3,
    NRF24L01_50_ACTIVATE    = 0x50,
    NRF24L01_60_R_RX_PL_WID = 0x60,
    NRF24L01_B0_TX_PYLD_NOACK = 0xB0,
    NRF24L01_FF_NOP         = 0xFF,
    NRF24L01_A8_W_ACK_PAYLOAD0 = 0xA8,
    NRF24L01_A8_W_ACK_PAYLOAD1 = 0xA9,
    NRF24L01_A8_W_ACK_PAYLOAD2 = 0xAA,
    NRF24L01_A8_W_ACK_PAYLOAD3 = 0xAB,
    NRF24L01_A8_W_ACK_PAYLOAD4 = 0xAC,
    NRF24L01_A8_W_ACK_PAYLOAD5 = 0xAD,
};

// Bit mnemonics
enum {
    NRF24L01_00_PRIM_RX     = 0,
    NRF24L01_00_PWR_UP      = 1,
    NRF24L01_00_CRCO        = 2,
    NRF24L01_00_EN_CRC      = 3,
    NRF24L01_00_MASK_MAX_RT = 4,
    NRF24L01_00_MASK_TX_DS  = 5,
    NRF24L01_00_MASK_RX_DR  = 6,

    NRF24L01_01_ENAA_P0     = 0,
    NRF24L01_01_ENAA_P1     = 1,

    NRF24L01_02_ERX_P0      = 0,
    NRF24L01_02_ERX_P1      = 1,

    NRF24L01_07_MAX_RT      = 4,
    NRF24L01_07_TX_DS       = 5,
    NRF24L01_07_RX_DR       = 6,

    NRF24L01_17_RX_EMPTY    = 0,
    NRF24L01_17_RX_FULL     = 1,
    NRF24L01_17_TX_EMPTY    = 4,
    NRF24L01_17_TX_FULL     = 5,

    NRF24L01_1C_DYNPD_P0    = 0,
    NRF24L01_1C_DYNPD_P1    = 1,

    NRF2401_1D_EN_DYN_ACK   = 0,
    NRF2401_1D_EN_ACK_PAY   = 1,
    NRF2401_1D_EN_DPL       = 2,
};

// Pre-shifted and combined bits
enum {
    NRF24L01_04_ARD_250us   = 0x00,
    NRF24L01_04_ARD_500us   = 0x10,
    NRF24L01_04_ARD_750us   = 0x20,
    NRF24L01_04_ARD_1000us  = 0x30,
    NRF24L01_04_ARD_1250us  = 0x40,
    NRF24L01_04_ARD_1500us  = 0x50,
    NRF24L01_04_ARD_1750us  = 0x60,
    NRF24L01_04_ARD_2000us  = 0x70,
    NRF24L01_04_ARD_2250us  = 0x80,
    NRF24L01_04_ARD_2500us  = 0x90,

    NRF24L01_04_ARC_0       = 0x00,
    NRF24L01_04_ARC_1       = 0x01,
    NRF24L01_04_ARC_2       = 0x02,
    NRF24L01_04_ARC_3       = 0x03,

    NRF24L01_08_ARC_CNT_MASK  = 0x0f,
    NRF24L01_08_PLOS_CNT_MASK = 0xf0,
};

// Bitrates
enum {
    NRF24L01_BR_1M = 0,
    NRF24L01_BR_2M,
    NRF24L01_BR_250K,
    NRF24L01_BR_RSVD
};
    

void NRF24L01_Initialize();
int NRF24L01_Reset();
u8 NRF24L01_WriteReg(u8 reg, u8 data);
u8 NRF24L01_WriteRegisterMulti(u8 reg, const u8 data[], u8 length);
u8 NRF24L01_WritePayload(u8 *data, u8 len);
u8 NRF24L01_ReadReg(u8 reg);
u8 NRF24L01_ReadRegisterMulti(u8 reg, u8 data[], u8 length);
u8 NRF24L01_ReadPayload(u8 *data, u8 len);

u8 NRF24L01_GetStatus(void);
u8 NRF24L01_GetDynamicPayloadSize(void);
u8 NRF24L01_FlushTx();
u8 NRF24L01_FlushRx();
u8 NRF24L01_Activate(u8 code);

// Bitrate 0 - 1Mbps, 1 - 2Mbps, 3 - 250K (for nRF24L01+)
u8 NRF24L01_SetBitrate(u8 bitrate);

u8 NRF24L01_SetPower(u8 power);
void NRF24L01_SetTxRxMode(enum TXRX_State);
int NRF24L01_Reset();

// To enable radio transmit after WritePayload you need to turn the radio
//void NRF24L01_PulseCE();

// XN297 emulation layer
enum {
    XN297_UNSCRAMBLED = 0,
    XN297_SCRAMBLED
};

extern const u8 xn297_scramble[];
extern const u16 xn297_crc_xorout_scrambled[];
extern const u16 xn297_crc_xorout[];
uint8_t bit_reverse(uint8_t b_in);

void XN297_SetTXAddr(const u8* addr, int len);
void XN297_SetRXAddr(const u8* addr, int len);
void XN297_Configure(u8 flags);
void XN297_SetScrambledMode(const u8 mode);
u8 XN297_WritePayload(u8* msg, int len);
u8 XN297_WriteEnhancedPayload(u8* msg, int len, int noack, u16 crc_xorout);
u8 XN297_ReadPayload(u8* msg, int len);
u8 XN297_ReadEnhancedPayload(u8* msg, int len);
u16 crc16_update(u16 crc, u8 a, u8 bits);

// HS6200 emulation layer
void HS6200_SetTXAddr(const u8* addr, u8 len);
void HS6200_Configure(u8 flags);
u8 HS6200_WritePayload(u8* msg, u8 len);

#endif
