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

/* This protocol is for the HM Hobby HM830 RC Paper Airplane */
/* Protocol spec:
   Channel data:
   AA BB CC DD EE FF GG
   AA : Throttle Min=0x00 max =0x64
   BB : 
        bit 0,1,2: Left/Right magnitude, bit 5 Polarity (set = right)
        bit 6: Accelerate
        bit 7: Right button (also the ABC Button)
   CC : bit 0 seems to be impacted by the Right button
   DD
   EE
   FF : Trim (bit 0-5: Magnitude, bit 6 polarity (set = right)
   GG : Checksum (CRC8 on bytes AA-FF), init = 0xa5, poly = 0x01
*/
#ifdef MODULAR
  //Allows the linker to properly relocate
  #define HM830_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"


// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7,
    CHANNEL8,
    CHANNEL9,
    CHANNEL10
};

#define PAYLOADSIZE 7
#define NFREQCHANNELS 15
#define TXID_SIZE 4

#ifdef EMULATOR
#define USE_FIXED_MFGID
#endif

enum {
    HM830_BIND1A = 0,
    HM830_BIND2A,
    HM830_BIND3A,
    HM830_BIND4A,
    HM830_BIND5A,
    HM830_BIND6A,
    HM830_BIND7A,
    HM830_DATA1,
    HM830_DATA2,
    HM830_DATA3,
    HM830_DATA4,
    HM830_DATA5,
    HM830_DATA6,
    HM830_DATA7,
    HM830_BIND1B = 0x80,
    HM830_BIND2B,
    HM830_BIND3B,
    HM830_BIND4B,
    HM830_BIND5B,
    HM830_BIND6B,
    HM830_BIND7B,
};

static const u8 init_vals[][2] = {
    {NRF24L01_17_FIFO_STATUS, 0x00},
    {NRF24L01_16_RX_PW_P5,    0x07},
    {NRF24L01_15_RX_PW_P4,    0x07},
    {NRF24L01_14_RX_PW_P3,    0x07},
    {NRF24L01_13_RX_PW_P2,    0x07},
    {NRF24L01_12_RX_PW_P1,    0x07},
    {NRF24L01_11_RX_PW_P0,    0x07},
    {NRF24L01_0F_RX_ADDR_P5,  0xC6},
    {NRF24L01_0E_RX_ADDR_P4,  0xC5},
    {NRF24L01_0D_RX_ADDR_P3,  0xC4},
    {NRF24L01_0C_RX_ADDR_P2,  0xC3},
    {NRF24L01_09_CD,          0x00},
    {NRF24L01_08_OBSERVE_TX,  0x00},
    {NRF24L01_07_STATUS,      0x07},
//    {NRF24L01_06_RF_SETUP,    0x07},
    {NRF24L01_05_RF_CH,       0x18},
    {NRF24L01_04_SETUP_RETR,  0x3F},
    {NRF24L01_03_SETUP_AW,    0x03},
    {NRF24L01_02_EN_RXADDR,   0x3F},
    {NRF24L01_01_EN_AA,       0x3F},
    {NRF24L01_00_CONFIG,      0x0E},
};

static u8 packet[7];
static u8 count;
static u8 phase;
static u8 rf_ch[]     = {0x08, 0x35, 0x12, 0x3f, 0x1c, 0x49, 0x26};
static u8 bind_addr[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xc2};
static u8 rx_addr[]   = {0xb6, 0x0c, 0x00, 0x40, 0xee, 0xc2};

static u8 crc8(u32 result, u8 *data, int len)
{
    int polynomial = 0x01;
    for(int i = 0; i < len; i++) {
        result = result ^ data[i];
        for(int j = 0; j < 8; j++) {
            if(result & 0x80) {
                result = (result << 1) ^ polynomial;
            } else {
                result = result << 1;
            }
        }
    }
    return result & 0xff;
}

static void HM830_init()
{
    NRF24L01_Initialize();
    for (u32 i = 0; i < sizeof(init_vals) / sizeof(init_vals[0]); i++)
        NRF24L01_WriteReg(init_vals[i][0], init_vals[i][1]);

    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_SetBitrate(0);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_addr,   5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, bind_addr+1, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    bind_addr,   5);
    NRF24L01_Activate(0x73);  //Enable FEATURE
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD,   0x3F);
    //NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x07

    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    // For detailed description of what's happening here see 
    //   http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    printf("=>H377 : Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("=>H377 : BK2421 detected\n");
        long nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\xF9\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x06, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x07, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x08, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x09, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0A, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0B, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        //NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
    } else {
        printf("=>H377 : nRF24L01 detected\n");
    }
    //NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x07
    NRF24L01_Activate(0x53); // switch bank back

    NRF24L01_FlushTx();
    //NRF24L01_ReadReg(NRF24L01_07_STATUS) ==> 0x0e
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x0e);
    //NRF24L01_ReadReg(NRF24L01_00_CONFIG); ==> 0x0e
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0e);
    NRF24L01_ReadReg(NRF24L01_01_EN_AA);      // No Auto Acknoledgement
}

static void build_bind_packet()
{
    for(int i = 0; i < 6; i++)
        packet[i] = rx_addr[i];
    packet[6] = crc8(0xa5, packet, 6);
}

static void build_data_packet()
{
    s8 throttle = (s32)Channels[0] * 50 / CHAN_MAX_VALUE + 50;
    if (throttle < 0)
        throttle = 0;
    s8 aileron = (s32)Channels[1] * 8 / CHAN_MAX_VALUE;
    u8 turbo = (s32)Channels[2] > 0 ? 1 : 0;
    u8 trim = ((s32)Channels[3] * 0x1f / CHAN_MAX_VALUE) & 0x3F;
    u8 rbutton = (s32)Channels[4] > 0 ? 1 : 0;
    packet[0] = throttle;
    packet[1] = aileron & 0x07;
    if (aileron < 0)
        packet[1] |= 0x20;
    if (turbo)
        packet[1] |= 0x40;
    if (rbutton)
        packet[1] |= 0x80;
    packet[5] = trim;
    packet[6] = crc8(0xa5, packet, 6);
}

static void send_packet()
{
    NRF24L01_ReadReg(NRF24L01_17_FIFO_STATUS);
    NRF24L01_WritePayload(packet, 7);
}

static u16 handle_binding()
{
    u8 status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    if (status & 0x20) {
        //Binding  complete
        phase = HM830_DATA1;
        count = 0;
        PROTOCOL_SetBindState(0);
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_addr,   5);
        NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_addr+1, 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_addr,   5);
        NRF24L01_FlushTx();
        build_data_packet();
        //NRF24L01_ReadReg(NRF24L01_07_STATUS); ==> 1E
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x1E);
        //NRF24L01_ReadReg(NRF24L01_00_CONFIG); ==> 0x0e
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0E);
        send_packet();
        return 14000;
    }
    switch (phase) {
    case HM830_BIND1A:
        //Look for a Rx that is already bound
        NRF24L01_SetPower(Model.tx_power);
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_addr,   5);
        NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_addr+1, 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    rx_addr,   5);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
        build_bind_packet();
        break;
    case HM830_BIND1B:
        //Look for a Rx that is not yet bound
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_addr,   5);
        NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, bind_addr+1, 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    bind_addr,   5);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
        break;
    case HM830_BIND2A:
    case HM830_BIND3A:
    case HM830_BIND4A:
    case HM830_BIND5A:
    case HM830_BIND6A:
    case HM830_BIND7A:
    case HM830_BIND2B:
    case HM830_BIND3B:
    case HM830_BIND4B:
    case HM830_BIND5B:
    case HM830_BIND6B:
    case HM830_BIND7B:
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[(phase&0x7F)-HM830_BIND1A]);
        break;
    }
    NRF24L01_FlushTx();
    u8 rb = NRF24L01_ReadReg(NRF24L01_07_STATUS); //==> 0x0E
    NRF24L01_WriteReg(NRF24L01_07_STATUS, rb & 0x1E);
    rb = NRF24L01_ReadReg(NRF24L01_00_CONFIG);    //==> 0x0E
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, rb & 0x0E);
    send_packet();
    phase++;
    if (phase == HM830_BIND7B+1) {
        phase = HM830_BIND1A;
    } else if (phase == HM830_BIND7A+1) {
        phase = HM830_BIND1B;
    }
    return 20000;
}

static u16 handle_data()
{
    u8 status = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    if (count == 0) {
        if(! (status & 0x20)) {
            //We didn't get a response on this channel, try the next one
            phase++;
            if (phase-HM830_DATA1 > 6)
                phase = HM830_DATA1;

            NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_ch[0]);
            NRF24L01_FlushTx();
            build_data_packet();
            //NRF24L01_ReadReg(NRF24L01_07_STATUS); ==> 1E
            NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x1E);
            //NRF24L01_ReadReg(NRF24L01_00_CONFIG); ==> 0x0e
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0E);
            send_packet();
            return 14000;
        }
    }
    build_data_packet();
    count++;
    if(count == 98) {
        count = 0;
        NRF24L01_SetPower(Model.tx_power);
    }
    send_packet();
    return 20000;
}



MODULE_CALLTYPE
static u16 HM830_callback()
{
    if ((phase & 0x7F) < HM830_DATA1)
        return handle_binding();
    else
        return handle_data();
}

static void initialize()
{
    CLOCK_StopTimer();
    PROTOCOL_SetBindState(0xFFFFFFFF); //Wait for binding
    count = 0;
    HM830_init();
    phase = HM830_BIND1A;

    CLOCK_StartTimer(50000, HM830_callback);
}

const void *HM830_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // Always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 5L; // T, A, E, R, G
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L;
        // TODO: return id correctly
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_SET_TXPOWER:
            NRF24L01_SetPower(Model.tx_power);
            break;
        default: break;
    }
    return 0;
}
#endif //PROTO_HAS_NRF24L01
