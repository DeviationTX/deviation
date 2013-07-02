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
#include "target_defs.h"
#include "config/model.h"

#include <stdio.h>

#if DATALOG_ENABLED
#define DATALOG_VERSION 0x01

#define UPDATE_DELAY 4000 //wiat 4 seconds after changing enable before sample start
#define DATALOG_HEADER_SIZE (3 + ((7 + NUM_DATALOG) / 8))
const u32 sample_rate[DLOG_RATE_LAST] = {
    [DLOG_RATE_1SEC]  =  1000,
    [DLOG_RATE_5SEC]  =  5000,
    [DLOG_RATE_10SEC] = 10000,
    [DLOG_RATE_30SEC] = 30000,
    [DLOG_RATE_1MIN]  = 60000,
};

static struct FAT DatalogFAT;
static FILE *fh;
static u32 next_update;
static u32 dlog_pos;
static u32 dlog_size;
u8 need_header_update;
u16 data_size;

const char *DATALOG_RateString(int idx)
{
    switch(idx) {
        case 0: return _tr("1 sec");
        case 1: return _tr("5 sec");
        case 2: return _tr("10 sec");
        case 3: return _tr("30 sec");
        case 4: return _tr("60 sec");
    }
    return "";
}

const char *DATALOG_Source(char *str, int idx)
{
#if HAS_RTC
    if (idx == DLOG_TIME) {
        strcpy(str, _tr("RTC Time"));
    } else
#endif
    if (idx == DLOG_GPSTIME) {
        strcpy(str, _tr("GPS Time"));
    } else if (idx == DLOG_GPSLOC) {
        strcpy(str, _tr("GPS Coords"));
    } else if (idx == DLOG_GPSALT) {
        strcpy(str, _tr("GPS Alt."));
    } else if (idx == DLOG_GPSSPEED) {
        strcpy(str, _tr("GPS Speed"));
    } else if (idx >= DLOG_INPUTS) {
        return INPUT_SourceName(str, idx - DLOG_INPUTS + 1);
    } else if (idx >= DLOG_TELEMETRY) {
        return TELEMETRY_Name(str, idx - DLOG_TELEMETRY + 1);
    } else { // idx >= DLOG_TIMERS 
        return TIMER_Name(str, idx);
    }
    return str;
}

void DATALOG_ApplyMask(int idx, int set) {
    if (set) {
        Model.datalog.source[DATALOG_BYTE(idx)] |= 1 << DATALOG_POS(idx);
    } else {
        Model.datalog.source[DATALOG_BYTE(idx)] &= ~(1L << DATALOG_POS(idx));
    }
}

int DATALOG_GetSize(u8 *src)
{
    int size = 0;
    for(int i = 0; i < NUM_DATALOG; i++) {
        if (! src || (src[DATALOG_BYTE(i)] & 1 << DATALOG_POS(i))) {
#if HAS_RTC
            if (i == DLOG_TIME) {
                size += CLOCK_SIZE;
            } else
#endif
            if (i >= DLOG_GPSALT) {
                size += GPSTIME_SIZE;
            } else if (i == DLOG_GPSLOC) {
                size += GPSLOC_SIZE;
            } else if (i < DLOG_INPUTS) {
                size += TIMER_SIZE;
            } else {
                size += 1;
            }
        }
    }
    return size;
}

s16 _get_src_value(int idx, u32 opts)
{
    s16 val;
    if (idx <= NUM_INPUTS || idx > NUM_INPUTS + NUM_CHANNELS /*PPM*/) {
        volatile s16 *raw = MIXER_GetInputs();
        val = raw[idx];
    } else {
        val = MIXER_GetChannel(idx - NUM_INPUTS - 1, opts);
    }
    return val;
}
long _find_fpos() {
    dlog_pos = 0;
    int size = 1;
    u8 data[DATALOG_HEADER_SIZE];
    while(1) {
        fread((char *)data, DATALOG_HEADER_SIZE, 1, fh);
        if (data[0] == 0x00) {
            fseek(fh, dlog_pos, SEEK_SET);
            return dlog_pos;
        }
        if (data[0] == 0xff) {
            dlog_pos += size;
            fseek(fh, dlog_pos, SEEK_SET);
            continue;
        }
        dlog_pos += DATALOG_HEADER_SIZE;
        size = DATALOG_GetSize(data+2) + 1;
    }
}

void _write_8(s32 data)
{
    fwrite(&data, 1, 1, fh);
    dlog_pos++;
}
void _write_16(s32 data)
{
    u8 x[2] = {(data & 0xff),
               (data >> 8) & 0xff,
              };
    fwrite(x, 2, 1, fh);
    dlog_pos+=2;
}
void _write_32(s32 data)
{
    u8 x[4] = {(data & 0xff),
               (data >> 8) & 0xff,
               (data >> 16) & 0xff,
               (data >> 24) & 0xff,
              };
    fwrite(x, 4, 1, fh);
    dlog_pos+=4;
}

void _write_header() {
    need_header_update = 0;
    _write_8(0x01);
    _write_8(TXID);
    _write_8(Model.datalog.rate);
    fwrite(Model.datalog.source, sizeof(Model.datalog.source), 1, fh);
    dlog_pos += 3 + sizeof(Model.datalog.source);
}

int DATALOG_IsEnabled()
{
    if(! Model.datalog.enable)
        return 0;
    u8 src = Model.datalog.enable;
    s16 val = _get_src_value(MIXER_SRC(src), APPLY_SAFETY);
    if (MIXER_SRC_IS_INV(src))
        val = -val;
    return (val - CHAN_MIN_VALUE > (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 20) ? 1 : 0;
}

void DATALOG_Write()
{
    _write_8(0xff);
    for (int i = 0; i < DLOG_LAST; i++) {
        if(! (Model.datalog.source[DATALOG_BYTE(i)] & (1 << DATALOG_POS(i))))
            continue;
#if HAS_RTC
        if(i == DLOG_TIME) {
            _write_32(RTC_GetValue());
        } else
#endif
        if(i == DLOG_GPSTIME) {
            _write_32(Telemetry.gps.time);
        } else if(i == DLOG_GPSSPEED) {
            _write_32(Telemetry.gps.velocity);
        } else if(i == DLOG_GPSALT) {
            _write_32(Telemetry.gps.altitude);
        } else if(i == DLOG_GPSLOC) {
            _write_32(Telemetry.gps.latitude);
            _write_32(Telemetry.gps.longitude);
        } else if(i >= DLOG_INPUTS) {
            int val = _get_src_value(i - DLOG_INPUTS + 1, APPLY_SAFETY | APPLY_SCALAR);
            val = RANGE_TO_PCT(val);
            if (val > 127)
                val = 127;
            if(val < -128)
                val = -128;
            _write_8(val);
        } else if(i >= DLOG_TELEMETRY) {
            _write_16(TELEMETRY_GetValue(i - DLOG_TELEMETRY + 1));
        } else {
            _write_16(TIMER_GetValue(i) / 1000);
        }
    }
}

void DATALOG_Update()
{
    if (! fh)
        return;
    if(DATALOG_IsEnabled() && ((int)dlog_size - ftell(fh) >= data_size)) {
        u32 time = CLOCK_getms();
        if(time >= next_update) {
            if (need_header_update)
                _write_header();
            next_update = time + sample_rate[Model.datalog.rate];
            DATALOG_Write();
        }
    }
}

void DATALOG_UpdateState()
{
    next_update = CLOCK_getms() + UPDATE_DELAY;
    data_size = DATALOG_GetSize(Model.datalog.source);
    need_header_update = 1;
}

void DATALOG_Reset()
{
    if (fh) {
        fempty(fh);
        dlog_pos = 0;
        DATALOG_UpdateState();
    }
}

int DATALOG_Remaining()
{
    if(fh)
       return dlog_size - ftell(fh);
    return 0;
}

void DATALOG_Init()
{
    next_update = 0;
    need_header_update = 1;
    finit(&DatalogFAT, "");
    fh = fopen2(&DatalogFAT, "datalog.bin", "r+");
    if (fh) {
        setbuf(fh, 0);
        long pos = _find_fpos();
        fseek(fh, 0, SEEK_END);
        dlog_size = ftell(fh);
        fseek(fh, pos, SEEK_SET);
        data_size = DATALOG_GetSize(Model.datalog.source);
        printf("num data: %d data size: %d\n", DLOG_LAST, DATALOG_GetSize(NULL));
        next_update = CLOCK_getms();
    }
} 
#endif
