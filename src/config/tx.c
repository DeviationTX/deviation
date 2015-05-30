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
#include "target.h"
#include "gui/gui.h"
#include "tx.h"
#include "rtc.h"

#include <stdlib.h>
#include <string.h>
#include "autodimmer.h"

struct Transmitter Transmitter;
static u32 crc32;

const char CURRENT_MODEL[] = "current_model";

const char LANGUAGE[] = "language";
const char MUSIC_SHUTD[]="music_shutdown";
const char MODE[]="mode";
const char RTC[]="has_rtc";

const char BRIGHTNESS[] = "brightness";
const char CONTRAST[] = "contrast";
const char VOLUME[] = "volume";
const char VIBRATION[] = "vibration";
const char POWER_ALARM[] = "power_alarm";

const char BATT_ALARM[] = "batt_alarm";
const char BATT_CRITICAL[] = "batt_critical";
const char BATT_WARNING_INTERVAL[] = "batt_warning_interval";

const char SPLASH_DELAY[] = "splash_delay";
const char CLOCK_12HR[] = "12hr_clock";
const char TIME_FORMAT[] = "time_format";
const char DATE_FORMAT[] = "date_format";


const char SECTION_CALIBRATE[] = "calibrate";
const char CALIBRATE_MAX[] = "max";
const char CALIBRATE_MIN[] = "min";
const char CALIBRATE_ZERO[] = "zero";

const char SECTION_TOUCH[] = "touch";
const char TOUCH_XSCALE[] = "xscale";
const char TOUCH_YSCALE[] = "yscale";
const char TOUCH_XOFFSET[] = "xoffset";
const char TOUCH_YOFFSET[] = "yoffset";

const char SECTION_AUTODIMMER[] = "autodimmer";
const char AUTODIMMER_TIME[] = "timer";
const char AUTODIMMER_DIMVALUE[] = "dimvalue";

const char SECTION_TIMERSETTINGS[] = "countdowntimersettings";
const char TIMERSETTINGS_PREALERT_TIME[] = "prealerttime";
const char TIMERSETTINGS_PREALERT_INTERVAL[] = "prealertinterval";
const char TIMERSETTINGS_TIMEUP_INTERVAL[] = "timeupinterval";

/* Section: Telemetry */
static const char SECTION_TELEMETRY[] = "telemetry";
static const char TELEM_TEMP[] = "temp";
static const char * const TELEM_TEMP_VAL[2] = { "celcius", "farenheit"};
static const char TELEM_LENGTH[] = "length";
static const char * const TELEM_LENGTH_VAL[2] = { "meters", "feet" };

#define MATCH_SECTION(s) strcasecmp(section, s) == 0
#define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
#define MATCH_KEY(s)     strcasecmp(name,    s) == 0
#define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
#define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct Transmitter *t = (struct Transmitter *)user;

    s32 value_int = atoi(value);
    if (section[0] == '\0') {
        if (MATCH_KEY(CURRENT_MODEL)) {
            t->current_model = value_int;
            return 1;
        }
        if (MATCH_KEY(LANGUAGE)) {
            t->language = value_int;
            return 1;
        }
	if (MATCH_KEY(MUSIC_SHUTD)) {
            t->music_shutdown = atoi(value);
            return 1;
        }
        if (MATCH_KEY(MODE)) {
            t->mode = atoi(value);
            return 1;
        }
        if (MATCH_KEY(BRIGHTNESS)) {
            t->brightness = atoi(value);
            return 1;
        }
        if (MATCH_KEY(CONTRAST)) {
            t->contrast = atoi(value);
            return 1;
        }
        if (MATCH_KEY(VOLUME)) {
            t->volume = atoi(value);
            return 1;
        }
        if (MATCH_KEY(VIBRATION)) {
            t->vibration_state = atoi(value);
            return 1;
        }
        if (MATCH_KEY(POWER_ALARM)) {
            t->power_alarm = atoi(value);
            return 1;
        }
        if (MATCH_KEY(BATT_ALARM)) {
            t->batt_alarm = atoi(value);
            return 1;
        }
        if (MATCH_KEY(BATT_CRITICAL)) {
            t->batt_critical = atoi(value);
            return 1;
        }
	if (MATCH_KEY(BATT_WARNING_INTERVAL)) {
            t->batt_warning_interval = atoi(value);
            return 1;
        }
	if (MATCH_KEY(SPLASH_DELAY)) {
            t->splash_delay = atoi(value);
            return 1;
        }
    #if HAS_RTC
        if (MATCH_KEY(TIME_FORMAT)) {
            t->rtcflags = (t->rtcflags & ~TIMEFMT) | (atoi(value) & TIMEFMT);
            return 1;
        }
        if (MATCH_KEY(DATE_FORMAT)) {
            t->rtcflags = (t->rtcflags & ~DATEFMT) | ((atoi(value) << 4) & DATEFMT);
            return 1;
        }
    #endif
    }
    if(MATCH_START(section, SECTION_CALIBRATE) && strlen(section) >= sizeof(SECTION_CALIBRATE)) {
        u8 idx = atoi(section + sizeof(SECTION_CALIBRATE)-1);
        if (idx == 0) {
            printf("%s: Unknown Calibration\n", section);
            return 0;
        }
        if (idx > INP_HAS_CALIBRATION) {
            printf("%s: Only %d calibrations are supported\n", section, INP_HAS_CALIBRATION);
            return 1;
        }
        idx--;
        if (MATCH_KEY(CALIBRATE_MAX)) {
            t->calibration[idx].max = value_int;
            return 1;
        }
        if (MATCH_KEY(CALIBRATE_MIN)) {
            t->calibration[idx].min = value_int;
            return 1;
        }
        if (MATCH_KEY(CALIBRATE_ZERO)) {
            t->calibration[idx].zero = value_int;
            return 1;
        }
    }
    if (HAS_TOUCH) {
        if (MATCH_SECTION(SECTION_TOUCH)) {
            if (MATCH_KEY(TOUCH_XSCALE)) {
                t->touch.xscale = value_int;
                return 1;
            }
            if (MATCH_KEY(TOUCH_YSCALE)) {
                t->touch.yscale = value_int;
                return 1;
            }
            if (MATCH_KEY(TOUCH_XOFFSET)) {
                t->touch.xoffset = value_int;
                return 1;
            }
            if (MATCH_KEY(TOUCH_YOFFSET)) {
                t->touch.yoffset = value_int;
                return 1;
            }
        }
    }
    if (MATCH_SECTION(SECTION_AUTODIMMER)) {
        if (MATCH_KEY(AUTODIMMER_TIME)) {
            t->auto_dimmer.timer = value_int;
            return 1;
        }
        if (MATCH_KEY(AUTODIMMER_DIMVALUE)) {
            t->auto_dimmer.backlight_dim_value = value_int;
            return 1;
        }
    }
    if (MATCH_SECTION(SECTION_TIMERSETTINGS)) {
        if (MATCH_KEY(TIMERSETTINGS_PREALERT_TIME)) {
            t->countdown_timer_settings.prealert_time = value_int;
            return 1;
        }
        if (MATCH_KEY(TIMERSETTINGS_PREALERT_INTERVAL)) {
            t->countdown_timer_settings.prealert_interval = value_int;
            return 1;
        }
        if (MATCH_KEY(TIMERSETTINGS_TIMEUP_INTERVAL)) {
            t->countdown_timer_settings.timeup_interval = value_int;
            return 1;
        }
    }
    if (MATCH_SECTION(SECTION_TELEMETRY)) {
        if (MATCH_KEY(TELEM_TEMP)) {
            if(strcasecmp(TELEM_TEMP_VAL[1], value) == 0)
                t->telem |= TELEMUNIT_FAREN;
            return 1;
        }
        if (MATCH_KEY(TELEM_LENGTH)) {
            if(strcasecmp(TELEM_LENGTH_VAL[1], value) == 0)
                t->telem |= TELEMUNIT_FEET;
            return 1;
        }
    }
    printf("Unknown values section: %s key: %s\n", section, name);
    return 0;
}

void CONFIG_WriteTx()
{
    int i;
    FILE *fh;
    struct Transmitter *t = &Transmitter;
    fh = fopen("tx.ini", "w");
    if (! fh) {
        printf("Couldn't open tx.ini\n");
        return;
    }
    CONFIG_EnableLanguage(0);
    fprintf(fh, "%s=%d\n", CURRENT_MODEL, Transmitter.current_model);
    fprintf(fh, "%s=%d\n", LANGUAGE, Transmitter.language);
    fprintf(fh, "%s=%d\n", MUSIC_SHUTD, Transmitter.music_shutdown);
    fprintf(fh, "%s=%d\n", MODE, Transmitter.mode);
    fprintf(fh, "%s=%d\n", BRIGHTNESS, Transmitter.brightness);
    fprintf(fh, "%s=%d\n", CONTRAST, Transmitter.contrast);
    fprintf(fh, "%s=%d\n", VOLUME, Transmitter.volume);
    fprintf(fh, "%s=%d\n", VIBRATION, Transmitter.vibration_state);
    fprintf(fh, "%s=%d\n", POWER_ALARM, Transmitter.power_alarm);
    fprintf(fh, "%s=%d\n", BATT_ALARM, Transmitter.batt_alarm);
    fprintf(fh, "%s=%d\n", BATT_CRITICAL, Transmitter.batt_critical);
    fprintf(fh, "%s=%d\n", BATT_WARNING_INTERVAL, Transmitter.batt_warning_interval);
    fprintf(fh, "%s=%d\n", SPLASH_DELAY, Transmitter.splash_delay);
#if HAS_RTC
    fprintf(fh, "%s=%d\n", TIME_FORMAT, Transmitter.rtcflags & TIMEFMT);
    fprintf(fh, "%s=%d\n", DATE_FORMAT, (Transmitter.rtcflags & DATEFMT) >> 4);
#endif
    for(i = 0; i < INP_HAS_CALIBRATION; i++) {
        fprintf(fh, "[%s%d]\n", SECTION_CALIBRATE, i+1);
        fprintf(fh, "  %s=%d\n", CALIBRATE_MAX, t->calibration[i].max);
        fprintf(fh, "  %s=%d\n", CALIBRATE_MIN, t->calibration[i].min);
        fprintf(fh, "  %s=%d\n", CALIBRATE_ZERO, t->calibration[i].zero);
    }
    if (HAS_TOUCH) {
        fprintf(fh, "[%s]\n", SECTION_TOUCH);
        fprintf(fh, "  %s=%d\n", TOUCH_XSCALE, (int)t->touch.xscale);
        fprintf(fh, "  %s=%d\n", TOUCH_YSCALE, (int)t->touch.yscale);
        fprintf(fh, "  %s=%d\n", TOUCH_XOFFSET, (int)t->touch.xoffset);
        fprintf(fh, "  %s=%d\n", TOUCH_YOFFSET, (int)t->touch.yoffset);
    }
    fprintf(fh, "[%s]\n", SECTION_AUTODIMMER);
    fprintf(fh, "%s=%u\n", AUTODIMMER_TIME, (unsigned int)t->auto_dimmer.timer);
    fprintf(fh, "%s=%u\n", AUTODIMMER_DIMVALUE, t->auto_dimmer.backlight_dim_value);
    fprintf(fh, "[%s]\n", SECTION_TIMERSETTINGS);
    fprintf(fh, "%s=%u\n", TIMERSETTINGS_PREALERT_TIME, (unsigned int)t->countdown_timer_settings.prealert_time);
    fprintf(fh, "%s=%u\n", TIMERSETTINGS_PREALERT_INTERVAL, t->countdown_timer_settings.prealert_interval);
    fprintf(fh, "%s=%u\n", TIMERSETTINGS_TIMEUP_INTERVAL, t->countdown_timer_settings.timeup_interval);

    fprintf(fh, "[%s]\n", SECTION_TELEMETRY);
    fprintf(fh, "%s=%s\n", TELEM_TEMP, TELEM_TEMP_VAL[(t->telem & TELEMUNIT_FAREN) ? 1 : 0]);
    fprintf(fh, "%s=%s\n", TELEM_LENGTH, TELEM_LENGTH_VAL[(t->telem & TELEMUNIT_FEET) ? 1 : 0]);

    CONFIG_EnableLanguage(1);
    fclose(fh);
}

void CONFIG_LoadTx()
{
    memset(&Transmitter, 0, sizeof(Transmitter));
    Transmitter.current_model = 1;
    Transmitter.music_shutdown = 0; // default to off
    Transmitter.mode = MODE_2;
    Transmitter.brightness = 5;
    Transmitter.contrast = 5;
    Transmitter.volume = 10;
    Transmitter.vibration_state = 0; // default to off since only devo10 support it
    Transmitter.power_alarm = 0;     // default to off 
    Transmitter.batt_alarm = DEFAULT_BATTERY_ALARM;
    Transmitter.batt_critical = DEFAULT_BATTERY_CRITICAL;
    Transmitter.batt_warning_interval = DEFAULT_BATTERY_WARNING_INTERVAL;
    Transmitter.splash_delay = DEFAULT_SPLASH_DELAY;
    Transmitter.auto_dimmer.timer = DEFAULT_BACKLIGHT_DIMTIME;
    Transmitter.auto_dimmer.backlight_dim_value = DEFAULT_BACKLIGHT_DIMVALUE;
    Transmitter.countdown_timer_settings.prealert_time = DEFAULT_PERALERT_TIME;
    Transmitter.countdown_timer_settings.prealert_interval = DEFAULT_PREALERT_INTERVAL;
    Transmitter.countdown_timer_settings.timeup_interval = DEFAULT_TIMEUP_INTERVAL;
#if HAS_EXTRA_SWITCHES
    CHAN_SetSwitchCfg("");
#endif
    MCU_InitModules();
    CONFIG_LoadHardware();
    CONFIG_IniParse("tx.ini", ini_handler, (void *)&Transmitter);
    crc32 = Crc(&Transmitter, sizeof(Transmitter));
    return;
}

void CONFIG_SaveTxIfNeeded()
{
    u32 newCrc = Crc(&Transmitter, sizeof(Transmitter));
    if (crc32 == newCrc)
        return;
    crc32 = newCrc;
    CONFIG_WriteTx();
}
