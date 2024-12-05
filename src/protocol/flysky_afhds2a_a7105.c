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
#include "config/tx.h"
#include "telemetry.h"

#ifdef PROTO_HAS_A7105

#define MAX(A, B) (A > B ? A : B)
#define TXPACKET_SIZE 38
#define RXPACKET_SIZE (9 + 7 * 6)
#define NUMFREQ       16
#define TXID_SIZE     4
#define RXID_SIZE     4

static u8 packet[MAX(TXPACKET_SIZE, RXPACKET_SIZE)];
static u8 txid[TXID_SIZE];
static u8 rxid[RXID_SIZE];
static u8 hopping_frequency[NUMFREQ];
static u8 packet_type;
static s32 packet_count;
static u8 bind_reply;
static u8 state;
static u8 channel;
static u8 tx_power;
static s16 freq_offset;

static const u8 AFHDS2A_regs[] = {
    -1  , 0x42 | (1<<5), 0x00, 0x25, 0x00,   -1,   -1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x3c, 0x05, 0x00, 0x50, // 00 - 0f
    0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80,   -1,   -1, 0x2a, 0x32, 0xc3, 0x1f, // 10 - 1f
    0x1e,   -1, 0x00,   -1, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00, // 20 - 2f
    0x01, 0x0f // 30 - 31
};

enum{
    PACKET_STICKS,
    PACKET_SETTINGS,
    PACKET_FAILSAFE,
};

enum{
    BIND1,
    BIND2,
    BIND3,
    BIND4,
    DATA1,
};

static const char * const afhds2a_opts[] = {
    _tr_noop("Outputs"), "PWM/IBUS", "PPM/IBUS", "PWM/SBUS", "PPM/SBUS", NULL,
    _tr_noop("Servo Hz"), "50", "400", "5", NULL,
    _tr_noop("LQI output"), "None", "Ch5", "Ch6", "Ch7", "Ch8", "Ch9", "Ch10", "Ch11", "Ch12", "Ch13", "Ch14", NULL,
    _tr_noop("Freq Tune"), "-300", "300", "655361", NULL, // big step 10, little step 1
    "RX ID", "-32768", "32767", "1", NULL, // todo: store that elsewhere
    "RX ID2","-32768", "32767", "1", NULL, // ^^^^^^^^^^^^^^^^^^^^^^^^^^
    NULL
};

enum {
    PWM_IBUS = 0,
    PPM_IBUS,
    PWM_SBUS,
    PPM_SBUS
};

enum {
    PROTOOPTS_OUTPUTS = 0,
    PROTOOPTS_SERVO_HZ,
    PROTOOPTS_LQI_OUT,
    PROTOOPTS_FREQTUNE,
    PROTOOPTS_RXID, // todo: store that elsewhere
    PROTOOPTS_RXID2,// ^^^^^^^^^^^^^^^^^^^^^^^^^^
    LAST_PROTO_OPT,
};

ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

static int afhds2a_init()
{
    u8 i;
    u8 if_calibration1;
    u8 vco_calibration0;
    u8 vco_calibration1;

    A7105_WriteID(0x5475c52a);
    for (i = 0; i < sizeof(AFHDS2A_regs); i++)
        if((s8)AFHDS2A_regs[i] != -1)
            A7105_WriteReg(i, AFHDS2A_regs[i]);

    A7105_Strobe(A7105_STANDBY);
    A7105_SetTxRxMode(TX_EN);

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
        return 0;
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
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
        return 0;
    vco_calibration0 = A7105_ReadReg(0x25);
    if (vco_calibration0 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Calibrate channel 0xa0?
    //Set Channel
    A7105_WriteReg(0x0f, 0xa0);
    //VCO Calibration
    A7105_WriteReg(0x02, 2);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(A7105_02_CALC))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    vco_calibration1 = A7105_ReadReg(0x25);
    if (vco_calibration1 & A7105_MASK_VBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //Reset VCO Band calibration
    A7105_WriteReg(0x25, 0x0A);

    A7105_SetTxRxMode(TX_EN);

    A7105_SetPower(Model.tx_power);
    tx_power = Model.tx_power;

    freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
    A7105_AdjustLOBaseFreq(freq_offset);

    A7105_Strobe(A7105_STANDBY);
    A7105_SetTxRxMode(TX_EN);
    return 1;
}

static void build_sticks_packet()
{
    packet[0] = 0x58;
    memcpy( &packet[1], txid, 4);
    memcpy( &packet[5], rxid, 4);
    for(u8 ch=0; ch<14; ch++)
    {
        if (ch >= Model.num_channels) {
            packet[9 + ch*2] = 0xdc;
            packet[10 + ch*2] = 0x05;
            continue;
        }
        s32 value = (s32)Channels[ch] * 500 / CHAN_MAX_VALUE + 1500;
        if (value < 875)
            value = 875;
        else if (value > 2125)
            value = 2125;
        packet[9 +  ch*2] = value & 0xff;
        packet[10 + ch*2] = (value >> 8) & 0xff;
    }
    // override channel with telemetry LQI
    if(Model.proto_opts[PROTOOPTS_LQI_OUT] > 0) {
        u16 val = 1000 + (Telemetry.value[TELEM_FRSKY_LQI] * 10);
        packet[17+((Model.proto_opts[PROTOOPTS_LQI_OUT]-1)*2)] = val & 0xff;
        packet[18+((Model.proto_opts[PROTOOPTS_LQI_OUT]-1)*2)] = (val >> 8) & 0xff;
    }
    packet[37] = 0x00;
}

static void build_failsafe_packet()
{
    packet[0] = 0x56;
    memcpy( &packet[1], txid, 4);
    memcpy( &packet[5], rxid, 4);
    for(u8 ch=0; ch<14; ch++) {
        if(ch < Model.num_channels && (Model.limits[ch].flags & CH_FAILSAFE_EN)) {
            s32 value = ((s32)Model.limits[ch].failsafe + 100) * 5 + 1000;
            packet[9 + ch*2] = value & 0xff;
            packet[10+ ch*2] = (value >> 8) & 0xff;
        }
        else {
            packet[9 + ch*2] = 0xff;
            packet[10+ ch*2] = 0xff;
        }
    }
    packet[37] = 0x00;
}

static void build_settings_packet()
{
    packet[0] = 0xaa;
    memcpy( &packet[1], txid, 4);
    memcpy( &packet[5], rxid, 4);
    packet[9] = 0xfd;
    packet[10]= 0xff;
    packet[11]= Model.proto_opts[PROTOOPTS_SERVO_HZ] & 0xff;
    packet[12]= (Model.proto_opts[PROTOOPTS_SERVO_HZ] >> 8) & 0xff;
    if(Model.proto_opts[PROTOOPTS_OUTPUTS] == PPM_IBUS || Model.proto_opts[PROTOOPTS_OUTPUTS] == PPM_SBUS)
        packet[13] = 0x01; // PPM output enabled
    else
        packet[13] = 0x00;
    packet[14]= 0x00;
    for(u8 i=15; i<37; i++)
        packet[i] = 0xff;
    packet[18] = 0x05; // ?
    packet[19] = 0xdc; // ?
    packet[20] = 0x05; // ?
    if(Model.proto_opts[PROTOOPTS_OUTPUTS] == PWM_SBUS || Model.proto_opts[PROTOOPTS_OUTPUTS] == PPM_SBUS)
        packet[21] = 0xdd; // SBUS output enabled
    else
        packet[21] = 0xde;
    packet[37] = 0x00;
}

static void build_packet(u8 type)
{
    switch(type) {
        case PACKET_STICKS:
            build_sticks_packet();
            break;
        case PACKET_SETTINGS:
            build_settings_packet();
            break;
        case PACKET_FAILSAFE:
            build_failsafe_packet();
            break;
    }
}

// telemetry sensors ID
enum{
    SENSOR_VOLTAGE        = 0x00,    // Internal Voltage
    SENSOR_TEMPERATURE    = 0x01,    // Temperature
    SENSOR_MOT            = 0x02,    // RPM
    SENSOR_EXTV           = 0x03,    // External Voltage
    SENSOR_CELL_VOLTAGE   = 0x04,    // Avg Cell voltage
// TODO edgetx script says SENSOR_TYPE_GYROSCOPE_1_AXIS = 0x04,    // signed short in 0.1 degrees per second, 0x8000 if unknown
    SENSOR_BAT_CURR       = 0x05,    // battery current A * 100
    SENSOR_FUEL           = 0x06,    // remaining battery percentage / mah drawn otherwise or fuel level no unit!
    SENSOR_RPM            = 0x07,    // throttle value / battery capacity
    SENSOR_CMP_HEAD       = 0x08,    // Heading  0..360 deg, 0=north 2bytes
    SENSOR_CLIMB_RATE     = 0x09,    // 2 bytes m/s *100
    SENSOR_COG            = 0x0a,    // 2 bytes  Course over ground(NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. unknown max uint
    SENSOR_GPS_STATUS     = 0x0b,    // 2 bytes
    SENSOR_ACC_X          = 0x0c,    // 2 bytes m/s *100 signed
    SENSOR_ACC_Y          = 0x0d,    // 2 bytes m/s *100 signed
    SENSOR_ACC_Z          = 0x0e,    // 2 bytes m/s *100 signed
    SENSOR_ROLL           = 0x0f,    // 2 bytes deg *100 signed
    SENSOR_PITCH          = 0x10,    // 2 bytes deg *100 signed
    SENSOR_YAW            = 0x11,    // 2 bytes deg *100 signed
    SENSOR_VERTICAL_SPEED = 0x12,    // 2 bytes m/s *100
    SENSOR_GROUND_SPEED   = 0x13,    // 2 bytes m/s *100 different unit than build-in sensor
    SENSOR_GPS_DIST       = 0x14,    // 2 bytes dist from home m unsigned
    SENSOR_ARMED          = 0x15,    // 2 bytes
    SENSOR_FLIGHT_MODE    = 0x16,    // 2 bytes simple index listed below

    SENSOR_PRES           = 0x41,    // Pressure
    SENSOR_ODO1           = 0x7c,    // Odometer1
    SENSOR_ODO2           = 0x7d,    // Odometer2
    SENSOR_SPE            = 0x7e,    // Speed            //2byte km/h
    SENSOR_TX_V           = 0x7f,    // TX Voltage

    // 4 byte sensors
    SENSOR_GPS_LAT        = 0x80,    // 4bytes signed WGS84 in degrees * 1E7
    SENSOR_GPS_LON        = 0x81,    // 4bytes signed WGS84 in degrees * 1E7
    SENSOR_GPS_ALT        = 0x82,    // 4bytes signed!!! GPS alt m*100
    SENSOR_ALT            = 0x83,    // 4bytes signed!!! Alt m*100
    SENSOR_S84            = 0x84,
    SENSOR_S85            = 0x85,
    SENSOR_S86            = 0x86,
    SENSOR_S87            = 0x87,
    SENSOR_S88            = 0x88,
    SENSOR_S89            = 0x89,
    SENSOR_S8a            = 0x8a,

// commented out in MAVLink    SENSOR_ALT_FLYSKY     = 0xf9,    // Altitude         //2 bytes signed in m
    SENSOR_RX_SNR         = 0xfa,    // SNR
    SENSOR_RX_NOISE       = 0xfb,    // Noise
    SENSOR_RX_RSSI        = 0xfc,    // RSSI
    SENSOR_RX_ERR_RATE    = 0xfe,    // Error rate
    SENSOR_UNKNOWN        = 0xff,

// AC type telemetry with multiple values in one packet
    SENSOR_GPS_FULL       = 0xfd,
    SENSOR_VOLT_FULL      = 0xf0,
    SENSOR_ACC_FULL       = 0xef,
};

static void set_telemetry(frsky_telem_t offset, s32 value) {
    Telemetry.value[offset] = value;
    TELEMETRY_SetUpdated(offset);
}

// copied from edgetx
const int16_t tAltitude[225]=
{ // In half meter unit
    20558, 20357, 20158, 19962, 19768, 19576, 19387, 19200, 19015,  18831, 18650, 18471, 18294, 18119, 17946, 17774,
    17604, 17436, 17269, 17105, 16941, 16780, 16619, 16461, 16304,  16148, 15993, 15841, 15689, 15539, 15390, 15242,
    15096, 14950, 14806, 14664, 14522, 14381, 14242, 14104, 13966,  13830, 13695, 13561, 13428, 13296, 13165, 13035,
    12906, 12777, 12650, 12524, 12398, 12273, 12150, 12027, 11904,  11783, 11663, 11543, 11424, 11306, 11189, 11072,
    10956, 10841, 10726, 10613, 10500, 10387, 10276, 10165, 10054,   9945,  9836,  9727,  9620,  9512,  9406,  9300,
    9195,  9090,  8986,  8882,  8779,  8677,   8575,  8474,  8373,   8273,  8173,  8074,  7975,  7877,  7779,  7682,
    7585,  7489,  7394,  7298,  7204,  7109,   7015,  6922,  6829,   6737,  6645,  6553,  6462,  6371,  6281,  6191,
    6102,  6012,  5924,  5836,  5748,  5660,   5573,  5487,  5400,   5314,  5229,  5144,  5059,  4974,  4890,  4807,
    4723,  4640,  4557,  4475,  4393,  4312,   4230,  4149,  4069,   3988,  3908,  3829,  3749,  3670,  3591,  3513,
    3435,  3357,  3280,  3202,  3125,  3049,   2972,  2896,  2821,   2745,  2670,  2595,  2520,  2446,  2372,  2298,
    2224,  2151,  2078,  2005,  1933,   1861,  1789,  1717,  1645,   1574,  1503,  1432,  1362,  1292,  1222,  1152,
    1082,  1013,   944,   875,   806,   738,    670,   602,   534,    467,   399,   332,   265,   199,   132,    66,
     0,     -66,  -131,  -197,  -262,  -327,   -392,  -456,  -521,   -585,  -649,  -713,  -776,  -840,  -903,  -966,
    -1029,-1091, -1154, -1216, -1278, -1340,  -1402, -1463,  -1525, -1586, -1647, -1708, -1769, -1829, -1889, -1950,
    -2010
};
#define PRESSURE_MASK 0x7FFFF
int32_t getALT(uint32_t Pressure)
{
    uint32_t Index;
    int32_t Altitude1;
    int32_t Altitude2;
    uint32_t Decimal;
    uint64_t Ratio;
    uint32_t SeaLevelPressure=101320;
    Pressure = Pressure & PRESSURE_MASK;
    Ratio = ( ( ( unsigned long long ) Pressure << 16 ) + ( SeaLevelPressure / 2 ) ) / SeaLevelPressure;
    if( Ratio < ( ( 1 << 16 ) * 250 / 1000 ) )// 0.250 inclusive
    {
        Ratio = ( 1 << 16 ) * 250 / 1000;
    }
    else if( Ratio > ( 1 << 16 ) * 1125 / 1000 - 1 ) // 1.125 non-inclusive
    {
        Ratio = ( 1 << 16 ) * 1125 / 1000 - 1;
    }

    Ratio -= ( 1 << 16 ) * 250 / 1000; // from 0.000 (inclusive) to 0.875 (non-inclusive)
    Index = Ratio >> 8;
    Decimal = Ratio & ( ( 1 << 8 ) - 1 );
    Altitude1 = tAltitude[Index];
    Altitude2 = Altitude1 - tAltitude[Index + 1];
    Altitude1 = Altitude1 - ( Altitude2 * Decimal + ( 1 << 7 ) ) / ( 1 << 8 );
    Altitude1 *= 100;
    if( Altitude1 >= 0 )
    {
        return( ( Altitude1 + 1 ) / 2 );
    }
    else
    {
        return( ( Altitude1 - 1 ) / 2 );
    }
}

static void update_telemetry()
{
    // 0    1234   5678   9           10         11       12
    // AA | TXID | RXID | sensor id | sensor # | value 16 bit big endian | sensor id ......
    // AC | TXID | RXID | sensor id | sensor # | length | value 32 bit big endian | sensor id ......
    // max 7 sensors per packet

    u8 voltage_index = 0;
    u8 *sensor = packet + 9;   // first sensor ID in telemetry packet
#if HAS_EXTENDED_TELEMETRY
    u8 cell_index = 0;
    u16 cell_total = 0;
#endif

    while (sensor[0] != 0xff && (sensor+9 < packet+RXPACKET_SIZE-6)) {
        u16 data16 = sensor[3] << 8 | sensor[2];
#if HAS_EXTENDED_TELEMETRY
        s32 data32 = sensor[6] << 24 | sensor[5] << 16 | sensor[4] << 8 | sensor[3];
#endif
        switch(sensor[0]) {
        case SENSOR_VOLTAGE:
            voltage_index++;
            if (sensor[1] == 0)  // Rx voltage
            {
                set_telemetry(TELEM_FRSKY_VOLT1, data16);
            }
            else if (voltage_index == 2)  // external voltage sensor #1
            {
                set_telemetry(TELEM_FRSKY_VOLT2, data16);
            }
#if HAS_EXTENDED_TELEMETRY
            else if (voltage_index == 3)  // external voltage sensor #2
            {
                set_telemetry(TELEM_FRSKY_VOLT3, data16);
            }
#endif
            break;
        case SENSOR_EXTV:
            voltage_index++;
            set_telemetry(TELEM_FRSKY_VOLT2, data16);
            break;
#if HAS_EXTENDED_TELEMETRY
        case SENSOR_TEMPERATURE:
        {
            u8 temp_index = sensor[1];
            if (temp_index == 0) {
                set_telemetry(TELEM_FRSKY_TEMP1, (data16 - 400)/10);
            } else if (temp_index == 1) {
                set_telemetry(TELEM_FRSKY_TEMP2, (data16 - 400)/10);
            }
            break;
        }
        case SENSOR_PRES:
            set_telemetry(TELEM_FRSKY_TEMP1, ((data32 >> 19) - 400)/10);
            // simplified pressure to altitude calculation, since very linear
            // below 2000m and display is relative (AGL)
            // linear approximation of barometric formula in ISA at low altitude
            // y = -0.079x + 1513, scaled to centimeters
            //int altitude = (int)(-0.08 * (uint32_t)(data32 & 0x7ffff) + 1513) * 100;
            int altitude = getALT(data32);
            if (Model.ground_level == 0) Model.ground_level = altitude;
            set_telemetry(TELEM_FRSKY_ALTITUDE, altitude - Model.ground_level);
            break;
        case SENSOR_CELL_VOLTAGE:
            if (cell_index < 6) {
                set_telemetry(TELEM_FRSKY_CELL1 + cell_index, data16);
                cell_total += data16;
            }
            cell_index++;
            break;
        case SENSOR_CLIMB_RATE:
            set_telemetry(TELEM_FRSKY_VARIO, data16);
            break;
        case SENSOR_ALT:
            set_telemetry(TELEM_FRSKY_ALTITUDE, data32);
            break;
        case SENSOR_FUEL:
            set_telemetry(TELEM_FRSKY_FUEL, data16);
            break;
        case SENSOR_BAT_CURR:
            set_telemetry(TELEM_FRSKY_CURRENT, data16 * 10);
            break;
        case SENSOR_MOT:
            set_telemetry(TELEM_FRSKY_RPM, data16);
            break;
        case SENSOR_GPS_LON:
            Telemetry.gps.longitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_LONG);
            break;
        case SENSOR_GPS_LAT:
            Telemetry.gps.latitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_LAT);
            break;
        case SENSOR_GPS_ALT:
            Telemetry.gps.altitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_ALT);
            break;
        case SENSOR_GPS_FULL: {
            sensor += 5;     // skip GPS status
            data32 = sensor[3] << 24 | sensor[2] << 16 | sensor[1] << 8 | sensor[0];
            Telemetry.gps.latitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_LAT);
            sensor += 4;
            data32 = sensor[3] << 24 | sensor[2] << 16 | sensor[1] << 8 | sensor[0];
            Telemetry.gps.longitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_LONG);
            sensor += 4;
            data32 = sensor[3] << 24 | sensor[2] << 16 | sensor[1] << 8 | sensor[0];
            Telemetry.gps.altitude = data32;
            TELEMETRY_SetUpdated(TELEM_GPS_ALT);
        }
            break;
#endif
        case SENSOR_RX_SNR:
//            set_telemetry(TELEM_FRSKY_LQI, 100 - sensor[2]);
            break;
        case SENSOR_RX_NOISE:
//            set_telemetry(TELEM_FRSKY_LQI, 100 - sensor[2]);
            break;
        case SENSOR_RX_RSSI:
            set_telemetry(TELEM_FRSKY_RSSI, sensor[2]);
            break;
        case SENSOR_RX_ERR_RATE:
            set_telemetry(TELEM_FRSKY_LQI, 100 - sensor[2]);
            break;
        default:
            // unknown sensor ID or end of list
            break;
        }
        if (packet[0] == 0xaa)
            sensor += ((sensor[0] & 0xf0) == 0x80 ? 6 : 4);
        else
            sensor += sensor[2] + 3;
    }
#if HAS_EXTENDED_TELEMETRY
    if(cell_index > 0) {
        Telemetry.value[TELEM_FRSKY_ALL_CELL] = cell_total;
        TELEMETRY_SetUpdated(TELEM_FRSKY_ALL_CELL);
    }
#endif
}

static void build_bind_packet()
{
    u8 ch;
    memcpy( &packet[1], txid, 4);
    memset( &packet[5], 0xff, 4);
    packet[10]= 0x00;
    for(ch=0; ch<16; ch++) {
        packet[11+ch] = hopping_frequency[ch];
    }
    memset( &packet[27], 0xff, 10);
    packet[37] = 0x00;
    switch(state) {
        case BIND1:
            packet[0] = 0xbb;
            packet[9] = 0x01;
            break;
        case BIND2:
        case BIND3:
        case BIND4:
            packet[0] = 0xbc;
            if(state == BIND4) {
                memcpy( &packet[5], &rxid, 4);
                memset( &packet[11], 0xff, 16);
            }
            packet[9] = state-1;
            if(packet[9] > 0x02)
                packet[9] = 0x02;
            packet[27]= 0x01;
            packet[28]= 0x80;
            break;
    }
}

static void calc_fh_channels(uint32_t seed)
{
    int idx = 0;
    uint32_t rnd = seed;
    while (idx < NUMFREQ) {
        int i;
        int count_1_42 = 0, count_43_85 = 0, count_86_128 = 0, count_129_168=0;
        rnd = rnd * 0x0019660D + 0x3C6EF35F; // Randomization

        uint8_t next_ch = ((rnd >> (idx%32)) % 0xa8) + 1;
        // Keep the distance 2 between the channels - either odd or even
        if (((next_ch ^ seed) & 0x01 )== 0)
            continue;
        // Check that it's not duplicate and spread uniformly
        for (i = 0; i < idx; i++) {
            if(hopping_frequency[i] == next_ch)
                break;
            if(hopping_frequency[i] <= 42)
                count_1_42++;
            else if (hopping_frequency[i] <= 85)
                count_43_85++;
            else if (hopping_frequency[i] <= 128)
                count_86_128++;
            else
                count_129_168++;
        }
        if (i != idx)
            continue;
        if ((next_ch <= 42 && count_1_42 < 5)
          ||(next_ch >= 43 && next_ch <= 85 && count_43_85 < 5)
          ||(next_ch >= 86 && next_ch <=128 && count_86_128 < 5)
          ||(next_ch >= 129 && count_129_168 < 5))
        {
            hopping_frequency[idx++] = next_ch;
        }
    }
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void initialize_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    printf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        printf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    printf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }

    // Pump zero bytes for LFSR to diverge more
    for (int i = 0; i < TXID_SIZE; ++i) rand32_r(&lfsr, 0);

    for (u8 i = 0; i < TXID_SIZE; ++i) {
        txid[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }

    // Use LFSR to seed frequency hopping sequence after another
    // divergence round
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
    calc_fh_channels(lfsr);
}

#define WAIT_WRITE 0x80

static u16 afhds2a_cb()
{
    // keep frequency tuning updated
    if(freq_offset != Model.proto_opts[PROTOOPTS_FREQTUNE]) {
            freq_offset = Model.proto_opts[PROTOOPTS_FREQTUNE];
            A7105_AdjustLOBaseFreq(freq_offset);
    }
    switch(state) {
        case BIND1:
        case BIND2:
        case BIND3:
            A7105_Strobe(A7105_STANDBY);
            A7105_SetTxRxMode(TX_EN);
            build_bind_packet();
            A7105_WriteData(packet, 38, packet_count%2 ? 0x0d : 0x8c);
            if(A7105_ReadReg(0) == 0x1b) { // todo: replace with check crc+fec
                A7105_Strobe(A7105_RST_RDPTR);
                A7105_ReadData(packet, RXPACKET_SIZE);
                if(packet[0] == 0xbc) {
                    for(u8 i=0; i<4; i++) {
                        rxid[i] = packet[5+i];
                    }
                    Model.proto_opts[PROTOOPTS_RXID] = (u16)rxid[0]<<8 | rxid[1];
                    Model.proto_opts[PROTOOPTS_RXID2]= (u16)rxid[2]<<8 | rxid[3];
                    if(packet[9] == 0x01)
                        state = BIND4;
                }
            }
            packet_count++;
            state |= WAIT_WRITE;
            return 1700;

        case BIND1|WAIT_WRITE:
        case BIND2|WAIT_WRITE:
        case BIND3|WAIT_WRITE:
            A7105_SetTxRxMode(RX_EN);
            A7105_Strobe(A7105_RX);
            state &= ~WAIT_WRITE;
            state++;
            if(state > BIND3)
                state = BIND1;
            return 2150;

        case BIND4:
            A7105_Strobe(A7105_STANDBY);
            A7105_SetTxRxMode(TX_EN);
            build_bind_packet();
            A7105_WriteData(packet, 38, packet_count%2 ? 0x0d : 0x8c);
            packet_count++;
            bind_reply++;
            if (bind_reply >= 4) {
                packet_count=0;
                channel=1;
                state = DATA1;
                PROTOCOL_SetBindState(0);
            }
            state |= WAIT_WRITE;
            return 1700;

        case BIND4|WAIT_WRITE:
            A7105_SetTxRxMode(RX_EN);
            A7105_Strobe(A7105_RX);
            state &= ~WAIT_WRITE;
            return 2150;

        case DATA1:
            A7105_Strobe(A7105_STANDBY);
            A7105_SetTxRxMode(TX_EN);
            build_packet(packet_type);
            A7105_WriteData(packet, 38, hopping_frequency[channel++]);
            if(channel >= 16)
                channel = 0;
            if(!(packet_count % 1313))
                packet_type = PACKET_SETTINGS;
            else if(!(packet_count % 1569))
                packet_type = PACKET_FAILSAFE;
            else
                packet_type = PACKET_STICKS; // todo : check for settings changes
            // keep transmit power in sync
            if(tx_power != Model.tx_power) {
                A7105_SetPower(Model.tx_power);
                tx_power = Model.tx_power;
            }
            // got some data from RX ?
            // we've no way to know if RX fifo has been filled
            // as we can't poll GIO1 or GIO2 to check WTR
            // we can't check A7105_MASK_TREN either as we know
            // it's currently in transmit mode.
            if(!(A7105_ReadReg(0) & (1<<5 | 1<<6))) { // FECF+CRCF Ok
                A7105_Strobe(A7105_RST_RDPTR);
                u8 check;
                A7105_ReadData(&check,1);
                if (check == 0xaa || check == 0xac) {
                    A7105_Strobe(A7105_RST_RDPTR);
                    A7105_ReadData(packet, RXPACKET_SIZE);
                    if (packet[9] == 0xfc) {  // rx is asking for settings
                        packet_type=PACKET_SETTINGS;
                    } else {
                        update_telemetry();
                    }
                }
            }
            packet_count++;
            state |= WAIT_WRITE;
            return 1700;

        case DATA1|WAIT_WRITE:
            state &= ~WAIT_WRITE;
            A7105_SetTxRxMode(RX_EN);
            A7105_Strobe(A7105_RX);
            return 2150;
    }
    return 3850; // never reached, please the compiler
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (afhds2a_init())
            break;
    }
    initialize_tx_id();
    packet_type = PACKET_STICKS;
    packet_count = 0;
    bind_reply = 0;
    if(bind) {
        state = BIND1;
        PROTOCOL_SetBindState(0xffffffff);
    }
    else {
        state = DATA1;
        rxid[0] = (Model.proto_opts[PROTOOPTS_RXID] >> 8) & 0xff;
        rxid[1] = (Model.proto_opts[PROTOOPTS_RXID]) & 0xff;
        rxid[2] = (Model.proto_opts[PROTOOPTS_RXID2] >> 8)  & 0xff;
        rxid[3] = (Model.proto_opts[PROTOOPTS_RXID2]) & 0xff;
    }
    channel = 0;
    CLOCK_StartTimer(50000, afhds2a_cb);
}

uintptr_t AFHDS2A_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (A7105_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return 14;
        case PROTOCMD_DEFAULT_NUMCHAN: return 8;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS:
            if( Model.proto_opts[PROTOOPTS_SERVO_HZ] < 50 || Model.proto_opts[PROTOOPTS_SERVO_HZ] > 400)
                Model.proto_opts[PROTOOPTS_SERVO_HZ] = 50;
            return (uintptr_t)afhds2a_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE:
            return TELEM_FRSKY;
#if HAS_EXTENDED_TELEMETRY
        case PROTOCMD_TELEMETRYRESET:
            Model.ground_level = 0;
            return 0;
#endif
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
