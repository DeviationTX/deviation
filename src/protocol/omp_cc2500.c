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

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"  // for Transmitter


#ifdef PROTO_HAS_CC2500

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define OMP_BIND_COUNT      20
#define OMP_PACKET_PERIOD   100
#define dbgprintf printf
#else
#define OMP_BIND_COUNT      100
#define OMP_PACKET_PERIOD   5000  // Timeout for callback in uSec
// printf inside an interrupt handler is really dangerous
// this shouldn't be enabled even in debug builds without explicitly
// turning it on
#define dbgprintf if (0) printf
#endif

#define OMP_PACKET_SIZE           16
#define OMP_RF_BIND_CHANNEL       35
#define OMP_NUM_RF_CHANNELS       8
#define OMP_ADDR_LEN              5


static const char * const omp_opts[] = {
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  _tr_noop("Telemetry"), _tr_noop("On"), _tr_noop("Off"), NULL,
  NULL
};

enum {
    PROTOOPTS_FREQFINE = 0,
    PROTOOPTS_TELEMETRY,
    LAST_PROTO_OPT,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_ON  0
#define TELEM_OFF 1


static u8 tx_power;
static u8 packet[16];
static u8 hopping_frequency_no = 0;
static u8 rx_tx_addr[5];
static u8 hopping_frequency[OMP_NUM_RF_CHANNELS];
static u16 bind_counter;
static u8 phase;
static u8 calibration[OMP_NUM_RF_CHANNELS];
static u8 calibration_fscal2;
static u8 calibration_fscal3;
static s8 fine;
static u8 telm_req = 0;
static u16 tx_wait = 0;
static u8 last_good_v_lipo = 0;


enum{
    OMP_BIND,
    OMP_DATA,
    OMP_PACKET_SEND,
};

enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
    CHANNEL7,
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u16 scale_channel(u8 ch, u16 destMin, u16 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = (s32) destMax - (s32) destMin;

    if (chanval < CHAN_MIN_VALUE)
        chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE)
        chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)


// calibrate used RF channels for faster hopping
static void calibrate_rf_chans()
{
    for (int c = 0; c < OMP_NUM_RF_CHANNELS; c++) {
        CLOCK_ResetWatchdog();
        CC2500_Strobe(CC2500_SIDLE);
        XN297L_SetChannel(hopping_frequency[c]);
        CC2500_Strobe(CC2500_SCAL);
        usleep(900);
        calibration[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
    }
    calibration_fscal3 = CC2500_ReadReg(CC2500_23_FSCAL3);  // only needs to be done once
    calibration_fscal2 = CC2500_ReadReg(CC2500_24_FSCAL2);  // only needs to be done once
    CC2500_Strobe(CC2500_SIDLE);
}

static void calc_fh_channels(u8 num_ch)
{
    u8 idx = 0;
    u32 rnd = Model.fixed_id;
    u8 max = (num_ch/3)+2;

    while (idx < num_ch)
    {
        u8 i;
        u8 count_2_26 = 0, count_27_50 = 0, count_51_74 = 0;

        rnd = rnd * 0x0019660D + 0x3C6EF35F;  // Randomization
        // Use least-significant byte. 73 is prime, so channels 76..77 are unused
        u8 next_ch = ((rnd >> 8) % 73) + 2;
        // Keep a distance of 5 between consecutive channels
        if (idx !=0)
        {
            if (hopping_frequency[idx-1] > next_ch)
            {
                if (hopping_frequency[idx-1] - next_ch < 5)
                    continue;
            }
            else
                if (next_ch-hopping_frequency[idx-1] < 5)
                continue;
        }
        // Check that it's not duplicated and spread uniformly
        for (i = 0; i < idx; i++) {
            if (hopping_frequency[i] == next_ch)
                break;
            if (hopping_frequency[i] <= 26)
                count_2_26++;
            else if (hopping_frequency[i] <= 50)
                count_27_50++;
            else
                count_51_74++;
        }
        if (i != idx)
            continue;
        if ( (next_ch <= 26 && count_2_26 < max) || (next_ch >= 27 && next_ch <= 50 && count_27_50 < max) || (next_ch >= 51 && count_51_74 < max) )
            hopping_frequency[idx++] = next_ch;  // find hopping frequency
    }
}

static void omp_update_telemetry()
{
// raw receive data    first byte should be 0x55 (last xn297l preamble word) and then 5 byte address + 2byte PCF + 16byte payload +2 byte crc, total=1+5+2+16+2=26
// packet_in = 01 00 98 2C 03 19 19 F0 49 02 00 00 00 00 00 00
// all bytes are fixed and unknown except 2 and 3 which represent the battery voltage: packet_in[3]*256+packet_in[2]=lipo voltage*100 in V
    const u8 *update = NULL;
    static const u8 omp_telem[] = { TELEM_DEVO_VOLT1, TELEM_DEVO_RPM1, TELEM_DEVO_RPM2, 0 };  // use TELEM_DEVO_RPM1 for LRSSI, TELEM_DEVO_RPM2 for LQI

    u16 V = 0;
    u8 telem_len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;

    if (telem_len == 28)  // 26 plus 2byte append rx_status(RSSI+LQI)
        {
            u8 crc_ok = XN297L_ReadEnhancedPayload(packet, telem_len);

            if (crc_ok)
                {  // packet with good CRC and length
                    V = ((packet[3] << 8) + packet[2]) / 100;
                    last_good_v_lipo = V;
                    Telemetry.value[TELEM_DEVO_VOLT1] = V;
                    Telemetry.value[TELEM_DEVO_RPM1] = ((s8)crc_ok) / 2 - 72;  // rssi to dbm
                    update = omp_telem;
                }
            else
                {  // As soon as the motor spins the telem packets are becoming really bad and the CRC throws most of them in error as it should but...
                    if (packet[0] == 0x01 && packet[1] == 0x00)
                        {  // the start of the packet looks ok...
                            V = ((packet[3] << 8) + packet[2]) / 100;
                            if (V < 130 && V > 60)
                                {  // voltage is less than 13V and more than 6V (3V/element)
                                    u16 v1 = V - last_good_v_lipo;
                                    if (v1&0x8000) v1 = -v1;
                                    if (v1 < 10)  // the batt voltage is within 1V from a good reading...
                                        {
                                            Telemetry.value[TELEM_DEVO_VOLT1] = V;
                                            update = omp_telem;
                                        }
                                }
                        }
                }
            Telemetry.value[TELEM_DEVO_RPM2] = CC2500_ReadReg(CC2500_33_LQI | CC2500_READ_BURST) & 0x7F;
            update = omp_telem;
        }
    CC2500_Strobe(CC2500_SFRX);
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_SetTxRxMode(TXRX_OFF);
    if (update)
    {
        while (*update) {
            TELEMETRY_SetUpdated(*update++);
        }
    }
}

static void OMP_send_packet(u8 bind)
{
    CC2500_SetTxRxMode(TX_EN);
    CLOCK_ResetWatchdog();
    CLOCK_RunMixer();
    if (!bind)
    {
    memset(packet, 0x00, OMP_PACKET_SIZE);
    packet[0] = hopping_frequency_no;
    telm_req++;
    telm_req %= OMP_NUM_RF_CHANNELS-1;  // Change telem RX channels every time

    if (telm_req == 0)
        packet[0] |= 0x40;
    CC2500_WriteReg(CC2500_23_FSCAL3, calibration_fscal3);
    CC2500_WriteReg(CC2500_24_FSCAL2, calibration_fscal2);
    CC2500_WriteReg(CC2500_25_FSCAL1, calibration[hopping_frequency_no]);
    XN297L_SetChannel(hopping_frequency[hopping_frequency_no]);
    hopping_frequency_no++;
    hopping_frequency_no &= OMP_NUM_RF_CHANNELS-1;

        packet[1] = 0x08                                // unknown
                    | GET_FLAG(CHANNEL5, 0x20);         // HOLD
        packet[2] = 0x40;                               // unknown
        u16 ch = scale_channel(CHANNEL6, 0, 0x7FF);     // throttle
        if (ch > 0x600)
            packet[2] |= 0x20;                          // IDLE2
        else if (ch > 0x200)
            packet[1] |= 0x40;                          // IDLE1
        ch = scale_channel(CHANNEL7, 0, 0x7FF);         // throttle
        if (ch > 0x600)
            packet[2] |= 0x08;                          // 3D
        else if (ch > 0x200)
            packet[2] |= 0x04;                          // ATTITUDE
        ch = scale_channel(CHANNEL3, 0, 0x7FF);         // throttle
        packet[7] = ch;
        packet[8] = ch >> 8;
        ch = scale_channel(CHANNEL1, 0, 0x7FF);         // aileron
        packet[8] |= ch << 3;
        packet[9] = ch >> 5;
        ch = scale_channel(CHANNEL2, 0, 0x7FF);         // elevator
        packet[9] |= ch << 6;
        packet[10] = ch >> 2;
        packet[11] = ch >> 10;
        ch = scale_channel(CHANNEL4, 0, 0x7FF);         // rudder
        packet[11] |= ch << 1;
        packet[12] = ch >> 7;
        packet[15] = 0x04;
    }

    XN297L_WriteEnhancedPayload(packet, OMP_PACKET_SIZE, telm_req != 0);   // ack/8packet

    if (tx_power != Model.tx_power)  // Keep transmit power updated
    {
        tx_power = Model.tx_power;
        CC2500_SetPower(tx_power);
    }
}


static u16 OMP_callback()
{
    u16 timeout = OMP_PACKET_PERIOD;
    if (fine != (s8)Model.proto_opts[PROTOOPTS_FREQFINE])
    {
        fine = (s8)Model.proto_opts[PROTOOPTS_FREQFINE];
        CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
    }
    switch (phase) {
        case OMP_BIND:
            if (bind_counter == 0)
                {
                    PROTOCOL_SetBindState(0);
                    XN297L_SetTXAddr(rx_tx_addr, 5);
                    phase = OMP_DATA;
                }
            else
                {
                    OMP_send_packet(1);
                    bind_counter--;
                }
            break;

        case OMP_DATA:
            OMP_send_packet(0);
                    phase = OMP_PACKET_SEND;
                    timeout = 1250;
                    tx_wait = 0;
            break;

        case OMP_PACKET_SEND:
            if (CC2500_ReadReg(CC2500_35_MARCSTATE | CC2500_READ_BURST) == 0x13)
                {
                    timeout = 50;
                    tx_wait += 50;
                    if (tx_wait > 1000)
                        {
                            phase = OMP_DATA;
                            timeout = OMP_PACKET_PERIOD-2250;
                        }
                    break;
                }
                CC2500_Strobe(CC2500_SIDLE);
                CC2500_SetTxRxMode(TXRX_OFF);
                timeout = OMP_PACKET_PERIOD-1250-tx_wait;
                phase = OMP_DATA;

                if (Model.proto_opts[PROTOOPTS_TELEMETRY])
                    break;

                switch (telm_req)
                    {
                        case 0:
                                    CC2500_SetTxRxMode(RX_EN);
                                    CC2500_Strobe(CC2500_SFRX);
                                    CC2500_Strobe(CC2500_SRX);
                            break;
                        case 1:
                            omp_update_telemetry();
                            timeout -= 50;
                            break;
                        default:
                            break;
                    }
            break;
    }

    return timeout;
}

static void OMP_init()
{
    // setup cc2500 for xn297L@250kbps emulation, scrambled, crc enabled
    XN297L_Configure(XN297L_SCRAMBLED, XN297L_CRC, OMP_PACKET_SIZE+10);  // packet_size + 5byte address + 2 byte pcf + 2byte crc + 1byte preamble
    calibrate_rf_chans();
    CC2500_SetPower(tx_power);
}

static void OMP_initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i, j;

    if (Model.fixed_id) {
       for (i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    for (i=0, j=0; i < 4; i++, j+=8)
        rx_tx_addr[i] = (lfsr >> j) & 0xff;

    rand32_r(&lfsr, 0);
    rx_tx_addr[4] = lfsr & 0xff;
    // channels
     calc_fh_channels(OMP_NUM_RF_CHANNELS);
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    OMP_initialize_txid();

    tx_power = Model.tx_power;
    OMP_init();

    fine = (s8)Model.proto_opts[PROTOOPTS_FREQFINE];
    CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);

    if (bind)
        {
            bind_counter = OMP_BIND_COUNT;
            PROTOCOL_SetBindState(OMP_BIND_COUNT * OMP_PACKET_PERIOD / 1000);
            phase = OMP_BIND;
            XN297L_SetTXAddr((u8*)"FLPBD", OMP_ADDR_LEN);
            XN297L_SetChannel(OMP_RF_BIND_CHANNEL);
            CC2500_Strobe(CC2500_SCAL);
            usleep(900);
            CC2500_Strobe(CC2500_SIDLE);
            memset(packet, 0x00, OMP_PACKET_SIZE);
            memcpy(packet, "BND", 3);
            memcpy(&packet[3], rx_tx_addr, 5);
            memcpy(&packet[8], hopping_frequency, 8);
        }
    else
        {
            XN297L_SetTXAddr(rx_tx_addr, OMP_ADDR_LEN);
            XN297L_SetRXAddr(rx_tx_addr, OMP_ADDR_LEN);
            phase = OMP_DATA;
        }

    CLOCK_StartTimer(OMP_PACKET_PERIOD, OMP_callback);
}

uintptr_t OMP_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (CC2500_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 7;
        case PROTOCMD_DEFAULT_NUMCHAN: return 7;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)omp_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (Model.proto_opts[PROTOOPTS_TELEMETRY] != TELEM_OFF ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_DEVO;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
