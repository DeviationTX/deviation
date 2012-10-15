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

#include <stdlib.h>
#include <string.h>

struct Transmitter Transmitter;
static u32 crc32;

const char CURRENT_MODEL[] = "current_model";

const char LANGUAGE[] = "language";

const char MODE[]="mode";

const char BRIGHTNESS[] = "brightness";

const char BATT_ALARM[] = "batt_alarm";
const char BATT_CRITICAL[] = "batt_critical";

const char SECTION_CALIBRATE[] = "calibrate";
const char CALIBRATE_MAX[] = "max";
const char CALIBRATE_MIN[] = "min";
const char CALIBRATE_ZERO[] = "zero";

const char SECTION_TOUCH[] = "touch";
const char TOUCH_XSCALE[] = "xscale";
const char TOUCH_YSCALE[] = "yscale";
const char TOUCH_XOFFSET[] = "xoffset";
const char TOUCH_YOFFSET[] = "yoffset";

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
        if (MATCH_KEY(MODE)) {
            t->mode = atoi(value);
            return 1;
        }
        if (MATCH_KEY(BRIGHTNESS)) {
            t->brightness = atoi(value);
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
    }
    if(MATCH_START(section, SECTION_CALIBRATE) && strlen(section) >= sizeof(SECTION_CALIBRATE)) {
        u8 idx = atoi(section + sizeof(SECTION_CALIBRATE)-1);
        if (idx == 0) {
            printf("%s: Unknown Calibration\n", section);
            return 0;
        }
        if (idx > 4) {
            printf("%s: Only 4 calibrations are supported\n", section);
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
    printf("Unknown values section: %s key: %s\n", section, name);
    return 0;
}

void CONFIG_LoadTx()
{
    memset(&Transmitter, 0, sizeof(Transmitter));
    Transmitter.current_model = 1;
    Transmitter.mode = MODE_2;
    Transmitter.brightness = 9;
    Transmitter.batt_alarm = DEFAULT_BATTERY_ALARM;
    Transmitter.batt_critical = DEFAULT_BATTERY_CRITICAL;
    CONFIG_IniParse("tx.ini", ini_handler, (void *)&Transmitter);
    crc32 = Crc(&Transmitter, sizeof(Transmitter));
    return;
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
    fprintf(fh, "%s=%d\n", MODE, Transmitter.mode);
    fprintf(fh, "%s=%d\n", BRIGHTNESS, Transmitter.brightness);
    fprintf(fh, "%s=%d\n", BATT_ALARM, Transmitter.batt_alarm);
    fprintf(fh, "%s=%d\n", BATT_CRITICAL, Transmitter.batt_critical);
    for(i = 0; i < 4; i++) {
        fprintf(fh, "[%s%d]\n", SECTION_CALIBRATE, i+1);
        fprintf(fh, "  %s=%d\n", CALIBRATE_MAX, t->calibration[i].max);
        fprintf(fh, "  %s=%d\n", CALIBRATE_MIN, t->calibration[i].min);
        fprintf(fh, "  %s=%d\n", CALIBRATE_ZERO, t->calibration[i].zero);
    }
    fprintf(fh, "[%s]\n", SECTION_TOUCH);
    fprintf(fh, "  %s=%d\n", TOUCH_XSCALE, (int)t->touch.xscale);
    fprintf(fh, "  %s=%d\n", TOUCH_YSCALE, (int)t->touch.yscale);
    fprintf(fh, "  %s=%d\n", TOUCH_XOFFSET, (int)t->touch.xoffset);
    fprintf(fh, "  %s=%d\n", TOUCH_YOFFSET, (int)t->touch.yoffset);

    CONFIG_EnableLanguage(1);
    fclose(fh);
}

void CONFIG_SaveTxIfNeeded()
{
    u32 newCrc = Crc(&Transmitter, sizeof(Transmitter));
    if (crc32 == newCrc)
        return;
    CONFIG_WriteTx();
}
