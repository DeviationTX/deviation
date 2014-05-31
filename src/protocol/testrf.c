/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef MODULAR
  //Allows the linker to properly relocate
  #define TESTRF_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#if 1 || defined PROTO_HAS_CYRF6936 && defined PROTO_HAS_A7105 && defined PROTO_HAS_CC2500 && defined PROTO_HAS_NRF24L01
#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

static const char * const testrf_opts[] = {
  _tr_noop("Radio"), _tr_noop("CYRF6936"), _tr_noop("A7105"), _tr_noop("CC2500"), _tr_noop("NRF24L01"), NULL,
  _tr_noop("RF Channel"), "1", "85", NULL,
  _tr_noop("Rate(ms)"), "1", "50", NULL,
  NULL
};
enum {
    TESTRF_RF = 0,
    TESTRF_RFCHAN = 1,
    TESTRF_RFRATE = 2,
};
//if sizeof(packet) changes, must change wMaxPacketSize to match in Joystick_ConfigDescriptor
static unsigned char packet[16];
#define BV(bit) (1 << bit)

MODULE_CALLTYPE
static u16 testrf_cb()
{
    packet[0]++;
    switch(Model.proto_opts[TESTRF_RF]) {
        case 0: //CYRF
            if (packet[0] == 1) {
                CYRF_SetPower(Model.tx_power);
            }
            CYRF_WriteDataPacketLen(packet, 16);
            break;
        case 1: //A7105
            if (packet[0] == 1) {
                A7105_SetPower(Model.tx_power);
            }
            A7105_WriteData(packet, 16, Model.proto_opts[TESTRF_RFCHAN]);
            break;
        case 2: //CC2500
            if (packet[0] == 1) {
                CC2500_SetPower(Model.tx_power);
            }
            CC2500_WriteData(packet, 16);
            break;
        case 3: //NRF
            if (packet[0] == 1) {
                NRF24L01_SetPower(Model.tx_power);
            }
            // clear packet status bits and TX FIFO
            NRF24L01_WriteReg(NRF24L01_07_STATUS, (BV(NRF24L01_07_TX_DS) | BV(NRF24L01_07_MAX_RT)));
            NRF24L01_FlushTx();
            NRF24L01_WritePayload(packet, 16);
            break;
        default: break;
    }
    return Model.proto_opts[TESTRF_RFRATE]*1000;
}

static void init_cyrf()
{
    static u8 initcmd[] = {
        CYRF_1D_MODE_OVERRIDE,  0x38,
        CYRF_03_TX_CFG,         0x08,
        CYRF_06_RX_CFG,         0x4A,
        CYRF_0B_PWR_CTRL,       0x00,
        CYRF_10_FRAMING_CFG,    0xA4,
        CYRF_11_DATA32_THOLD,   0x05,
        CYRF_12_DATA64_THOLD,   0x0E,
        CYRF_1B_TX_OFFSET_LSB,  0x55,
        CYRF_1C_TX_OFFSET_MSB,  0x05,
        CYRF_32_AUTO_CAL_TIME,  0x3C,
        CYRF_35_AUTOCAL_OFFSET, 0x14,
        CYRF_39_ANALOG_CTRL,    0x01,
        CYRF_1E_RX_OVERRIDE,    0x10,
        CYRF_1F_TX_OVERRIDE,    0x00,
        CYRF_01_TX_LENGTH,      0x10,
        CYRF_0F_XACT_CFG,       0x10,
        CYRF_27_CLK_OVERRIDE,   0x02,
        CYRF_28_CLK_EN,         0x02,
        CYRF_0F_XACT_CFG,       0x28,
    };
    for(unsigned i = 0; i < sizeof(initcmd); i+= 2) {
        CYRF_WriteRegister(initcmd[i], initcmd[i+1]);
    }
    CYRF_SetTxRxMode(TX_EN);
}

static void init_a7105()
{
    static const u8 A7105_regs[] = {
         -1,  0x42, 0x00, 0x14, 0x00,  -1 ,  -1 , 0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,
        0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,
        0x13, 0xc3, 0x00,  -1,  0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,
        0x01, 0x0f,  -1,
    };
    int i;
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;

    A7105_WriteID(0x5475c52a);
    for (i = 0; i < 0x33; i++)
        if((s8)A7105_regs[i] != -1)
            A7105_WriteReg(i, A7105_regs[i]);

    A7105_Strobe(A7105_STANDBY);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return;
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return;
    }

    //VCO Current Calibration
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet

    //VCO Bank Calibration
    A7105_WriteReg(0x26, 0x3b); //Recomended limits from A7105 Datasheet

    //VCO Bank Calibrate channel 0?
    //Set Channel
    A7105_WriteReg(0x0f, 0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return;
    vco_calibration0 = A7105_ReadReg(0x25);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return;
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(0x0f, 0xa0); //Should we choose a different channel?
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return;
    vco_calibration1 = A7105_ReadReg(0x25);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return;
    }

    //Reset VCO Band calibration
    A7105_WriteReg(0x25, 0x08);

    A7105_SetTxRxMode(TX_EN);

    A7105_Strobe(A7105_STANDBY);
}

static void init_cc2500()
{
    static u8 initcmd[] = {
        CC2500_16_MCSM2,    0x07,
        CC2500_17_MCSM1,    0x30,
        CC2500_1E_WOREVT1,  0x87,
        CC2500_1F_WOREVT0,  0x6b,
        CC2500_20_WORCTRL,  0xf8,
        CC2500_2A_PTEST,    0x7f,
        CC2500_2B_AGCTEST,  0x3f,
        CC2500_0B_FSCTRL1,  0x09,
        CC2500_0C_FSCTRL0,  0x00,
        CC2500_0D_FREQ2,    0x5d,
        CC2500_0E_FREQ1,    0x93,
        CC2500_0F_FREQ0,    0xb1,
        CC2500_10_MDMCFG4,  0x2d,
        CC2500_11_MDMCFG3,  0x20,
        CC2500_12_MDMCFG2,  0x73,
        CC2500_13_MDMCFG1,  0x22,
        CC2500_14_MDMCFG0,  0xf8,
        CC2500_0A_CHANNR,   0xcd,
        CC2500_15_DEVIATN,  0x50,
        CC2500_21_FREND1,   0xb6,
        CC2500_22_FREND0,   0x10,
        CC2500_18_MCSM0,    0x18,
        CC2500_19_FOCCFG,   0x1d,
        CC2500_1A_BSCFG,    0x1c,
        CC2500_1B_AGCCTRL2, 0xc7,
        CC2500_1C_AGCCTRL1, 0x00,
        CC2500_1D_AGCCTRL0, 0xb2,
        CC2500_23_FSCAL3,   0xea,
        CC2500_24_FSCAL2,   0x0a,
        CC2500_25_FSCAL1,   0x00,
        CC2500_26_FSCAL0,   0x11,
        CC2500_29_FSTEST,   0x59,
        CC2500_2C_TEST2,    0x88,
        CC2500_2D_TEST1,    0x31,
        CC2500_2E_TEST0,    0x0b,
        CC2500_07_PKTCTRL1, 0x05,
        CC2500_08_PKTCTRL0, 0x05,
        CC2500_09_ADDR,     0x43,
        CC2500_06_PKTLEN,   0xff,
        CC2500_04_SYNC1,    0x13,
        CC2500_05_SYNC0,    0x18,
    };
    for(unsigned i = 0; i < sizeof(initcmd); i+= 2) {
        CC2500_WriteReg(initcmd[i], initcmd[i+1]);
    }
    CC2500_SetTxRxMode(TX_EN);
    CC2500_SetPower(Model.tx_power);
    CC2500_Strobe(CC2500_SFTX);
    CC2500_Strobe(CC2500_SFRX);
    CC2500_Strobe(CC2500_SXOFF);
    CC2500_Strobe(CC2500_SIDLE);
}

static void init_nrf()
{
    static const u8 initcmd[] = {
    // CRC, radio on
        NRF24L01_00_CONFIG,      BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_PWR_UP), 
        NRF24L01_01_EN_AA,       0x3F,        // Auto Acknoledgement on all data pipes
        NRF24L01_02_EN_RXADDR,   0x3F,        // Enable all data pipes
        NRF24L01_03_SETUP_AW,    0x03,        // 5-byte RX/TX address
        NRF24L01_04_SETUP_RETR,  0x1A,        // 500uS retransmit t/o, 10 tries
        NRF24L01_07_STATUS,      0x70,        // Clear data ready, data sent, and retransmit
        NRF24L01_0C_RX_ADDR_P2,  0xC3,        // LSB byte of pipe 2 receive address
        NRF24L01_0D_RX_ADDR_P3,  0xC4,
        NRF24L01_0E_RX_ADDR_P4,  0xC5,
        NRF24L01_0F_RX_ADDR_P5,  0xC6,
        NRF24L01_11_RX_PW_P0,    sizeof(packet), // bytes of data payload for pipe 1
        NRF24L01_12_RX_PW_P1,    sizeof(packet),
        NRF24L01_13_RX_PW_P2,    sizeof(packet),
        NRF24L01_14_RX_PW_P3,    sizeof(packet),
        NRF24L01_15_RX_PW_P4,    sizeof(packet),
        NRF24L01_16_RX_PW_P5,    sizeof(packet),
        NRF24L01_17_FIFO_STATUS, 0x00,        // Just in case, no real bits to write here
        NRF24L01_1C_DYNPD,       0x3F,        // Enable dynamic payload length on all pipes
    };
    NRF24L01_Initialize();
    for(unsigned i = 0; i < sizeof(initcmd); i+= 2) {
        NRF24L01_WriteReg(initcmd[i], initcmd[i+1]);
    }

    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Set feature bits on


    //NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    //NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    //dbgprintf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        //dbgprintf("BK2421 detected\n");
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
    } else {
        //dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back

    NRF24L01_WriteReg(NRF24L01_05_RF_CH, Model.proto_opts[TESTRF_RFCHAN]);   // reset packet loss counter
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_SetTxRxMode(TX_EN);
}

static void initialize()
{
    CLOCK_StopTimer();
    CYRF_Reset();
    NRF24L01_Reset();
    A7105_Reset();
    CC2500_Reset();
    switch(Model.proto_opts[TESTRF_RF]) {
        case 0: //CYRF
            init_cyrf();
            break;
        case 1: //A7105
            init_a7105();
            break;
        case 2: //CC2500
            init_cc2500();
            break;
        case 3: //NRF
            init_nrf();
            break;
        default: break;
    }
    CLOCK_StartTimer(20000, testrf_cb);
}

const void * TESTRF_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
            CYRF_Reset();
            NRF24L01_Reset();
            A7105_Reset();
            CC2500_Reset();
            return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)12L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)6L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_GETOPTIONS:
            return testrf_opts;
        case PROTOCMD_SETOPTIONS:
            initialize();
            return 0;
        default: break;
    }
    return 0;
}
#endif //PROTO_HAS_*
