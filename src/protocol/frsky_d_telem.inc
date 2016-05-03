/***************************************************************
 telemetry processing that's common for Frsky and FrskyX
***************************************************************/

static s32 ground_level;
static u32 discharge_time;
static u32 discharge_dAms;   // deciAmp-milliseconds


static void set_telemetry(u8 offset, s32 value) {
    Telemetry.value[offset] = value;
    TELEMETRY_SetUpdated(offset);
}


// store "before punctuation" and "after punctuation" values locally
// keeps Telemetry.gps compatible with devo and dsm
static struct s_fr_gps {
    s16 latitude_bp;
    s16 latitude_ap;
    s16 longitude_bp;
    s16 longitude_ap;
    s16 altitude_bp;
    s16 altitude_ap;
    s16 velocity_bp;
    s16 velocity_ap;
    u16 day_month;             // 0x15
    u16 year;                  // 0x16
    u16 hour_min;              // 0x17
    u16 second;                // 0x18
    u16 amp_voltage_bp;
    u16 amp_voltage_ap;
//    u16 heading;
//    u8 satcount;
} fr_gps;


static void processHubPacket(u8 id, u16 value) {
    switch (id) {
    //defined in protocol_sensor_hub.pdf
    case 0x01: //GPS_ALT (whole number & sign) -500m-9000m (.01m/count)
        fr_gps.altitude_bp = value;
        break;
    case 0x09: //GPS_ALT (fraction)
        fr_gps.altitude_ap = value;
        Telemetry.gps.altitude = (fr_gps.altitude_bp * 100 + fr_gps.altitude_ap) * 10;
        TELEMETRY_SetUpdated(TELEM_GPS_ALT);
        break;
    case 0x02: //TEMP1 -30C-250C (1C/ count)
        Telemetry.value[TELEM_FRSKY_TEMP1] = value;
        TELEMETRY_SetUpdated(TELEM_FRSKY_TEMP1);
        break;
    case 0x03: //RPM   0-60000
        Telemetry.value[TELEM_FRSKY_RPM] = value * 60;
        TELEMETRY_SetUpdated(TELEM_FRSKY_RPM);
        break;
    case 0x04: //Fuel  0, 25, 50, 75, 100
        Telemetry.value[TELEM_FRSKY_FUEL] = value;
        TELEMETRY_SetUpdated(TELEM_FRSKY_FUEL);
        break;
    case 0x05: //TEMP2 -30C-250C (1C/ count)
        Telemetry.value[TELEM_FRSKY_TEMP2] = value;
        TELEMETRY_SetUpdated(TELEM_FRSKY_TEMP2);
        break;
    case 0x06: { //Battery voltages - CELL# and VOLT
        u8 cell = (value >> 4) & 0x0f;
        value = (((u16)(value & 0x0F) << 8) + (value & 0xff00)) / 5;
        if (cell < 6 || value == 0) {
            Telemetry.value[TELEM_FRSKY_ALL_CELL] +=
              value - Telemetry.value[TELEM_FRSKY_CELL1 + cell];
            TELEMETRY_SetUpdated(TELEM_FRSKY_ALL_CELL);

            Telemetry.value[TELEM_FRSKY_CELL1 + cell] = value;
            TELEMETRY_SetUpdated(TELEM_FRSKY_CELL1 + cell);
            if (Telemetry.value[TELEM_FRSKY_MIN_CELL] == 0 ||
                (value < Telemetry.value[TELEM_FRSKY_MIN_CELL])) {
                Telemetry.value[TELEM_FRSKY_MIN_CELL] = value;
                TELEMETRY_SetUpdated(TELEM_FRSKY_MIN_CELL);
            }
        }
        break;
        }
    case 0x10: //ALT (whole number & sign) -500m-9000m (.01m/count)
        if (ground_level == 0) ground_level = value;
        set_telemetry(TELEM_FRSKY_ALTITUDE, value - ground_level);
        break;
    case 0x21: //ALT (fraction)
            //convert to mm
        set_telemetry(TELEM_FRSKY_ALTITUDE_DECIMETERS, value);
        break;
    case 0x11: //GPS Speed (whole number and sign) in Knots
        fr_gps.velocity_bp = value;
        break;
    case 0x19: //GPS Speed (fraction)
        fr_gps.velocity_ap = value;
        // Convert 1/100 knot to mm/sec
        Telemetry.gps.velocity = (fr_gps.velocity_bp * 100 + fr_gps.velocity_ap) * 5556 / 1080;
        TELEMETRY_SetUpdated(TELEM_GPS_SPEED);
        break;
    case 0x12: //GPS Longitude (whole number) dddmm.mmmm
        fr_gps.longitude_bp = value;
        break;
    case 0x1A: //GPS Longitude (fraction)
        fr_gps.longitude_ap = value;
        break;
    case 0x22: { //GPS Longitude E/W
        s32 deg = fr_gps.longitude_bp / 100;
        s32 min = fr_gps.longitude_bp % 100;
        Telemetry.gps.longitude = (deg * 60 + min) * 60 * 1000 + fr_gps.longitude_ap * 6;
        if (value == 'W')
            Telemetry.gps.longitude = -Telemetry.gps.longitude;
        TELEMETRY_SetUpdated(TELEM_GPS_LONG);
        break;}
    case 0x13: //GPS Latitude (whole number) ddmm.mmmm
        fr_gps.latitude_bp = value;
        break;
    case 0x1B: //GPS Latitude (fraction)
        fr_gps.latitude_ap = value;
        break;
    case 0x23: { //GPS Latitude N/S
        s32 deg = fr_gps.latitude_bp / 100;
        s32 min = fr_gps.latitude_bp % 100;
        Telemetry.gps.latitude = (deg * 60 + min) * 60 * 1000 + fr_gps.latitude_ap * 6;
        if (value == 'S')
            Telemetry.gps.latitude = -Telemetry.gps.latitude;
        TELEMETRY_SetUpdated(TELEM_GPS_LAT);
        break;}
    //case 0x14: //GPS Compass (whole number) (0-259.99) (.01degree/count)
    //case 0x1C: //GPS Compass (fraction)
    case 0x15: //GPS Date/Month
        fr_gps.day_month = value;
        break;
    case 0x16: //GPS Year
        fr_gps.year = value;
        break;
    case 0x17: //GPS Hour/Minute
        fr_gps.hour_min = value;
        break;
    case 0x18: //GPS Second
        fr_gps.second = value;
        Telemetry.gps.time = ( (u32)fr_gps.year & 0x3f)            << 26
                           | (((u32)fr_gps.day_month >> 8) & 0x0f) << 22
                           | ( (u32)fr_gps.day_month & 0x1f)       << 17
                           | ( (u32)fr_gps.hour_min & 0x1f)        << 12
                           | (((u32)fr_gps.hour_min >> 8) & 0x3f)  << 6
                           | ( (u32)fr_gps.second & 0x3f);
        TELEMETRY_SetUpdated(TELEM_GPS_TIME);
        break;
    //case 0x24: //Accel X
    //case 0x25: //Accel Y
    //case 0x26: //Accel Z
    case 0x28: //Current 0A-100A (0.1A/count)
        set_telemetry(TELEM_FRSKY_CURRENT, value);
        if (discharge_time == 0) discharge_time = CLOCK_getms();
        discharge_dAms += (u32)value * (CLOCK_getms() - discharge_time);
        discharge_time = CLOCK_getms();
        set_telemetry(TELEM_FRSKY_DISCHARGE, discharge_dAms / 36000);
        break;
    case 0x3A: //Ampere sensor (whole number) (measured as V) 0V-48V (0.5V/count)
        fr_gps.amp_voltage_bp = value;
        break;
    case 0x3B: //Ampere sensor (fractional)
        fr_gps.amp_voltage_ap = value;
        Telemetry.value[TELEM_FRSKY_VOLTA] = fr_gps.amp_voltage_bp * 50 + fr_gps.amp_voltage_ap / 2;
        TELEMETRY_SetUpdated(TELEM_FRSKY_VOLTA);
        break;
    }
}
