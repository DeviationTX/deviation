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
#include "config.h"
#include "gui/gui.h"
#include "tx.h"
#include "rtc.h"
#include "voice.h"

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
#if HAS_EXTENDED_AUDIO
const char AUDIO_VOL[] = "audio_vol";
#endif
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
static const char TELEM_ALERT_INTERVAL[] ="alertinterval";

#define MATCH_SECTION(s) strcasecmp(section, s) == 0
#define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
#define MATCH_KEY(s)     strcasecmp(name,    s) == 0
#define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
#define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))

static const struct struct_map _secmodel[] =
{
    {CURRENT_MODEL,          OFFSET(struct Transmitter, current_model)},
    {LANGUAGE,               OFFSET(struct Transmitter, language)},
    {MUSIC_SHUTD,            OFFSET(struct Transmitter, music_shutdown)},
    {MODE,                   OFFSET(struct Transmitter, mode)},
    {BRIGHTNESS,             OFFSET(struct Transmitter, backlight)},
    {CONTRAST,               OFFSET(struct Transmitter, contrast)},
    {VOLUME,                 OFFSET(struct Transmitter, volume)},
    {VIBRATION,              OFFSET(struct Transmitter, vibration_state)},
    {POWER_ALARM,            OFFSET(struct Transmitter, power_alarm)},
    {BATT_ALARM,             OFFSET(struct Transmitter, batt_alarm)},
    {BATT_CRITICAL,          OFFSET(struct Transmitter, batt_critical)},
    {BATT_WARNING_INTERVAL,  OFFSET(struct Transmitter, batt_warning_interval)},
    {SPLASH_DELAY,           OFFSET(struct Transmitter, splash_delay)},

#if HAS_EXTENDED_AUDIO
    {AUDIO_VOL,              OFFSET(struct Transmitter, audio_vol)},
#endif
};

static const struct struct_map _seccalibrate[] =
{
    {CALIBRATE_MAX,          OFFSET(struct StickCalibration, max)},
    {CALIBRATE_MIN,          OFFSET(struct StickCalibration, min)},
    {CALIBRATE_ZERO,         OFFSET(struct StickCalibration, zero)},
};

#if HAS_TOUCH
static const struct struct_map _sectouch[] =
{
    {TOUCH_XSCALE,           OFFSET(struct TouchCalibration, xscale)},
    {TOUCH_YSCALE,           OFFSET(struct TouchCalibration, yscale)},
    {TOUCH_XOFFSET,          OFFSET(struct TouchCalibration, xoffset)},
    {TOUCH_YOFFSET,          OFFSET(struct TouchCalibration, yoffset)},
};
#endif

static const struct struct_map _secautodimmer[] =
{
    {AUTODIMMER_TIME,        OFFSET(struct AutoDimmer, timer)},
    {AUTODIMMER_DIMVALUE,    OFFSET(struct AutoDimmer, backlight_dim_value)},
};

static const struct struct_map _sectimer[] =
{
    {TIMERSETTINGS_PREALERT_TIME, OFFSET(struct CountDownTimerSettings, prealert_time)},
    {TIMERSETTINGS_PREALERT_INTERVAL, OFFSET(struct CountDownTimerSettings, prealert_interval)},
    {TIMERSETTINGS_TIMEUP_INTERVAL, OFFSET(struct CountDownTimerSettings, timeup_interval)},
};

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct Transmitter *t = (struct Transmitter *)user;

    s32 value_int = atoi(value);
    if (section[0] == '\0') {
        if (assign_int(t, _secmodel, ARRAYSIZE(_secmodel), name, value))
            return 1;
    #if HAS_RTC
        // TODO: no need to pack into 1 byte
        if (MATCH_KEY(TIME_FORMAT)) {
            t->rtc_timeformat = atoi(value);
            return 1;
        }
        if (MATCH_KEY(DATE_FORMAT)) {
            t->rtc_dateformat = atoi(value);
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
        if (assign_int(&t->calibration[idx], _seccalibrate, ARRAYSIZE(_seccalibrate), name, value))
            return 1;
    }
    if (HAS_TOUCH && MATCH_SECTION(SECTION_TOUCH)) {
        if (assign_int(&t->touch, _sectouch, ARRAYSIZE(_sectouch), name, value))
            return 1;
    }
    if (MATCH_SECTION(SECTION_AUTODIMMER)) {
        if (assign_int(&t->auto_dimmer, _secautodimmer, ARRAYSIZE(_secautodimmer), name, value))
            return 1;
    }
    if (MATCH_SECTION(SECTION_TIMERSETTINGS)) {
        if (assign_int(&t->countdown_timer_settings, _sectimer, ARRAYSIZE(_sectimer), name, value))
            return 1;
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
        if (MATCH_KEY(TELEM_ALERT_INTERVAL)) {
            t->telem_alert_interval = value_int;
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
    write_int(&Transmitter, _secmodel, ARRAYSIZE(_secmodel), fh);
#if HAS_RTC
    fprintf(fh, "%s=%d\n", TIME_FORMAT, Transmitter.rtc_timeformat);
    fprintf(fh, "%s=%d\n", DATE_FORMAT, Transmitter.rtc_dateformat);
#endif

    for(i = 0; i < INP_HAS_CALIBRATION; i++) {
        fprintf(fh, "[%s%d]\n", SECTION_CALIBRATE, i+1);
        write_int(&t->calibration[i], _seccalibrate, ARRAYSIZE(_seccalibrate), fh);
    }

    if (HAS_TOUCH) {
        fprintf(fh, "[%s]\n", SECTION_TOUCH);
        write_int(&t->touch, _sectouch, ARRAYSIZE(_sectouch), fh);
    }
    fprintf(fh, "[%s]\n", SECTION_AUTODIMMER);
    write_int(&t->auto_dimmer, _secautodimmer, ARRAYSIZE(_secautodimmer), fh);

    fprintf(fh, "[%s]\n", SECTION_TIMERSETTINGS);
    write_int(&t->countdown_timer_settings, _sectimer, ARRAYSIZE(_sectimer), fh);

    fprintf(fh, "[%s]\n", SECTION_TELEMETRY);
    fprintf(fh, "%s=%s\n", TELEM_TEMP, TELEM_TEMP_VAL[(t->telem & TELEMUNIT_FAREN) ? 1 : 0]);
    fprintf(fh, "%s=%s\n", TELEM_LENGTH, TELEM_LENGTH_VAL[(t->telem & TELEMUNIT_FEET) ? 1 : 0]);
    fprintf(fh, "%s=%u\n", TELEM_ALERT_INTERVAL, t->telem_alert_interval);

    CONFIG_EnableLanguage(1);
    fclose(fh);
}

void CONFIG_LoadTx()
{
    memset(&Transmitter, 0, sizeof(Transmitter));
    Transmitter.current_model = 1;
    Transmitter.music_shutdown = 0; // default to off
    Transmitter.mode = MODE_2;
    Transmitter.backlight = 5;
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
    Transmitter.countdown_timer_settings.prealert_time = DEFAULT_PREALERT_TIME;
    Transmitter.countdown_timer_settings.prealert_interval = DEFAULT_PREALERT_INTERVAL;
    Transmitter.countdown_timer_settings.timeup_interval = DEFAULT_TIMEUP_INTERVAL;
    Transmitter.telem_alert_interval = DEFAULT_TELEM_INTERVAL;
#if HAS_EXTRA_SWITCHES
    CHAN_SetSwitchCfg("");
#endif
#if HAS_EXTENDED_AUDIO
    Transmitter.audio_player = AUDIO_NONE;
    Transmitter.audio_vol = 10;
#endif
#if HAS_AUDIO_UART
    Transmitter.audio_uart = 0;
#endif
#if HAS_EXTRA_BUTTONS
    CHAN_SetButtonCfg("");
#endif
    MCU_InitModules();
    CONFIG_LoadHardware();
    CONFIG_IniParse("tx.ini", ini_handler, (void *)&Transmitter);
    crc32 = Crc(&Transmitter, sizeof(Transmitter));
#if HAS_EXTENDED_AUDIO
    CONFIG_VoiceParse(MAX_VOICEMAP_ENTRIES);
#endif
    return;
}

void CONFIG_SaveTxIfNeeded()
{
    u32 newCrc = Crc(&Transmitter, sizeof(Transmitter));
    if (crc32 == newCrc)
        return;
    crc32 = newCrc;
    //printf("Saving TX\n");
    CONFIG_WriteTx();
}
