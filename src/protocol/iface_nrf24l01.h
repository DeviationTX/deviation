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
    NRF24L01_1D_FEATURE     = 0x1D
};

// Bit mnemonics
enum {
    NRF24L01_00_MASK_RX_DR  = 6,
    NRF24L01_00_MASK_TX_DS  = 5,
    NRF24L01_00_MASK_MAX_RT = 4,
    NRF24L01_00_EN_CRC      = 3,
    NRF24L01_00_CRCO        = 2,
    NRF24L01_00_PWR_UP      = 1,
    NRF24L01_00_PRIM_RX     = 0,
    NRF24L01_07_RX_DR       = 6,
    NRF24L01_07_TX_DS       = 5,
    NRF24L01_07_MAX_RT      = 4,
};

// Bitrates
enum {
    NRF24L01_BR_1M = 0,
    NRF24L01_BR_2M,
    NRF24L01_BR_250K,
    NRF24L01_BR_RSVD
};
    

void NRF24L01_Initialize();
u8 NRF24L01_WriteReg(u8 reg, u8 data);
u8 NRF24L01_WriteRegisterMulti(u8 reg, const u8 data[], u8 length);
u8 NRF24L01_WritePayload(u8 *data, u8 len);
u8 NRF24L01_ReadReg(u8 reg);
u8 NRF24L01_ReadRegisterMulti(u8 reg, u8 data[], u8 length);
u8 NRF24L01_ReadPayload(u8 *data, u8 len);

u8 NRF24L01_FlushTx();
u8 NRF24L01_FlushRx();
u8 NRF24L01_Activate(u8 code);


// Bitrate 0 - 1Mbps, 1 - 2Mbps, 3 - 250K (for nRF24L01+)
u8 NRF24L01_SetBitrate(u8 bitrate);

u8 NRF24L01_SetPower(u8 power);

// To enable radio transmit after WritePayload you need to turn the radio
void NRF24L01_PulseCE();


#endif
