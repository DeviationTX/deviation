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
#include "model.h"
#include "telemetry.h"
#include "tx.h"
#include <stdlib.h>
#include <string.h>

struct Model Model;
/*set this to write all model data even if it is the same as the default */
#define WRITE_FULL_MODEL 0
static u32 crc32;
static u8 auto_map;

const char *MODEL_NAME = "name";
const char *MODEL_ICON = "icon";
const char *MODEL_TYPE = "type";
const char *MODEL_TEMPLATE = "template";
const char *MODEL_AUTOMAP = "automap";
const char * const MODEL_TYPE_VAL[MODELTYPE_LAST] = { "heli", "plane" };

/* Section: Radio */
static const char SECTION_RADIO[]   = "radio";

static const char RADIO_PROTOCOL[] = "protocol";
#define RADIO_PROTOCOL_VAL ProtocolNames

static const char RADIO_NUM_CHANNELS[] = "num_channels";
static const char RADIO_FIXED_ID[] = "fixed_id";

static const char RADIO_TX_POWER[] = "tx_power";
const char * const RADIO_TX_POWER_VAL[TXPOWER_LAST] =
     { "100uW", "300uW", "1mW", "3mW", "10mW", "30mW", "100mW", "150mW" };

/* Section: Mixer */
static const char SECTION_MIXER[]   = "mixer";

static const char MIXER_SOURCE[] = "src";
static const char MIXER_DEST[] = "dest";
static const char MIXER_SWITCH[] = "switch";
static const char MIXER_SCALAR[] = "scalar";
static const char MIXER_OFFSET[] = "offset";
static const char MIXER_USETRIM[] = "usetrim";

static const char MIXER_MUXTYPE[] = "muxtype";
static const char * const MIXER_MUXTYPE_VAL[MUX_LAST]  = { "replace", "multiply", "add", "max", "min" };

static const char MIXER_CURVETYPE[] = "curvetype";
static const char * const MIXER_CURVETYPE_VAL[CURVE_MAX+1] = {
   "none", "fixed", "min/max", "zero/max", "greater-than-0", "less-than-0", "absval",
   "expo", "deadband", "3point", "5point", "7point", "9point", "11point", "13point" };
static const char MIXER_CURVE_POINTS[] = "points";

/* Section: Channel */
static const char SECTION_CHANNEL[] = "channel";

static const char CHAN_LIMIT_REVERSE[] = "reverse";
static const char CHAN_LIMIT_SAFETYSW[] = "safetysw";
static const char CHAN_LIMIT_SAFETYVAL[] = "safetyval";
static const char CHAN_LIMIT_FAILSAFE[] = "failsafe";
static const char CHAN_LIMIT_MAX[] = "max";
static const char CHAN_LIMIT_MIN[] = "min";
static const char CHAN_SUBTRIM[] = "subtrim";
#define CHAN_SCALAR   MIXER_SCALAR
#define CHAN_TEMPLATE MODEL_TEMPLATE
static const char * const CHAN_TEMPLATE_VAL[MIXERTEMPLATE_MAX+1] =
     { "none", "simple", "expo_dr", "complex", "cyclic1", "cyclic2", "cyclic3" };

/* Section: Virtual Channel */
static const char SECTION_VIRTCHAN[] = "virtchan";
#define VCHAN_TEMPLATE CHAN_TEMPLATE
#define VCHAN_TEMPLATE_VAL CHAN_TEMPLATE_VAL

/* Section: Trim */
static const char SECTION_TRIM[] = "trim";

#define TRIM_SOURCE MIXER_SOURCE
static const char TRIM_POS[]  = "pos";
static const char TRIM_NEG[]  = "neg";
static const char TRIM_STEP[] = "step";
static const char TRIM_VALUE[] = "value";

/* Section: Heli */
static const char SECTION_SWASH[] = "swash";
#define SWASH_TYPE MODEL_TYPE
static const char SWASH_AIL_INV[] = "ail_inv";
static const char SWASH_ELE_INV[] = "ele_inv";
static const char SWASH_COL_INV[] = "col_inv";

/* Section: Timer */
static const char SECTION_TIMER[] = "timer";

#define TIMER_SOURCE  MIXER_SOURCE
#define TIMER_TYPE MODEL_TYPE
static const char * const TIMER_TYPE_VAL[TIMER_LAST] = { "stopwatch", "countdown" };
static const char TIMER_TIME[] = "time";

/* Section: Safety */
static const char SECTION_SAFETY[] = "safety";
static const char * const SAFETY_VAL[SAFE_MAX+1] = { "none", "min", "zero", "max" };

/* Section: Telemetry */
static const char SECTION_TELEMETRY[] = "telemetry";
static const char TELEM_TEMP[] = "temp";
static const char * const TELEM_TEMP_VAL[2] = { "celcius", "farenheit"};
static const char TELEM_LENGTH[] = "length";
static const char * const TELEM_LENGTH_VAL[2] = { "meters", "feet" };

static const char SECTION_TELEMALARM[] = "telemalarm";
static const char TELEM_SRC[] = "source";
static const char TELEM_ABOVE[] =  "above";
static const char TELEM_VALUE[] = "value";

/* Section: Gui-QVGA */
static const char SECTION_GUI_QVGA[] = "gui-qvga";
#define GUI_TRIM SECTION_TRIM
static const char * const GUI_TRIM_VAL[TRIMS_LAST] = { "none", "4out", "4in", "6"};
static const char GUI_BARSIZE[] = "barsize";
static const char * const GUI_BARSIZE_VAL[BARS_LAST] = { "none", "half", "full"};
#define GUI_SOURCE MIXER_SOURCE
#define GUI_TIMER SECTION_TIMER
static const char GUI_TOGGLE[] = "toggle";
static const char GUI_TGLICO[] = "tglico";
static const char GUI_BAR[] = "bar";
static const char GUI_BOX[] = "box";
static const char GUI_TELEMETRY[] = "telemetry";
/* End */

s8 mapstrcasecmp(const char *s1, const char *s2)
{
    int i = 0;
    while(1) {
        if(s1[i] == s2[i]
           || (s2[i] >= 'a' && s1[i] + ('a'-'A') == s2[i])
           || (s1[i] >= 'a' && s2[i] + ('a'-'A') == s1[i])
           || (s2[i] == ' ' && s1[i] == '_')
           || (s1[i] == ' ' && s2[i] == '_'))
        {
            if(s1[i] == '\0')
                return 0;
            i++;
            continue;
        }
        return(s1[i] < s2[i] ? -1 : 1);
    }
}
static u8 get_source(const char *section, const char *value)
{
    u8 i;
    u8 val;
    const char *ptr = (value[0] == '!') ? value + 1 : value;
    const char *tmp;
    char cmp[10];
    for (i = 0; i <= NUM_SOURCES; i++) {
        if(mapstrcasecmp(INPUT_SourceName(cmp, i), ptr) == 0) {
            return ((ptr == value) ? 0 : 0x80) | i;
        }
    }
    for (i = 0; i < 4; i++) {
        if(mapstrcasecmp(tx_stick_names[i], ptr) == 0) {
            return ((ptr == value) ? 0 : 0x80) | (i + 1);
        }
    }
    i = 0;
    while((tmp = INPUT_MapSourceName(i++, &val))) {
        if(mapstrcasecmp(tmp, ptr) == 0) {
            return ((ptr == value) ? 0 : 0x80) | val;
        }
    }
    printf("%s: Could not parse Source %s\n", section, value);
    return 0;
}

static u8 get_button(const char *section, const char *value)
{
    u8 i;
    for (i = 0; i <= NUM_TX_BUTTONS; i++) {
        if(strcasecmp(INPUT_ButtonName(i), value) == 0) {
            return i;
        }
    }
    printf("%s: Could not parse Button %s\n", section, value);
    return 0;
}

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct Model *m = (struct Model *)user;
    u16 i;
    #define MATCH_SECTION(s) strcasecmp(section, s) == 0
    #define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
    #define MATCH_KEY(s)     strcasecmp(name,    s) == 0
    #define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
    #define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))
    if (MATCH_SECTION("")) {
        if(MATCH_KEY(MODEL_NAME)) {
            strncpy(m->name, value, sizeof(m->name)-1);
            m->name[sizeof(m->name)-1] = 0;
            return 1;
        }
        if(MATCH_KEY(MODEL_TEMPLATE)) {
            //A dummy rule
            return 1;
        }
        if (MATCH_KEY(MODEL_ICON)) {
            CONFIG_ParseIconName(m->icon, value);
            return 1;
        }
        if (MATCH_KEY(MODEL_TYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(MODEL_TYPE_VAL); i++) {
                if (MATCH_VALUE(MODEL_TYPE_VAL[i])) {
                    m->type = i;
                    return 1;
                }
            }
            return 0;
        }
        if (MATCH_KEY(MODEL_AUTOMAP)) {
            auto_map = atoi(value);
            return 1;
        }
    }
    if (MATCH_SECTION(SECTION_RADIO)) {
        if (MATCH_KEY(RADIO_PROTOCOL)) {
            for (i = 0; i < NUM_STR_ELEMS(RADIO_PROTOCOL_VAL); i++) {
                if (MATCH_VALUE(RADIO_PROTOCOL_VAL[i])) {
                    m->protocol = i;
                    return 1;
                }
            }
            printf("Unknown protocol: %s\n", value);
            return 1;
        }
        if (MATCH_KEY(RADIO_NUM_CHANNELS)) {
            m->num_channels = atoi(value);
            return 1;
        }
        if (MATCH_KEY(RADIO_FIXED_ID)) {
            m->fixed_id = atoi(value);
            return 1;
        }
        if (MATCH_KEY(RADIO_TX_POWER)) {
            for (i = 0; i < NUM_STR_ELEMS(RADIO_TX_POWER_VAL); i++) {
                if (MATCH_VALUE(RADIO_TX_POWER_VAL[i])) {
                    m->tx_power = i;
                    return 1;
                }
            }
            printf("Unknown Tx power: %s\n", value);
            return 1;
        }
        printf("Unknown Radio Key: %s\n", name);
        return 0;
    }
    if (MATCH_START(section, SECTION_MIXER)) {
        int idx;
        for (idx = 0; idx < NUM_MIXERS; idx++) {
            if(m->mixers[idx].src == 0)
                break;
        }
        s16 value_int = atoi(value);
        if (MATCH_KEY(MIXER_SOURCE)) {
            if (idx == NUM_MIXERS) {
                printf("%s: Only %d mixers are supported\n", section, NUM_MIXERS);
                return 1;
            }
            m->mixers[idx].src = get_source(section, value);
            return 1;
        }
        idx--;
        if (MATCH_KEY(MIXER_DEST)) {
            m->mixers[idx].dest = get_source(section, value) - NUM_INPUTS - 1;
            return 1;
        }
        if (MATCH_KEY(MIXER_SWITCH)) {
            m->mixers[idx].sw = get_source(section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_SCALAR)) {
            m->mixers[idx].scalar = value_int;
            return 1;
        }
        if (MATCH_KEY(MIXER_OFFSET)) {
            m->mixers[idx].offset = value_int;
            return 1;
        }
        if (MATCH_KEY(MIXER_USETRIM)) {
            m->mixers[idx].apply_trim = value_int;
            return 1;
        }
        if (MATCH_KEY(MIXER_MUXTYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(MIXER_MUXTYPE_VAL); i++) {
                if (MATCH_VALUE(MIXER_MUXTYPE_VAL[i])) {
                    m->mixers[idx].mux = i;
                    return 1;
                }
            }
            printf("%s: Unknown Mux type: %s\n", section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_CURVETYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(MIXER_CURVETYPE_VAL); i++) {
                if (MATCH_VALUE(MIXER_CURVETYPE_VAL[i])) {
                    m->mixers[idx].curve.type = i;
                    return 1;
                }
            }
            printf("%s: Unknown Curve type: %s\n", section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_CURVE_POINTS)) {
            u8 point = 0;
            u8 sign = 0;
            value_int = 0;
            const char *ptr = value;
            // This is a crude version of strtok/atoi
            while(1) {
                if(*ptr == ',' || *ptr == '\0') {
                    value_int = value_int * (sign ? -1 : 1);
                    if (point >= MAX_POINTS) {
                        printf("%s: Curve point %s is not valid (maxpoints = %d\n", section, name, MAX_POINTS);
                        return 0;
                    }
                    if (value_int > 100)
                        value_int = 100;
                    if (value_int < -100)
                        value_int = -100;
                    m->mixers[idx].curve.points[point] = value_int;
                    sign = 0;
                    value_int = 0;
                    if (*ptr == '\0')
                        return 1;
                    point++;
                } else if(*ptr == '-') {
                    sign = 1;
                } else if(*ptr >= '0' && *ptr <= '9') {
                    value_int = value_int * 10 + (*ptr - '0');
                } else {
                    printf("%s: Bad value in %s at:%s\n", section, name, ptr);
                    return 0;
                }
                ptr++;
            }
            return 1;
        }
        printf("%s: Couldn't parse key: %s\n", section, name);
        return 0;
    }
    if (MATCH_START(section, SECTION_CHANNEL)) {
        u8 idx = atoi(section + sizeof(SECTION_CHANNEL)-1);
        if (idx == 0) {
            printf("Unknown Channel: %s\n", section);
            return 0;
        }
        if (idx > NUM_OUT_CHANNELS) {
            printf("%s: Only %d channels are supported\n", section, NUM_OUT_CHANNELS);
            return 1;
        }
        idx--;
        s16 value_int = atoi(value);
        if (MATCH_KEY(CHAN_LIMIT_REVERSE)) {
            if (value_int)
                m->limits[idx].flags |= CH_REVERSE;
            else
                m->limits[idx].flags &= ~CH_REVERSE;
            return 1;
        }
        if (MATCH_KEY(CHAN_LIMIT_FAILSAFE)) {
            if(strcasecmp("off", value) == 0) {
                m->limits[idx].flags &= ~CH_FAILSAFE_EN;
            } else {
                m->limits[idx].failsafe = value_int;
                m->limits[idx].flags |= CH_FAILSAFE_EN;
            }
            return 1;
        }
        if (MATCH_KEY(CHAN_LIMIT_SAFETYSW)) {
            m->limits[idx].safetysw = get_source(section, value);
            return 1;
        }
        if (MATCH_KEY(CHAN_LIMIT_SAFETYVAL)) {
            m->limits[idx].safetyval = value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_LIMIT_MAX)) {
            m->limits[idx].max = value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_LIMIT_MIN)) {
            m->limits[idx].min = -value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_SCALAR)) {
            m->limits[idx].servoscale = value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_SUBTRIM)) {
            m->limits[idx].subtrim = value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_TEMPLATE)) {
            for (i = 0; i < NUM_STR_ELEMS(CHAN_TEMPLATE_VAL); i++) {
                if (MATCH_VALUE(CHAN_TEMPLATE_VAL[i])) {
                    m->templates[idx] = i;
                    return 1;
                }
            }
            printf("%s: Unknown template: %s\n", section, value);
            return 1;
        }
        printf("%s: Unknown key: %s\n", section, name);
        return 0;
    }
    if (MATCH_START(section, SECTION_VIRTCHAN)) {
        u8 idx = atoi(section + sizeof(SECTION_VIRTCHAN)-1);
        if (idx == 0) {
            printf("Unknown Virtual Channel: %s\n", section);
            return 0;
        }
        if (idx > NUM_VIRT_CHANNELS) {
            printf("%s: Only %d virtual channels are supported\n", section, NUM_VIRT_CHANNELS);
            return 1;
        }
        idx = idx + NUM_OUT_CHANNELS - 1;
        if (MATCH_KEY(VCHAN_TEMPLATE)) {
            for (i = 0; i < NUM_STR_ELEMS(VCHAN_TEMPLATE_VAL); i++) {
                if (MATCH_VALUE(VCHAN_TEMPLATE_VAL[i])) {
                    m->templates[idx] = i;
                    return 1;
                }
            }
            printf("%s: Unknown template: %s\n", section, value);
            return 1;
        }
        printf("%s: Unknown key: %s\n", section, name);
        return 0;
    }
    if (MATCH_START(section, SECTION_TRIM)) {
        u8 idx = atoi(section + sizeof(SECTION_TRIM)-1);
        if (idx == 0) {
            printf("Unknown Trim: %s\n", section);
            return 0;
        }
        if (idx > NUM_TRIMS) {
            printf("%s: Only %d trims are supported\n", section, NUM_TRIMS);
            return 1;
        }
        idx--;
        s16 value_int = atoi(value);
        if (MATCH_KEY(TRIM_SOURCE)) {
            m->trims[idx].src = get_source(section, value);
            return 1;
        }
        if (MATCH_KEY(TRIM_POS)) {
            m->trims[idx].pos = get_button(section, value);
            return 1;
        }
        if (MATCH_KEY(TRIM_NEG)) {
            m->trims[idx].neg = get_button(section, value);
            return 1;
        }
        if (MATCH_KEY(TRIM_STEP)) {
            m->trims[idx].step = value_int;
            return 1;
        }
        if (MATCH_KEY(TRIM_VALUE)) {
            m->trims[idx].value = value_int;
            return 1;
        }
        printf("%s: Unknown trim setting: %s\n", section, name);
        return 0;
    }
    if (MATCH_SECTION(SECTION_SWASH)) {
        if (MATCH_KEY(SWASH_TYPE)) {
            for (i = SWASH_TYPE_NONE; i <= SWASH_TYPE_90; i++) {
                if(strcasecmp(MIXER_SwashType(i), value) == 0) {
                    m->swash_type = i;
                    return 1;
                }
            }
            printf("%s: Unknown swash_type: %s\n", section, value);
            return 1;
        }
        s16 value_int = atoi(value);
        if (MATCH_KEY(SWASH_ELE_INV)) {
            if (value_int) 
                m->swash_invert |= 0x01;
            return 1;
        }
        if (MATCH_KEY(SWASH_AIL_INV)) {
            if (value_int) 
                m->swash_invert |= 0x02;
            return 1;
        }
        if (MATCH_KEY(SWASH_COL_INV)) {
            if (value_int) 
                m->swash_invert |= 0x04;
            return 1;
        }
    }
    if (MATCH_START(section, SECTION_TIMER)) {
        u8 idx = atoi(section + sizeof(SECTION_TIMER)-1);
        if (idx == 0) {
            printf("Unknown Timer: %s\n", section);
            return 0;
        }
        if (idx > NUM_TIMERS) {
            printf("%s: Only %d timers are supported\n", section, NUM_TIMERS);
            return 1;
        }
        idx--;
        if (MATCH_KEY(TIMER_TYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(TIMER_TYPE_VAL); i++) {
                if (MATCH_VALUE(TIMER_TYPE_VAL[i])) {
                    m->timer[idx].type = i;
                    return 1;
                }
            }
            printf("%s: Unknown timer type: %s\n", section, value);
            return 1;
        }
        if (MATCH_KEY(TIMER_SOURCE)) {
            m->timer[idx].src = get_source(section, value);
            return 1;
        }
        if (MATCH_KEY(TIMER_TIME)) {
            m->timer[idx].timer = atoi(value);
            return 1;
        }
    }
    if (MATCH_SECTION(SECTION_TELEMETRY)) {
        if (MATCH_KEY(TELEM_TEMP)) {
            if(strcasecmp(TELEM_TEMP_VAL[1], value) == 0)
                m->telem_flags |= TELEMFLAG_FAREN;
            return 1;
        }
        if (MATCH_KEY(TELEM_LENGTH)) {
            if(strcasecmp(TELEM_LENGTH_VAL[1], value) == 0)
                m->telem_flags |= TELEMFLAG_FEET;
            return 1;
        }
    }
    if (MATCH_START(section, SECTION_TELEMALARM)) {
        u8 idx = atoi(section + sizeof(SECTION_TELEMALARM)-1);
        if (idx == 0) {
            printf("Unknown Telem-alarm: %s\n", section);
            return 0;
        }
        if (idx > TELEM_NUM_ALARMS) {
            printf("%s: Only %d timers are supported\n", section, TELEM_NUM_ALARMS);
            return 1;
        }
        idx--;
        if (MATCH_KEY(TELEM_SRC)) {
            char str[20];
            for(i = TELEM_VOLT1; i <= TELEM_RPM2; i++) {
                if (strcasecmp(TELEMETRY_ShortName(str, i), value) == 0) {
                    m->telem_alarm[idx] = i;
                    return 1;
                }
            }
            printf("%s: Unknown telemetry src: %s\n", section, value);
            return 0;
        }
        if (MATCH_KEY(TELEM_ABOVE)) {
            if (atoi(value))
                m->telem_flags |= 1 << idx;
            else
                m->telem_flags &= ~(1 << idx);
            return 1;
        }
        if (MATCH_KEY(TELEM_VALUE)) {
            m->telem_alarm_val[idx] = atoi(value);
            return 1;
        }
    }
    if (MATCH_START(section, SECTION_SAFETY)) {
        int found = 0;
        u8 src;
        if (MATCH_KEY("auto")) {
            src = 0;
            found = 1;
        } else {
            src = get_source(section, name);
        }
        if(found || src) {
            u8 i;
            for (i = 0; i < NUM_STR_ELEMS(SAFETY_VAL); i++) {
                if (MATCH_VALUE(SAFETY_VAL[i])) {
                    m->safety[src] = i;
                    return 1;
                }
            }
        }
    }
    if (MATCH_START(section, SECTION_GUI_QVGA)) {
        if (MATCH_KEY(GUI_TRIM)) {
            for (i = 0; i < NUM_STR_ELEMS(GUI_TRIM_VAL); i++) {
                if (MATCH_VALUE(GUI_TRIM_VAL[i])) {
                    m->pagecfg.trims = i;
                    break;
                }
            }
            return 1;
        }
        if (MATCH_KEY(GUI_BARSIZE)) {
            for (i = 0; i < NUM_STR_ELEMS(GUI_BARSIZE_VAL); i++) {
                if (MATCH_VALUE(GUI_BARSIZE_VAL[i])) {
                    m->pagecfg.barsize = i;
                    break;
                }
            }
            return 1;
        }
        if (MATCH_START(name, GUI_BOX)) {
            char str[20];
            u8 idx = name[3] - '1';
            if (idx >= 8) {
                printf("%s: Unkown key: %s\n", section, name);
                return 1;
            }
            for(i = 0; i < NUM_TIMERS; i++) {
                if(mapstrcasecmp(value, TIMER_Name(str, i)) == 0) {
                    m->pagecfg.box[idx] = i + 1;
                    return 1;
                }
            }
            for(i = 0; i < NUM_TELEM; i++) {
                if(mapstrcasecmp(value, TELEMETRY_Name(str, i)) == 0) {
                    m->pagecfg.box[idx] = i + 1 + NUM_TIMERS;
                    return 1;
                }
            }
            u8 src = get_source(section, value);
            if(src > 0) {
                if(src <= NUM_INPUTS) {
                    printf("%s: Illegal input for box: %s\n", section, value);
                    return 1;
                }
                src -= NUM_INPUTS + 1 - (NUM_TIMERS + NUM_TELEM + 1);
            }
            m->pagecfg.box[idx] = src;
            return 1;
        }
        if (MATCH_START(name, GUI_BAR)) {
            u8 idx = name[3] - '1';
            if (idx >= 8) {
                printf("%s: Unkown key: %s\n", section, name);
                return 1;
            }
            u8 src = get_source(section, value);
            if(src > 0) {
                if(src <= NUM_INPUTS) {
                    printf("%s: Illegal input for bargraph: %s\n", section, value);
                    return 1;
                }
                src -= NUM_INPUTS;
            }
            m->pagecfg.bar[idx] = src;
            return 1;
        }
        if (MATCH_START(name, GUI_TOGGLE)) {
            u8 idx = name[6] - '1';
            if (idx >= 4) {
                printf("%s: Unkown key: %s\n", section, name);
                return 1;
            }
            m->pagecfg.toggle[idx] = get_source(section, value);
            return 1;
        }
        if (MATCH_START(name, GUI_TGLICO)) {
            u8 idx = name[6] - '1';
            if (idx >= 4) {
                printf("%s: Unkown key: %s\n", section, name);
                return 1;
            }
            m->pagecfg.tglico[idx] = atoi(value);
            return 1;
        }
    }
    printf("Unknown Section: '%s'\n", section);
    return 0;
}

static void get_model_file(char *file, u8 model_num)
{
    if (model_num == 0)
        sprintf(file, "models/default.ini");
    else
        sprintf(file, "models/model%d.ini", model_num);
}

u8 write_mixer(FILE *fh, struct Model *m, u8 channel)
{
    int idx;
    int i;
    char tmpstr[20];
    u8 changed = 0;
    for(idx = 0; idx < NUM_MIXERS; idx++) {
        if (! WRITE_FULL_MODEL && (m->mixers[idx].src == 0 || m->mixers[idx].dest != channel))
            continue;
        changed = 1;
        fprintf(fh, "[%s]\n", SECTION_MIXER);
        fprintf(fh, "%s=%s\n", MIXER_SOURCE, INPUT_SourceName(tmpstr, m->mixers[idx].src));
        fprintf(fh, "%s=%s\n", MIXER_DEST, INPUT_SourceName(tmpstr, m->mixers[idx].dest + NUM_INPUTS + 1));
        if(WRITE_FULL_MODEL || m->mixers[idx].sw != 0)
            fprintf(fh, "%s=%s\n", MIXER_SWITCH, INPUT_SourceName(tmpstr, m->mixers[idx].sw));
        if(WRITE_FULL_MODEL || m->mixers[idx].scalar != 100)
            fprintf(fh, "%s=%d\n", MIXER_SCALAR, m->mixers[idx].scalar);
        if(WRITE_FULL_MODEL || m->mixers[idx].offset != 0)
            fprintf(fh, "%s=%d\n", MIXER_OFFSET, m->mixers[idx].offset);
        if(WRITE_FULL_MODEL || m->mixers[idx].apply_trim != 1)
            fprintf(fh, "%s=%d\n", MIXER_USETRIM, m->mixers[idx].apply_trim);
        if(WRITE_FULL_MODEL || m->mixers[idx].mux != 0)
            fprintf(fh, "%s=%s\n", MIXER_MUXTYPE, MIXER_MUXTYPE_VAL[m->mixers[idx].mux]);
        if(WRITE_FULL_MODEL || m->mixers[idx].curve.type != 0) {
            fprintf(fh, "%s=%s\n", MIXER_CURVETYPE, MIXER_CURVETYPE_VAL[m->mixers[idx].curve.type]);
            u8 num_points = CURVE_NumPoints(&m->mixers[idx].curve);
            if (num_points > 0) {
                fprintf(fh, "%s=", MIXER_CURVE_POINTS);
                for (i = 0; i < num_points; i++) {
                    fprintf(fh, "%d", m->mixers[idx].curve.points[i]);
                    if (i != num_points - 1)
                        fprintf(fh, ",");
                }
                fprintf(fh, "\n");
            }
        }
    }
    return changed;
}

u8 CONFIG_WriteModel(u8 model_num) {
    char file[20];
    FILE *fh;
    u8 idx;
    u8 i;
    struct Model *m = &Model;
    get_model_file(file, model_num);
    fh = fopen(file, "w");
    if (! fh) {
        printf("Couldn't open file: %s\n", file);
        return 0;
    }
    CONFIG_EnableLanguage(0);
    fprintf(fh, "%s=%s\n", MODEL_NAME, m->name);
    if(m->icon[0] != 0)
        fprintf(fh, "%s=%s\n", MODEL_ICON, m->icon + 9);
    if(WRITE_FULL_MODEL || m->type != 0)
        fprintf(fh, "%s=%s\n", MODEL_TYPE, MODEL_TYPE_VAL[m->type]);
    fprintf(fh, "[%s]\n", SECTION_RADIO);
    fprintf(fh, "%s=%s\n", RADIO_PROTOCOL, RADIO_PROTOCOL_VAL[m->protocol]);
    fprintf(fh, "%s=%d\n", RADIO_NUM_CHANNELS, m->num_channels);
    if(WRITE_FULL_MODEL || m->fixed_id != 0)
        fprintf(fh, "%s=%d\n", RADIO_FIXED_ID, (int)m->fixed_id);
    fprintf(fh, "%s=%s\n", RADIO_TX_POWER, RADIO_TX_POWER_VAL[m->tx_power]);
    fprintf(fh, "\n");
    for(idx = 0; idx < NUM_OUT_CHANNELS; idx++) {
        if(!WRITE_FULL_MODEL &&
           m->limits[idx].flags == 0 &&
           m->limits[idx].safetysw == 0 &&
           m->limits[idx].safetyval == 0 &&
           m->limits[idx].max == DEFAULT_SERVO_LIMIT &&
           m->limits[idx].min == DEFAULT_SERVO_LIMIT &&
           m->limits[idx].servoscale == 100 &&
           m->templates[idx] == 0)
        {
            if (write_mixer(fh, m, idx))
                fprintf(fh, "\n");
            continue;
        }
        fprintf(fh, "[%s%d]\n", SECTION_CHANNEL, idx+1);
        if(WRITE_FULL_MODEL || (m->limits[idx].flags & CH_REVERSE))
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_REVERSE, (m->limits[idx].flags & CH_REVERSE) ? 1 : 0);
        if(WRITE_FULL_MODEL || m->limits[idx].safetysw != 0)
            fprintf(fh, "%s=%s\n", CHAN_LIMIT_SAFETYSW, INPUT_SourceName(file, m->limits[idx].safetysw));
        if(WRITE_FULL_MODEL || (m->limits[idx].flags & CH_FAILSAFE_EN)) {
            if(m->limits[idx].flags & CH_FAILSAFE_EN) {
                fprintf(fh, "%s=%d\n", CHAN_LIMIT_FAILSAFE, m->limits[idx].failsafe);
            } else {
                fprintf(fh, "%s=Off\n", CHAN_LIMIT_FAILSAFE);
            }
        }
        if(WRITE_FULL_MODEL || m->limits[idx].safetyval != 0)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_SAFETYVAL, m->limits[idx].safetyval);
        if(WRITE_FULL_MODEL || m->limits[idx].max != DEFAULT_SERVO_LIMIT)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_MAX, m->limits[idx].max);
        if(WRITE_FULL_MODEL || m->limits[idx].min != DEFAULT_SERVO_LIMIT)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_MIN, -(int)m->limits[idx].min);
        if(WRITE_FULL_MODEL || m->limits[idx].subtrim != 0)
            fprintf(fh, "%s=%d\n", CHAN_SUBTRIM, m->limits[idx].subtrim);
        if(WRITE_FULL_MODEL || m->limits[idx].servoscale != 100)
            fprintf(fh, "%s=%d\n", CHAN_SCALAR, m->limits[idx].servoscale);
        if(WRITE_FULL_MODEL || m->templates[idx] != 0)
            fprintf(fh, "%s=%s\n", CHAN_TEMPLATE, CHAN_TEMPLATE_VAL[m->templates[idx]]);
        write_mixer(fh, m, idx);
        fprintf(fh, "\n");
    }
    for(idx = 0; idx < NUM_VIRT_CHANNELS; idx++) {
        if(WRITE_FULL_MODEL || m->templates[idx+NUM_OUT_CHANNELS] != 0) {
            fprintf(fh, "[%s%d]\n", SECTION_VIRTCHAN, idx+1);
            if(WRITE_FULL_MODEL || m->templates[idx+NUM_OUT_CHANNELS] != 0)
                fprintf(fh, "%s=%s\n", VCHAN_TEMPLATE, VCHAN_TEMPLATE_VAL[m->templates[idx+NUM_OUT_CHANNELS]]);
        }
        if (write_mixer(fh, m, idx+NUM_OUT_CHANNELS))
            fprintf(fh, "\n");
    }
    for(idx = 0; idx < NUM_TRIMS; idx++) {
        if (! WRITE_FULL_MODEL && m->trims[idx].src == 0)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TRIM, idx+1);
        fprintf(fh, "%s=%s\n", TRIM_SOURCE,
             m->trims[idx].src >= 1 && m->trims[idx].src <= 4 
             ? tx_stick_names[m->trims[idx].src-1]
             : INPUT_SourceName(file, m->trims[idx].src));
        fprintf(fh, "%s=%s\n", TRIM_POS, INPUT_ButtonName(m->trims[idx].pos));
        fprintf(fh, "%s=%s\n", TRIM_NEG, INPUT_ButtonName(m->trims[idx].neg));
        if(WRITE_FULL_MODEL || m->trims[idx].step != 1)
            fprintf(fh, "%s=%d\n", TRIM_STEP, m->trims[idx].step);
        if(WRITE_FULL_MODEL || m->trims[idx].value)
            fprintf(fh, "%s=%d\n", TRIM_VALUE, m->trims[idx].value);
    }
    if (WRITE_FULL_MODEL || m->swash_type) {
        fprintf(fh, "[%s]\n", SECTION_SWASH);
        fprintf(fh, "%s=%s\n", SWASH_TYPE, MIXER_SwashType(m->swash_type));
        if (WRITE_FULL_MODEL || m->swash_invert & 0x01)
            fprintf(fh, "%s=1\n", SWASH_ELE_INV);
        if (WRITE_FULL_MODEL || m->swash_invert & 0x02)
            fprintf(fh, "%s=1\n", SWASH_AIL_INV);
        if (WRITE_FULL_MODEL || m->swash_invert & 0x04)
            fprintf(fh, "%s=1\n", SWASH_COL_INV);
    }
    for(idx = 0; idx < NUM_TIMERS; idx++) {
        if (! WRITE_FULL_MODEL && m->timer[idx].src == 0 && m->timer[idx].type == TIMER_STOPWATCH)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TIMER, idx+1);
        if (WRITE_FULL_MODEL || m->timer[idx].type != TIMER_STOPWATCH)
            fprintf(fh, "%s=%s\n", TIMER_TYPE, TIMER_TYPE_VAL[m->timer[idx].type]);
        if (WRITE_FULL_MODEL || m->timer[idx].src != 0)
            fprintf(fh, "%s=%s\n", TIMER_SOURCE, INPUT_SourceName(file, m->timer[idx].src));
        if (WRITE_FULL_MODEL || (m->timer[idx].type != TIMER_STOPWATCH && m->timer[idx].timer))
            fprintf(fh, "%s=%d\n", TIMER_TIME, m->timer[idx].timer);
    }
    if(WRITE_FULL_MODEL || (m->telem_flags & (TELEMFLAG_FEET | TELEMFLAG_FAREN))) {
        fprintf(fh, "[%s]\n", SECTION_TELEMETRY);
        if (WRITE_FULL_MODEL || (m->telem_flags & TELEMFLAG_FAREN)) {
            fprintf(fh, "%s=%s\n", TELEM_TEMP, TELEM_TEMP_VAL[(m->telem_flags & TELEMFLAG_FAREN) ? 1 : 0]);
        }
        if (WRITE_FULL_MODEL || (m->telem_flags & TELEMFLAG_FEET)) {
            fprintf(fh, "%s=%s\n", TELEM_LENGTH, TELEM_LENGTH_VAL[(m->telem_flags & TELEMFLAG_FEET) ? 1 : 0]);
        }
    }
    for(idx = 0; idx < TELEM_NUM_ALARMS; idx++) {
        if (! WRITE_FULL_MODEL && ! m->telem_alarm[idx])
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TELEMALARM, idx+1);
        fprintf(fh, "%s=%s\n", TELEM_SRC, TELEMETRY_ShortName(file, m->telem_alarm[idx]));
        if(WRITE_FULL_MODEL || (m->telem_flags & (1 << idx)))
            fprintf(fh, "%s=%d\n", TELEM_ABOVE, (m->telem_flags & (1 << idx)) ? 1 : 0);
        fprintf(fh, "%s=%d\n", TELEM_VALUE, m->telem_alarm_val[idx]);
    }
    fprintf(fh, "[%s]\n", SECTION_SAFETY);
    for(i = 0; i < NUM_SOURCES + 1; i++) {
        if (WRITE_FULL_MODEL || m->safety[i]) {
            fprintf(fh, "%s=%s\n", i == 0 ? "Auto" : INPUT_SourceName(file, i), SAFETY_VAL[m->safety[i]]);
        }
    }
    fprintf(fh, "[%s]\n", SECTION_GUI_QVGA);
    if (WRITE_FULL_MODEL || m->pagecfg.trims)
        fprintf(fh, "%s=%s\n", GUI_TRIM, GUI_TRIM_VAL[m->pagecfg.trims]);
    if (WRITE_FULL_MODEL || m->pagecfg.barsize)
        fprintf(fh, "%s=%s\n", GUI_BARSIZE, GUI_BARSIZE_VAL[m->pagecfg.barsize]);
    for(idx = 0; idx < 8; idx++) {
        if (WRITE_FULL_MODEL || m->pagecfg.box[idx]) {
            if(m->pagecfg.box[idx] && m->pagecfg.box[idx] <= NUM_TIMERS) {
                fprintf(fh, "%s%d=%s\n", GUI_BOX, idx+1, TIMER_Name(file, m->pagecfg.box[idx]-1));
            } else if(m->pagecfg.box[idx] && m->pagecfg.box[idx] - NUM_TIMERS <= NUM_TELEM) {
                fprintf(fh, "%s%d=%s\n", GUI_BOX, idx+1, TELEMETRY_Name(file, m->pagecfg.box[idx]-NUM_TIMERS-1));
            } else {
                u8 val = m->pagecfg.box[idx];
                if (val)
                    val += NUM_INPUTS-(NUM_TIMERS + NUM_TELEM);
                fprintf(fh, "%s%d=%s\n", GUI_BOX, idx+1, INPUT_SourceName(file, val));
            }
        }
    }
    for(idx = 0; idx < 8; idx++) {
        if (WRITE_FULL_MODEL || m->pagecfg.bar[idx]) {
            u8 val = m->pagecfg.bar[idx];
            if (val)
                val += NUM_INPUTS;
            fprintf(fh, "%s%d=%s\n", GUI_BAR, idx+1, INPUT_SourceName(file, val));
        }
    }
    for(idx = 0; idx < 4; idx++) {
        if (WRITE_FULL_MODEL || m->pagecfg.toggle[idx]) {
            u8 val = m->pagecfg.toggle[idx];
            fprintf(fh, "%s%d=%s\n", GUI_TOGGLE, idx+1, INPUT_SourceName(file, val));
            if (WRITE_FULL_MODEL || m->pagecfg.tglico[idx])
                fprintf(fh, "%s%d=%d\n", GUI_TGLICO, idx+1, m->pagecfg.tglico[idx]);
        }
    }
    CONFIG_EnableLanguage(1);
    fclose(fh);
    return 1;
}

void clear_model(u8 full)
{
    u8 i;
    if (full) {
        memset(&Model, 0, sizeof(Model));
    } else {
        memset(Model.mixers,   0, sizeof(Model.mixers));
        memset(Model.templates, 0, sizeof(Model.templates));
        memset(Model.trims,    0, sizeof(Model.trims));
        Model.swash_type = SWASH_TYPE_NONE;
        Model.swash_invert = 0;
    }
    for(i = 0; i < NUM_MIXERS; i++) {
        Model.mixers[i].scalar = 100;
        Model.mixers[i].apply_trim = 1;
    }
    for(i = 0; i < NUM_OUT_CHANNELS; i++) {
        MIXER_SetDefaultLimit(&Model.limits[i]);
    }
    for (i = 0; i < NUM_TRIMS; i++) {
        Model.trims[i].step = 1;
    }
}

u8 CONFIG_ReadModel(u8 model_num) {
    char file[20];
    crc32 = 0;
    Transmitter.current_model = model_num;
    get_model_file(file, model_num);
    clear_model(1);
    auto_map = 0;
    if (CONFIG_IniParse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
        return 0;
    }
    MIXER_SetMixers(NULL, 0);
    if(auto_map)
        MIXER_AdjustForProtocol();
    TIMER_Init();
    MIXER_RegisterTrimButtons();
    crc32 = Crc(&Model, sizeof(Model));
    if(! Model.name[0])
        sprintf(Model.name, "Model%d", model_num);
    return 1;
}

u8 CONFIG_SaveModelIfNeeded() {
    u32 newCrc = Crc(&Model, sizeof(Model));
    if (crc32 == newCrc)
        return 0;
    CONFIG_WriteModel(Transmitter.current_model);
    return 1;
}

void CONFIG_ResetModel()
{
    u8 model_num = Transmitter.current_model;
    CONFIG_ReadModel(0);
    Transmitter.current_model = model_num;
    sprintf(Model.name, "Model%d", model_num);
}

u8 CONFIG_GetCurrentModel() {
    return Transmitter.current_model;
}

const char *CONFIG_GetIcon(enum ModelType type) {
    const char *const icons[] = {
       "modelico/heli.bmp",
       "modelico/plane.bmp",
    };
    return icons[type];
}

const char *CONFIG_GetCurrentIcon() {
    if(Model.icon[0]) {
        return Model.icon;
    } else {
        return CONFIG_GetIcon(Model.type);
    }
}

void CONFIG_ParseIconName(char *name, const char *value)
{
    sprintf(name, "modelico/%s", value);
}

enum ModelType CONFIG_ParseModelType(const char *value)
{
    u8 i;
    for (i = 0; i < NUM_STR_ELEMS(MODEL_TYPE_VAL); i++) {
        if (MATCH_VALUE(MODEL_TYPE_VAL[i])) {
            return i;
        }
    }
    printf("Unknown model type: %s\n", value);
    return 0;
}

u8 CONFIG_ReadTemplate(u8 template_num) {
    char file[20];
    char filename[13];
    int type;

    if (! FS_OpenDir("template")) {
        printf("Failed to read dir 'template'\n");
        return 0;
    }
    while((type = FS_ReadDir(filename)) != 0) {
        if (type == 1 && strncasecmp(filename + strlen(filename) - 4, ".ini", 4) == 0) {
            template_num--;
            if (template_num == 0){
                sprintf(file, "template/%s", filename);
                break;
            }
        }
    }
    FS_CloseDir();
    if(template_num) {
        printf("Failed to find template #%d\n", template_num);
        return 0;
    }
    clear_model(0);
    auto_map = 0;
    if (CONFIG_IniParse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
        return 0;
    }
    if(auto_map)
        MIXER_AdjustForProtocol();
    MIXER_RegisterTrimButtons();
    return 1;
}
