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

#include "target.h"
#include "misc.h"
#include "ini.h"
#include "gui/gui.h"
#include "tx.h"

#include <stdlib.h>
#include <string.h>

struct Transmitter Transmitter;
static u32 crc32;

const char CURRENT_MODEL[] = "current_model";

const char SECTION_TRIM[] = "trim";
const char TRIM_VALUE[] = "value";

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
    }
    if(MATCH_SECTION(SECTION_TRIM)) {
        if(MATCH_START(name, TRIM_VALUE) && strlen(name) >= sizeof(TRIM_VALUE)) {
            u8 idx = atoi(name + sizeof(TRIM_VALUE)-1);
            if(idx == 0) {
                printf("%s: Unkown Trim %s\n", section, name);
                return 0;
            }
            if(idx > NUM_TRIMS) {
                printf("%s: Only %d trims are supported\n", section, NUM_TRIMS);
                return 0;
            }
            t->Trims[idx-1] = value_int;
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
    ini_parse("tx.ini", ini_handler, (void *)&Transmitter);
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
    fprintf(fh, "%s=%d\n", CURRENT_MODEL, Transmitter.current_model);
    fprintf(fh, "[%s]\n", SECTION_TRIM);
    for(i = 0; i < NUM_TRIMS; i++) {
        fprintf(fh, "  %s%d=%d\n", TRIM_VALUE, i+1, t->Trims[i]);
    }
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

    fclose(fh);
}

void CONFIG_SaveTxIfNeeded()
{
    u32 newCrc = Crc(&Transmitter, sizeof(Transmitter));
    if (crc32 == newCrc)
        return;
    CONFIG_WriteTx();
}
