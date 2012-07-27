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
#include "model.h"
#include "misc.h"
#include "ini.h"
#include <stdlib.h>
#include <string.h>

struct Model Model;
/*set this to write all model data even if it is the same as the default */
#define WRITE_FULL_MODEL 0
static u32 crc32;
static u8 current_model;
const char *MODEL_NAME = "name";
const char *MODEL_ICON = "icon";
const char *MODEL_TYPE = "type";
const char * const MODEL_TYPE_VAL[] = { "heli", "plane" };
/* Section: Radio */
static const char SECTION_RADIO[]   = "radio";

static const char RADIO_PROTOCOL[] = "protocol";
const char * const RADIO_PROTOCOL_VAL[] = {
     "None",
#ifdef PROTO_HAS_CYRF6936
     "DEVO", "WK2801", "WK2601", "WK2401", "DSM2", "J6Pro"
#endif
#ifdef PROTO_HAS_A7105
     "Flysky",
#endif
     };

static const char RADIO_NUM_CHANNELS[] = "num_channels";
static const char RADIO_FIXED_ID[] = "fixed_id";

static const char RADIO_TX_POWER[] = "tx_power";
const char * const RADIO_TX_POWER_VAL[] = { "300uW", "1mW", "3mW", "10mW", "30mW", "100mW" };

/* Section: Mixer */
static const char SECTION_MIXER[]   = "mixer";

static const char MIXER_SOURCE[] = "src";
static const char MIXER_DEST[] = "dest";
static const char MIXER_SWITCH[] = "switch";
static const char MIXER_SCALAR[] = "scalar";
static const char MIXER_OFFSET[] = "offset";

static const char MIXER_MUXTYPE[] = "muxtype";
static const char * const MIXER_MUXTYPE_VAL[]  = { "replace", "multiply", "add" };

static const char MIXER_CURVETYPE[] = "curvetype";
static const char * const MIXER_CURVETYPE_VAL[] = {
   "none", "min/max", "zero/max", "greater-than-0", "less-than-0", "absval",
   "expo", "3point", "5point", "7point", "9point", "11point", "13point" };
static const char MIXER_CURVE_POINTS[] = "points";

/* Section: Channel */
static const char SECTION_CHANNEL[] = "channel";

static const char CHAN_LIMIT_REVERSE[] = "reverse";
static const char CHAN_LIMIT_SAFETYSW[] = "safetysw";
static const char CHAN_LIMIT_SAFETYVAL[] = "safetyval";
static const char CHAN_LIMIT_MAX[] = "max";
static const char CHAN_LIMIT_MIN[] = "min";

static const char CHAN_TEMPLATE[] = "template";
static const char * const CHAN_TEMPLATE_VAL[]  = { "none", "simple", "expo_dr", "complex" };

/* Section: Trim */
static const char SECTION_TRIM[] = "trim";

#define TRIM_SOURCE MIXER_SOURCE
static const char TRIM_POS[]  = "pos";
static const char TRIM_NEG[]  = "neg";
static const char TRIM_STEP[] = "step";

/* Section: Timer */
static const char SECTION_TIMER[] = "timer";

#define TIMER_SOURCE  MIXER_SOURCE
#define TIMER_TYPE MODEL_TYPE
static const char * const TIMER_TYPE_VAL[] = { "stopwatch", "countdown" };
static const char TIMER_TIME[] = "time";

/* Section: Gui-QVGA */
static const char SECTION_GUI_QVGA[] = "gui-qvga";
#define GUI_TRIM SECTION_TRIM
static const char * const GUI_TRIM_VAL[] = { "none", "4out", "4in", "6"};
static const char GUI_BARSIZE[] = "barsize";
static const char * const GUI_BARSIZE_VAL[] = { "none", "half", "full"};
#define GUI_SOURCE MIXER_SOURCE
#define GUI_TIMER SECTION_TIMER
static const char GUI_TOGGLE[] = "toggle";
static const char GUI_BAR[] = "bar";
static const char GUI_BOX[] = "box";
/* End */

static u8 get_source(const char *section, const char *value)
{
    u8 i;
    const char *ptr = (value[0] == '!') ? value + 1 : value;
    char cmp[10];
    for (i = 0; i < NUM_INPUTS + NUM_CHANNELS; i++) {
        if(strcasecmp(MIXER_SourceName(cmp, i), ptr) == 0) {
            return ((ptr == value) ? 0 : 0x80) | i;
        }
    }
    printf("%s: Could not parse Source %s\n", section, value);
    return 0;
}

static u8 get_button(const char *section, const char *value)
{
    u8 i;
    for (i = 0; i <= NUM_TX_BUTTONS; i++) {
        if(strcasecmp(MIXER_ButtonName(i), value) == 0) {
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
    if (MATCH_SECTION("") && MATCH_KEY(MODEL_NAME)) {
        strncpy(m->name, value, sizeof(m->name)-1);
        m->name[sizeof(m->name)-1] = 0;
        return 1;
    }
    if (MATCH_SECTION("") && MATCH_KEY(MODEL_ICON)) {
        CONFIG_ParseModelName(m->icon, value);
        return 1;
    }
    if (MATCH_SECTION("") && MATCH_KEY(MODEL_TYPE)) {
        return 1;
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
        printf("Unkown Radio Key: %s\n", name);
        return 0;
    }
    if (MATCH_START(section, SECTION_MIXER)) {
        u8 idx = atoi(section + sizeof(SECTION_MIXER)-1);
        if (idx == 0) {
            printf("%s: Unknown Mixer\n", section);
            return 0;
        }
        if (idx > NUM_MIXERS) {
            printf("%s: Only %d mixers are supported\n", section, NUM_MIXERS);
            return 1;
        }
        idx--;
        s16 value_int = atoi(value);
        if (MATCH_KEY(MIXER_SOURCE)) {
            m->mixers[idx].src = get_source(section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_DEST)) {
            m->mixers[idx].dest = value_int;
            return 1;
        }
        if (MATCH_KEY(MIXER_SWITCH)) {
            m->mixers[idx].sw = value_int;
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
    }
    if (MATCH_START(section, SECTION_CHANNEL)) {
        u8 idx = atoi(section + sizeof(SECTION_CHANNEL)-1);
        if (idx == 0) {
            printf("Unknown Channel: %s\n", section);
            return 0;
        }
        if (idx > NUM_CHANNELS) {
            printf("%s: Only %d channels are supported\n", section, NUM_CHANNELS);
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
        if (MATCH_KEY(CHAN_LIMIT_SAFETYSW)) {
            m->limits[idx].safetysw = value_int;
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
            m->limits[idx].min = value_int;
            return 1;
        }
        if (MATCH_KEY(CHAN_TEMPLATE)) {
            for (i = 0; i < NUM_STR_ELEMS(CHAN_TEMPLATE_VAL); i++) {
                if (MATCH_VALUE(CHAN_TEMPLATE_VAL[i])) {
                    m->template[idx] = i;
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
            printf("%s: Only %d trims are supported\n", section, NUM_CHANNELS);
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
        printf("%s: Unknown trim setting: %s\n", section, name);
        return 0;
    }
    if (MATCH_START(section, SECTION_TIMER)) {
        u8 idx = atoi(section + sizeof(SECTION_TIMER)-1);
        if (idx == 0) {
            printf("Unknown Trim: %s\n", section);
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
            u8 idx = name[3] - '1';
            if (idx >= 8) {
                printf("%s: Unkown key: %s\n", section, name);
                return 1;
            }
            if (MATCH_START(value, GUI_TIMER)) {
                i = value[5] - '1';
                if (i < 2)
                    m->pagecfg.box[idx] = i+1;
                else
                    printf("%s: Unknown timer: %s\n", section, value);
                return 1;
            }
            u8 src = get_source(section, value);
            if(src > 0) {
                if(src <= NUM_INPUTS) {
                    printf("%s: Illegal input for box: %s\n", section, value);
                    return 1;
                }
                src -= NUM_INPUTS - 2;
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
    }
    printf("Unkown Section: '%s'\n", section);
    return 0;
}

static void get_model_file(char *file, u8 model_num)
{
    sprintf(file, "models/model%d.ini", model_num);
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
    fprintf(fh, "%s=%s\n", MODEL_NAME, m->name);
    if(m->icon[0] != 0)
        fprintf(fh, "%s=%s\n", MODEL_ICON, m->icon + 6);
    if(WRITE_FULL_MODEL || m->type != 0)
        fprintf(fh, "%s=%s\n", MODEL_TYPE, MODEL_TYPE_VAL[m->type]);
    fprintf(fh, "[%s]\n", SECTION_RADIO);
    fprintf(fh, "%s=%s\n", RADIO_PROTOCOL, RADIO_PROTOCOL_VAL[m->protocol]);
    fprintf(fh, "%s=%d\n", RADIO_NUM_CHANNELS, m->num_channels);
    if(WRITE_FULL_MODEL || m->fixed_id != 0)
        fprintf(fh, "%s=%d\n", RADIO_FIXED_ID, m->fixed_id);
    fprintf(fh, "%s=%s\n", RADIO_TX_POWER, RADIO_TX_POWER_VAL[m->tx_power]);

    for(idx = 0; idx < NUM_MIXERS; idx++) {
        if (! WRITE_FULL_MODEL && m->mixers[idx].src == 0)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_MIXER, idx+1);
        fprintf(fh, "%s=%s\n", MIXER_SOURCE, MIXER_SourceName(file, m->mixers[idx].src));
        fprintf(fh, "%s=%d\n", MIXER_DEST, m->mixers[idx].dest);
        if(WRITE_FULL_MODEL || m->mixers[idx].sw != 0)
            fprintf(fh, "%s=%d\n", MIXER_SWITCH, m->mixers[idx].sw);
        if(WRITE_FULL_MODEL || m->mixers[idx].scalar != 100)
            fprintf(fh, "%s=%d\n", MIXER_SCALAR, m->mixers[idx].scalar);
        if(WRITE_FULL_MODEL || m->mixers[idx].offset != 0)
            fprintf(fh, "%s=%d\n", MIXER_OFFSET, m->mixers[idx].offset);
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
    for(idx = 0; idx < NUM_CHANNELS; idx++) {
        if(!WRITE_FULL_MODEL &&
           m->limits[idx].flags == 0 &&
           m->limits[idx].safetysw == 0 &&
           m->limits[idx].safetyval == 0 &&
           m->limits[idx].max == 100 &&
           m->limits[idx].min == -100 &&
           m->template[idx] == 0)
        {
            continue;
        }
        fprintf(fh, "[%s%d]\n", SECTION_CHANNEL, idx+1);
        if(WRITE_FULL_MODEL || (m->limits[idx].flags & CH_REVERSE))
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_REVERSE, (m->limits[idx].flags & CH_REVERSE) ? 1 : 0);
        if(WRITE_FULL_MODEL || m->limits[idx].safetysw != 0)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_SAFETYSW, m->limits[idx].safetysw);
        if(WRITE_FULL_MODEL || m->limits[idx].safetyval != 0)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_SAFETYVAL, m->limits[idx].safetyval);
        if(WRITE_FULL_MODEL || m->limits[idx].max != 100)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_MAX, m->limits[idx].max);
        if(WRITE_FULL_MODEL || m->limits[idx].min != -100)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_MIN, m->limits[idx].min);
        if(WRITE_FULL_MODEL || m->template[idx] != 0)
            fprintf(fh, "%s=%s\n", CHAN_TEMPLATE, CHAN_TEMPLATE_VAL[m->template[idx]]);
    }
    for(idx = 0; idx < NUM_TRIMS; idx++) {
        if (! WRITE_FULL_MODEL && m->trims[idx].src == 0)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TRIM, idx+1);
        fprintf(fh, "%s=%s\n", TRIM_SOURCE, MIXER_SourceName(file, m->trims[idx].src));
        fprintf(fh, "%s=%s\n", TRIM_POS, MIXER_ButtonName(m->trims[idx].pos));
        fprintf(fh, "%s=%s\n", TRIM_NEG, MIXER_ButtonName(m->trims[idx].neg));
        if(WRITE_FULL_MODEL || m->trims[idx].step != 10)
            fprintf(fh, "%s=%d\n", TRIM_STEP, m->trims[idx].step);
    }
    for(idx = 0; idx < NUM_TIMERS; idx++) {
        if (! WRITE_FULL_MODEL && m->timer[idx].src == 0 && m->timer[idx].type == TIMER_STOPWATCH)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TIMER, idx+1);
        if (WRITE_FULL_MODEL || m->timer[idx].type != TIMER_STOPWATCH)
            fprintf(fh, "%s=%s\n", TIMER_TYPE, TIMER_TYPE_VAL[m->timer[idx].type]);
        if (WRITE_FULL_MODEL || m->timer[idx].src != 0)
            fprintf(fh, "%s=%s\n", TIMER_SOURCE, MIXER_SourceName(file, m->timer[idx].src));
        if (WRITE_FULL_MODEL || (m->timer[idx].type != TIMER_STOPWATCH && m->timer[idx].timer))
            fprintf(fh, "%s=%d\n", TIMER_TIME, m->timer[idx].timer);
    }
    fclose(fh);
    return 1;
}

void clear_model()
{
    u8 i;
    memset(&Model, 0, sizeof(Model));
    for(i = 0; i < NUM_MIXERS; i++) {
        Model.mixers[i].scalar = 100;
    }
    for(i = 0; i < NUM_CHANNELS; i++) {
        Model.limits[i].max = 100;
        Model.limits[i].min = -100;
    }
    for (i = 0; i < NUM_TRIMS; i++) {
        Model.trims[i].step = 10;
    }
}

u8 CONFIG_ReadModel(u8 model_num) {
    char file[20];
    current_model = model_num;
    get_model_file(file, model_num);
    clear_model();
    if (ini_parse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
        return 0;
    }
    TIMER_Init();
    crc32 = Crc(&Model, sizeof(Model));
    return 1;
}

u8 CONFIG_SaveModelIfNeeded() {
    u32 newCrc = Crc(&Model, sizeof(Model));
    if (crc32 == newCrc)
        return 0;
    CONFIG_WriteModel(current_model);
    return 1;
}

u8 CONFIG_GetCurrentModel() {
    return current_model;
}

const char *CONFIG_GetIcon(enum ModelType type) {
    const char *const icons[] = {
       "media/heli.bmp",
       "media/plane.bmp",
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

void CONFIG_ParseModelName(char *name, const char *value)
{
    strcpy(name, "media/");
    strncpy(name + 6, value, 12);
    name[19] = 0;
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
