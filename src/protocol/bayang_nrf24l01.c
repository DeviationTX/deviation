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

// Telemetry data if enabled ( does not work with factory firmware )
// TELEM_DSM_FLOG_VOLT1 - real battery voltage
// TELEM_DSM_FLOG_VOLT2 - compensated battery voltage
// TELEM_DSM_FLOG_HOLDS - rx reception strenght ( packets per second )
// TELEM_DSM_FLOG_FRAMELOSS - tx telemetry reception strenght ( packets per second )
// TELEM_DSM_FLOG_FADESL - battery low flag ( LVC )

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"          // for Transmitter
#include "telemetry.h"

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 10
#define PACKET_PERIOD    450
#define dbgprintf printf
#else
#define BIND_COUNT       4000
// Timeout for callback in uSec
#define PACKET_PERIOD    1000UL

//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT       500
#define PACKET_SIZE        15
#define RF_NUM_CHANNELS    4
#define RF_BIND_CHANNEL    0
#define RF_BIND_CHANNEL_X16_AH 10
#define ADDRESS_LENGTH     5


// For code readability
enum {
    CHANNEL1 = 0,               // Aileron
    CHANNEL2,                   // Elevator
    CHANNEL3,                   // Throttle
    CHANNEL4,                   // Rudder
    CHANNEL5,                   // Leds
    CHANNEL6,                   // Flip
    CHANNEL7,                   // Still camera
    CHANNEL8,                   // Video camera
    CHANNEL9,                   // Headless
    CHANNEL10,                  // Return To Home
    CHANNEL11,                  // Take Off/Landing
    CHANNEL12,                  // Emg. stop
    CHANNEL13,                  // Analog Aux1
    CHANNEL14,                  // Analog Aux2
};
#define CHANNEL_INVERTED    CHANNEL5    // inverted flight on Floureon H101
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_PICTURE     CHANNEL7
#define CHANNEL_VIDEO       CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10
#define CHANNEL_TO          CHANNEL11
#define CHANNEL_EMGSTOP     CHANNEL12
#define CHANNEL_ANAAUX1     CHANNEL13
#define CHANNEL_ANAAUX2     CHANNEL14
enum {
    Bayang_INIT1 = 0,
    Bayang_BIND2,
    Bayang_DATA
};

static const char *const bay_opts[] = {
    _tr_noop("Telemetry"), _tr_noop("Off"), _tr_noop("On"), NULL,
    _tr_noop("Format"), _tr_noop("regular"), "X16-AH", "IRDRONE", NULL,
    _tr_noop("Analog Aux"), _tr_noop("Off"), _tr_noop("On"), NULL,
    NULL
};


enum {
    PROTOOPTS_TELEMETRY = 0,
    PROTOOPTS_FORMAT = 1,
    PROTOOPTS_ANALOGAUX = 2,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_REGULAR,
    FORMAT_X16_AH,
    FORMAT_IRDRONE,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_OFF 0
#define TELEM_ON 1

static u16 counter;
static u8 phase;
static u8 telemetry;
static u8 analogaux;
static u8 packet[PACKET_SIZE];
static u8 tx_power;
static u8 txid[3];
static u8 bind_chan;
static u8 rf_chan;
static u8 rf_channels[RF_NUM_CHANNELS];
static u8 rx_tx_addr[ADDRESS_LENGTH];

static u16 telemetry_count = 0;
static u16 last_telemetry_count = 0;
static u16 loopcount = 0;
static u8 bay_count = 0;

// Bit vector from bit position
#define BV(bit) (1 << bit)

static u8 checksum()
{
    u8 sum = packet[0];
    for (int i = 1; i < PACKET_SIZE - 1; i++)
        sum += packet[i];
    return sum;
}

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

#define DYNTRIM(chval) ((u8)((chval >> 2) & 0xfc))
#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void send_packet(u8 bind)
{
    union {
        u16 value;
        struct {
            u8 lsb;
            u8 msb;
        } bytes;
    } chanval;

    if (bind) {
        if (telemetry) {
            if (analogaux)
                packet[0] = 0xa1;
            else
                packet[0] = 0xa3;
        }
        else if (analogaux)
            packet[0] = 0xa2;
        else
            packet[0] = 0xa4;

        memcpy(&packet[1], rx_tx_addr, 5);
        memcpy(&packet[6], rf_channels, 4);
        switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
                case FORMAT_REGULAR:
                    packet[10] = txid[0];
                    packet[11] = txid[1];
                    break;
                case FORMAT_X16_AH:
                    packet[10] = 0x00;
                    packet[11] = 0x00;
                    break;
                case FORMAT_IRDRONE:
                    packet[10] = 0x30;
                    packet[11] = 0x01;
                    break;
        }

    } else {
        switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
                case FORMAT_REGULAR:
                    packet[0] = 0xa5;
                    break;
                case FORMAT_X16_AH:
                case FORMAT_IRDRONE:
                    packet[0] = 0xa6;
                    break;
        }
        if (analogaux) {
            packet[1] = scale_channel(CHANNEL_ANAAUX1, 0, 0xff);
        } else {
            packet[1] = 0xfa;       // normal mode is 0xf7, expert 0xfa
        }
        packet[2] = GET_FLAG(CHANNEL_FLIP, 0x08)
            | GET_FLAG(CHANNEL_HEADLESS, 0x02)
            | GET_FLAG(CHANNEL_RTH, 0x01)
            | GET_FLAG(CHANNEL_VIDEO, 0x10)
            | GET_FLAG(CHANNEL_PICTURE, 0x20);
        packet[3] = GET_FLAG(CHANNEL_INVERTED, 0x80)
            | GET_FLAG(CHANNEL_TO, 0x20)
            | GET_FLAG(CHANNEL_EMGSTOP, 0x04);
        chanval.value = scale_channel(CHANNEL1, 0x3ff, 0);      // aileron
        packet[4] = chanval.bytes.msb + DYNTRIM(chanval.value);
        packet[5] = chanval.bytes.lsb;
        chanval.value = scale_channel(CHANNEL2, 0, 0x3ff);      // elevator
        packet[6] = chanval.bytes.msb + DYNTRIM(chanval.value);
        packet[7] = chanval.bytes.lsb;
        chanval.value = scale_channel(CHANNEL3, 0, 0x3ff);      // throttle
        packet[8] = chanval.bytes.msb + 0x7c;
        packet[9] = chanval.bytes.lsb;
        chanval.value = scale_channel(CHANNEL4, 0x3ff, 0);      // rudder
        packet[10] = chanval.bytes.msb + DYNTRIM(chanval.value);
        packet[11] = chanval.bytes.lsb;
    }

    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_REGULAR:
                packet[12] = txid[2];
                if (analogaux) {
                    packet[13] = scale_channel(CHANNEL_ANAAUX2, 0, 0xff);
                } else {
                    packet[13] = 0x0a;
                }
                break;
            case FORMAT_X16_AH:
                packet[12] = 0x00;
                packet[13] = 0x00;
                break;
            case FORMAT_IRDRONE:
                packet[12] = 0xe0;
                packet[13] = 0x2e;
                break;
    }

    packet[14] = checksum();



    NRF24L01_WriteReg(NRF24L01_05_RF_CH,
              bind ? bind_chan : rf_channels[rf_chan++]);

    rf_chan %= sizeof(rf_channels);

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();

    XN297_WritePayload(packet, PACKET_SIZE);

    NRF24L01_SetTxRxMode(TXRX_OFF);
    NRF24L01_SetTxRxMode(TX_EN);

    // Power on, TX mode, 2byte CRC
    // Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) |
                    BV(NRF24L01_00_PWR_UP));


    if (telemetry) {
        // switch radio to rx, no crc
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);
    }
    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
#ifdef EMULATOR
    dbgprintf("next chan 0x%02x, bind %d, data %02x",
              bind ? bind_chan : rf_channels[rf_chan], bind,
              packet[0]);
    for (int i = 1; i < PACKET_SIZE; i++)
        dbgprintf(" %02x", packet[i]);
    dbgprintf("\n");
#endif
}




static int check_rx(void)
{

    union {
        u16 value;
        struct {
            u8 lsb;
            u8 msb;
        } bytes;
    } chanval;

    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
        // data received from aircraft
        XN297_ReadPayload(packet, PACKET_SIZE);

        NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);

        NRF24L01_FlushRx();
        // decode data , check sum is ok as well, since there is no crc
        if (packet[0] == 0x85 && packet[14] == checksum()) {
            // uncompensated battery volts*100
            chanval.bytes.msb = packet[3] & 0x7;
            u8 flags = packet[3] >> 3;
            chanval.bytes.lsb = packet[4] & 0xff;
            Telemetry.value[TELEM_DSM_FLOG_VOLT1] = chanval.value;
            TELEMETRY_SetUpdated(TELEM_DSM_FLOG_VOLT1);

            // compensated battery volts*100
            chanval.bytes.msb = packet[5] & 0x7;
            chanval.bytes.lsb = packet[6] & 0xff;
            Telemetry.value[TELEM_DSM_FLOG_VOLT2] = chanval.value;
            TELEMETRY_SetUpdated(TELEM_DSM_FLOG_VOLT2);

            // reception in packets / sec , multiplied by 2
            Telemetry.value[TELEM_DSM_FLOG_HOLDS] = packet[7] * 2;
            TELEMETRY_SetUpdated(TELEM_DSM_FLOG_HOLDS);

            // battery low flag
            Telemetry.value[TELEM_DSM_FLOG_FADESL] = (flags & 1) ? 100 : 0;
            TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FADESL);

            telemetry_count++;
            return 1;
        }                       // end tel received

    }
    return 0;
}

static void bay_init()
{
    u8 bind_address[] = { 0, 0, 0, 0, 0 };
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
            case FORMAT_REGULAR:
                bind_chan = RF_BIND_CHANNEL;
                break;
            case FORMAT_X16_AH:
            case FORMAT_IRDRONE:
                bind_chan = RF_BIND_CHANNEL_X16_AH;
                break;
    }
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    // SPI trace of stock TX has these writes to registers that don't appear in
    // nRF24L01 or Beken 2421 datasheets.  Uncomment if you have an XN297 chip?
    // NRF24L01_WriteRegisterMulti(0x3f, "\x4c\x84\x67,\x9c,\x20", 5);
    // NRF24L01_WriteRegisterMulti(0x3e, "\xc9\x9a\xb0,\x61,\xbb,\xab,\x9c", 7);
    // NRF24L01_WriteRegisterMulti(0x39, "\x0b\xdf\xc4,\xa7,\x03,\xab,\x9c", 7);

    XN297_SetTXAddr(bind_address, ADDRESS_LENGTH);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00); // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);

    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PACKET_SIZE);

    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);    // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_1M);        // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);    // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00); // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
    NRF24L01_SetTxRxMode(TX_EN);
}



static u16 bay_callback()
{
    switch (phase) {
    case Bayang_INIT1:
        phase = Bayang_BIND2;
        break;

    case Bayang_BIND2:
        if (counter == 0) {
            XN297_SetTXAddr(rx_tx_addr, ADDRESS_LENGTH);
            XN297_SetRXAddr(rx_tx_addr, ADDRESS_LENGTH);
            phase = Bayang_DATA;
            PROTOCOL_SetBindState(0);
        } else {
            if (bay_count == 0)
                send_packet(1);
            bay_count++;
            bay_count %= 4;
            counter -= 1;
        }
        break;

    case Bayang_DATA:
        if (telemetry) {
            // telemetry is enabled
            loopcount++;
            if (loopcount > 1000) {
                //calculate telemetry reception packet rate - packets per second
                loopcount = 0;
                Telemetry.value[TELEM_DSM_FLOG_FRAMELOSS] =
                    telemetry_count;
                // set updated if lower than previous, so an alarm can be used when telemetry lost but does not go on forever
                if (telemetry_count < last_telemetry_count)
                    TELEMETRY_SetUpdated(TELEM_DSM_FLOG_FRAMELOSS);
                last_telemetry_count = telemetry_count;
                telemetry_count = 0;
            }

            if (bay_count == 0) {
                send_packet(0);
            } else {
                check_rx();
            }

            bay_count++;
            bay_count %= 5;
        } else {
            // no telemetry
            if (bay_count == 0) {
                send_packet(0);
            }
            bay_count++;
            bay_count %= 2;
        }
        break;
    }
    return PACKET_PERIOD;
}

static void initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;

#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if (Model.fixed_id) {
        for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
            rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i)
        rand32_r(&lfsr, 0);

    txid[0] = (lfsr >> 8) & 0xff;
    txid[1] = (lfsr >> 16) & 0xff;
    txid[2] = (lfsr >> 24) & 0xff;
    rx_tx_addr[0] = lfsr & 0xff;
    for (u8 i = 0; i < sizeof(lfsr); ++i)
        rand32_r(&lfsr, 0);
    rx_tx_addr[1] = lfsr & 0xff;
    rx_tx_addr[2] = (lfsr >> 8) & 0xff;
    rx_tx_addr[3] = (lfsr >> 16) & 0xff;
    rx_tx_addr[4] = (lfsr >> 24) & 0xff;
    for (u8 i = 0; i < sizeof(lfsr); ++i)
        rand32_r(&lfsr, 0);
    rf_channels[0] = 0;
    rf_channels[1] = (lfsr & 0x1f) + 0x10;
    rf_channels[2] = rf_channels[1] + 0x20;
    rf_channels[3] = rf_channels[2] + 0x20;

}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    counter = BIND_COUNT;

    last_telemetry_count = 0;
    telemetry_count = 0;
    telemetry = Model.proto_opts[PROTOOPTS_TELEMETRY];

    // Enable analog aux channels
    analogaux = Model.proto_opts[PROTOOPTS_ANALOGAUX];

    // set some initial values to show nothing has been received
    if (telemetry)
        Telemetry.value[TELEM_DSM_FLOG_VOLT1] = Telemetry.value[TELEM_DSM_FLOG_VOLT2] = 888;    //8.88V In 1/100 of Volts

    initialize_txid();
    bay_init();
    phase = Bayang_INIT1;

    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    CLOCK_StartTimer(INITIAL_WAIT, bay_callback);
}

uintptr_t Bayang_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
    case PROTOCMD_INIT:
        initialize();
        return 0;
    case PROTOCMD_DEINIT:
    case PROTOCMD_RESET:
        CLOCK_StopTimer();
        return (NRF24L01_Reset()? 1 : -1);
    case PROTOCMD_CHECK_AUTOBIND:
        return 1;     // always Autobind
    case PROTOCMD_BIND:
        initialize();
        return 0;
    case PROTOCMD_NUMCHAN:
        return 14;
    case PROTOCMD_DEFAULT_NUMCHAN:
        return 14;
    case PROTOCMD_CURRENT_ID:
        return Model.fixed_id;
    case PROTOCMD_GETOPTIONS:
        return (uintptr_t)bay_opts;
    case PROTOCMD_TELEMETRYSTATE:
        return (Model.proto_opts[PROTOOPTS_TELEMETRY] ==
                                TELEM_ON ? PROTO_TELEM_ON :
                                PROTO_TELEM_OFF);
    case PROTOCMD_TELEMETRYTYPE:
        return TELEM_DSM;
    case PROTOCMD_CHANNELMAP:
        return AETRG;
    default:
        break;
    }
    return 0;
}
#endif
