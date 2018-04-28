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
extern const u8 EATRG[PROTO_MAP_LEN];

struct Model Model;
/*set this to write all model data even if it is the same as the default */
static u32 crc32;

const char * const MODEL_TYPE_VAL[MODELTYPE_LAST] = { "heli", "plane", "multi" };
const char * const RADIO_TX_POWER_VAL[TXPOWER_LAST] =
     { "100uW", "300uW", "1mW", "3mW", "10mW", "30mW", "100mW", "150mW" };

#define MATCH_SECTION(s) (strcasecmp(section, s) == 0)
#define MATCH_START(x,y) (strncasecmp(x, y, sizeof(y)-1) == 0)
#define MATCH_KEY(s)     (strcasecmp(name,    s) == 0)
#define MATCH_VALUE(s)   (strcasecmp(value,   s) == 0)
#define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))

#define WRITE_FULL_MODEL 0
static u8 auto_map;

const char *MODEL_NAME = "name";
const char *MODEL_ICON = "icon";
const char *MODEL_TYPE = "type";
const char *MODEL_TEMPLATE = "template";
const char *MODEL_AUTOMAP = "automap";
const char *MODEL_MIXERMODE = "mixermode";

/* Section: Radio */
static const char SECTION_RADIO[]   = "radio";

static const char RADIO_PROTOCOL[] = "protocol";
static const char RADIO_VIDEOSRC[] = "videosrc";
static const char RADIO_VIDEOCH[]  = "videoch";
static const char RADIO_VIDEOCONTRAST[]  = "videocontrast";
static const char RADIO_VIDEOBRIGHTNESS[]  = "videobrightness";

#define RADIO_PROTOCOL_VAL ProtocolNames

static const char RADIO_NUM_CHANNELS[] = "num_channels";
static const char RADIO_FIXED_ID[] = "fixed_id";

static const char RADIO_TX_POWER[] = "tx_power";

static const char SECTION_PROTO_OPTS[] = "protocol_opts";
/* Section: Mixer */
static const char SECTION_MIXER[]   = "mixer";

static const char MIXER_SOURCE[] = "src";
static const char MIXER_DEST[] = "dest";
static const char MIXER_SWITCH[] = "switch";
static const char MIXER_SCALAR[] = "scalar";
static const char MIXER_OFFSET[] = "offset";
static const char MIXER_USETRIM[] = "usetrim";

static const char MIXER_MUXTYPE[] = "muxtype";
static const char * const MIXER_MUXTYPE_VAL[MUX_LAST]  = { "replace", "multiply", "add", "max", "min", "delay", "beep" };

static const char MIXER_CURVETYPE[] = "curvetype";
static const char * const MIXER_CURVETYPE_VAL[CURVE_MAX+1] = {
   "none", "fixed", "min/max", "zero/max", "greater-than-0", "less-than-0", "absval",
   "expo", "deadband", "3point", "5point", "7point", "9point", "11point", "13point" };
static const char MIXER_CURVE_POINTS[] = "points";
static const char MIXER_CURVE_SMOOTH[] = "smooth";

/* Section: Channel */
static const char SECTION_CHANNEL[] = "channel";

static const char CHAN_LIMIT_REVERSE[] = "reverse";
static const char CHAN_LIMIT_SAFETYSW[] = "safetysw";
static const char CHAN_LIMIT_SAFETYVAL[] = "safetyval";
static const char CHAN_LIMIT_FAILSAFE[] = "failsafe";
static const char CHAN_LIMIT_MAX[] = "max";
static const char CHAN_LIMIT_MIN[] = "min";
static const char CHAN_LIMIT_SPEED[] = "speed";
static const char CHAN_SUBTRIM[] = "subtrim";
static const char CHAN_SCALAR_NEG[] = "scalar-";
#define CHAN_SCALAR   MIXER_SCALAR
#define CHAN_TEMPLATE MODEL_TEMPLATE
static const char * const CHAN_TEMPLATE_VAL[MIXERTEMPLATE_MAX+1] =
     { "none", "simple", "expo_dr", "complex", "cyclic1", "cyclic2", "cyclic3" };

/* Section: Virtual Channel */
static const char SECTION_VIRTCHAN[] = "virtchan";
#define VCHAN_TEMPLATE CHAN_TEMPLATE
#define VCHAN_TEMPLATE_VAL CHAN_TEMPLATE_VAL
#define VCHAN_NAME MODEL_NAME

/* Section: PPM-In */
static const char SECTION_PPMIN[] = "ppm-in";
static const char PPMIN_MAP[] = "map";
static const char PPMIN_MODE[] = "mode";
static const char * const PPMIN_MODE_VALUE[4] =  {"none", "channel", "stick", "extend"};
static const char PPMIN_CENTERPW[] = "centerpw";
static const char PPMIN_DELTAPW[] = "deltapw";
#define PPMIN_NUM_CHANNELS  RADIO_NUM_CHANNELS
#define PPMIN_SWITCH MIXER_SWITCH

/* Section: Trim */
static const char SECTION_TRIM[] = "trim";

#define TRIM_SOURCE MIXER_SOURCE
static const char TRIM_POS[]  = "pos";
static const char TRIM_NEG[]  = "neg";
static const char TRIM_STEP[] = "step";
static const char TRIM_VALUE[] = "value";
#define TRIM_SWITCH MIXER_SWITCH

/* Section: Heli */
static const char SECTION_SWASH[] = "swash";
#define SWASH_TYPE MODEL_TYPE
static const char SWASH_AIL_INV[] = "ail_inv";
static const char SWASH_ELE_INV[] = "ele_inv";
static const char SWASH_COL_INV[] = "col_inv";
static const char SWASH_AILMIX[] = "ail_mix";
static const char SWASH_ELEMIX[] = "ele_mix";
static const char SWASH_COLMIX[] = "col_mix";

/* Section: Timer */
static const char SECTION_TIMER[] = "timer";

#define TIMER_SOURCE  MIXER_SOURCE
#define TIMER_TYPE MODEL_TYPE
static const char * const TIMER_TYPE_VAL[TIMER_LAST] = {
    [TIMER_STOPWATCH]      = "stopwatch",
    [TIMER_STOPWATCH_PROP] = "stop-prop",
    [TIMER_COUNTDOWN]      = "countdown",
    [TIMER_COUNTDOWN_PROP] = "cntdn-prop",
    [TIMER_PERMANENT]      = "permanent",
    };
static const char TIMER_TIME[] = "time";
static const char TIMER_RESETSRC[] = "resetsrc";
static const char PERMANENT_TIMER[] = "permanent_timer";
static const char TIMER_VAL[] = "val";

/* Section: Safety */
static const char SECTION_SAFETY[] = "safety";
static const char * const SAFETY_VAL[SAFE_MAX+1] = { "none", "min", "zero", "max" };

/* Section: Telemetry */
static const char SECTION_TELEMALARM[] = "telemalarm";
static const char TELEM_SRC[] = "source";
static const char TELEM_ABOVE[] =  "above";
static const char TELEM_VALUE[] = "value";

/* Section: Datalog */
static const char SECTION_DATALOG[] = "datalog";
static const char DATALOG_RATE[] = "rate";
#define DATALOG_SWITCH MIXER_SWITCH
#define DATALOG_SOURCE TELEM_SRC

/* Section: Gui-QVGA */
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x
static const char SECTION_GUI[] = "gui-" STRINGIFY(LCD_WIDTH) "x" STRINGIFY(LCD_HEIGHT);
static const char GUI_QUICKPAGE[] = "quickpage";

 
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
    u32 i;
    unsigned val;
    const char *ptr = (value[0] == '!') ? value + 1 : value;
    const char *tmp;
    char cmp[10];
    for (i = 0; i <= NUM_SOURCES; i++) {
        if(mapstrcasecmp(INPUT_SourceNameReal(cmp, i), ptr) == 0) {
            #if defined(HAS_SWITCHES_NOSTOCK) && HAS_SWITCHES_NOSTOCK
            #define SWITCH_NOSTOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) | \
                                    (1 << INP_FMOD0) | (1 << INP_FMOD1))
            if ((Transmitter.ignore_src & SWITCH_NOSTOCK) == SWITCH_NOSTOCK) {
                if(mapstrcasecmp("FMODE0", ptr) == 0 ||
                   mapstrcasecmp("FMODE1", ptr) == 0 ||
                   mapstrcasecmp("HOLD0", ptr) == 0 ||
                   mapstrcasecmp("HOLD1", ptr) == 0)
                    break;
            }
            #endif //HAS_SWITCHES_NOSTOCK
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
    u32 i;
    for (i = 0; i <= NUM_TX_BUTTONS; i++) {
        if(strcasecmp(INPUT_ButtonName(i), value) == 0) {
            return i;
        }
    }
    printf("%s: Could not parse Button %s\n", section, value);
    return 0;
}

static int handle_proto_opts(struct Model *m, const char* key, const char* value, const char **opts)
{
    const char **popts = opts;
    int idx = 0;
    while(*popts) {
        if(mapstrcasecmp(*popts, key) == 0) {
            popts++;
            int start = exact_atoi(popts[0]);
            int end = exact_atoi(popts[1]);
            int is_num = ((start != 0 || end != 0) && (popts[2] == 0 || (popts[3] == 0 && exact_atoi(popts[2]) != 0))) ? 1 : 0;
            if(is_num) {
                m->proto_opts[idx] = atoi(value);
                return 1;
            }
            int val = 0;
            while(popts[val]) {
                if(mapstrcasecmp(popts[val], value) == 0) {
                    m->proto_opts[idx] = val;
                    return 1;
                }
                val++;
            }
            printf("Unknown protocol option '%s' for '%s'\n", value, key);
            return 1;
        }
        //Find end of options
        while(*popts) {
            popts++;
        }
        popts++; //Go to next option
        idx++;
    }
    return 0;
}

enum {
    S8,
    U8,
    S16
};
static const char * parse_partial_int_list(const char *ptr, void *vals, int *max_count, int type)
{
    //const char *origptr = ptr;
    int value_int = 0;
    int idx = 0;
    int sign = 0;

    while(1) {
        if(*ptr == ',' || *ptr == '\0') {
            value_int = value_int * (sign ? -1 : 1);
            if (type == S8) {
                if (value_int > 127)
                    value_int = 127;
                else if (value_int < -127)
                    value_int = -127;
                ((s8 *)vals)[idx] = value_int;
            } else if (type == U8) {
                if (value_int > 255)
                    value_int = 255;
                else if (value_int < 0)
                    value_int = 0;
                ((u8 *)vals)[idx] = value_int;
            } else {
                ((s16 *)vals)[idx] = value_int;
            }
            sign = 0;
            value_int = 0;
            idx++;
            --*max_count;
            if (*max_count == 0 || *ptr == '\0')
                return ptr;
        } else if(*ptr == '-') {
            sign = 1;
        } else if (*ptr >= '0' && *ptr <= '9') {
            value_int = value_int * 10 + (*ptr - '0');
        } else {
            //printf("Bad value '%c' in '%s'\n", *ptr, origptr);
            return ptr;
        }
        ptr++;
    }
}

static int parse_int_list(const char *ptr, void *vals, int max_count, int type)
{
    int count = max_count;
    parse_partial_int_list(ptr, vals, &count, type);
    return max_count - count;
}

static void create_element(struct elem *elem, int type, s16 *data)
{
    //int x, int y, int src, int e0, int e1, int e2)
    ELEM_SET_X(*elem, data[0]);
    ELEM_SET_Y(*elem, data[1]);
    ELEM_SET_TYPE(*elem, type);
    elem->src = data[5];
    elem->extra[0] = data[2];
    elem->extra[1] = data[3];
    elem->extra[2] = data[4];
}

static int layout_ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct Model *m = (struct Model *)user;
    u32 i;
    int offset_x = 0, offset_y = 0;
    CLOCK_ResetWatchdog();
    u32 idx;
    if (MATCH_START(name, GUI_QUICKPAGE)) {
        idx = name[9] - '1';
        if (idx >= NUM_QUICKPAGES) {
            printf("%s: Only %d quickpages are supported\n", section, NUM_QUICKPAGES);
            return 1;
        }
        u16 max = PAGE_GetNumPages();
        for(i = 0; i < max; i++) {
            if(mapstrcasecmp(PAGE_GetName(i), value) == 0) {
                m->pagecfg2.quickpage[idx] = i;
                return 1;
            }
        }
        printf("%s: Unknown page '%s' for quickpage%d\n", section, value, idx+1);
        return 1;
    }
#ifdef ENABLE_320x240_GUI
    static u8 seen_res = 0;
    enum {
        LOWRES = 1,
        HIRES,
    };
    if (! MATCH_SECTION(SECTION_GUI)) {
        if(MATCH_SECTION("gui-320x240")
           && (! ELEM_USED(Model.pagecfg2.elem[0]) || seen_res != HIRES))
        {
            seen_res = LOWRES;
            offset_x = (LCD_WIDTH - 320) / 2;
            offset_y = (LCD_HEIGHT - 240) / 2;
        } else
            return 1;
    } else {
        if (seen_res == LOWRES) {
            memset(&Model.pagecfg2.elem, 0, sizeof(Model.pagecfg2.elem));
        }
        seen_res = HIRES;
    }
#else 
    if (! MATCH_SECTION(SECTION_GUI))
        return 1;
#endif
    for (idx = 0; idx < NUM_ELEMS; idx++) {
        if (! ELEM_USED(Model.pagecfg2.elem[idx]))
            break;
    }
    
    if (idx == NUM_ELEMS) {
        printf("No free element available (max = %d)\n", NUM_ELEMS);
        return 1;
    }
    u32 type;
    for (type = 0; type < ELEM_LAST; type++)
        if(mapstrcasecmp(name, GetElemName(type)) == 0)
            break;
    if (type == ELEM_LAST)
        return 1;
    int count = 5;
    s16 data[6] = {0};
    const char *ptr = parse_partial_int_list(value, data, &count, S16);
    data[0] += offset_x;
    data[1] += offset_y;
    if (count > 3) {
        printf("Could not parse coordinates from %s=%s\n", name,value);
        return 1;
    }
    switch(type) {
        //case ELEM_MODEL:  //x, y
        case ELEM_VTRIM:  //x, y, src
        case ELEM_HTRIM:  //x, y, src
            data[5] = data[2];
            data[2] = 0;
            break;
        case ELEM_SMALLBOX: //x, y, src
        case ELEM_BIGBOX:   //x. y. src
        {
            s16 src = -1;
            char str[20];
            if (count != 3)
                return 1;
#if HAS_RTC
            for(i = 0; i < NUM_RTC; i++) {
                if(mapstrcasecmp(ptr, RTC_Name(str, i)) == 0) {
                    src = i + 1;
                    break;
                }
            }
#endif
            if (src == -1) {
                for(i = 0; i < NUM_TIMERS; i++) {
                    if(mapstrcasecmp(ptr, TIMER_Name(str, i)) == 0) {
                        src = i + 1 + NUM_RTC;
                        break;
                    }
                }
            }
            if (src == -1) {
                for(i = 0; i < NUM_TELEM; i++) {
                    if(mapstrcasecmp(ptr, TELEMETRY_Name(str, i+1)) == 0) {
                        src = i + 1 + NUM_RTC + NUM_TIMERS;
                        break;
                    }
                }
            }
            if (src == -1) {
                u8 newsrc = get_source(section, ptr);
                if(newsrc >= NUM_INPUTS) {
                    src = newsrc - (NUM_INPUTS + 1 - (NUM_RTC + NUM_TIMERS + NUM_TELEM + 1));
                }
            }
            if (src == -1)
                src = 0;
            data[5] = src;
            break;
        }
        case ELEM_BAR: //x, y, src
        {
            if (count != 3)
                return 1;
            u8 src = get_source(section, ptr);
            if (src < NUM_INPUTS)
                src = 0;
            data[5] = src - NUM_INPUTS;
            break;
        }
        case ELEM_TOGGLE: //x, y, tgl0, tgl1, tgl2, src
        {
            if(count)
                return 1;
            for (u32 j = 0; j <= NUM_SOURCES; j++) {
                char cmp[10];
                if(mapstrcasecmp(INPUT_SourceNameAbbrevSwitchReal(cmp, j), ptr+1) == 0) {
                    data[5] = j;
                    break;
                }
            }
            break;
        }
    }
    create_element(&m->pagecfg2.elem[idx], type, data);
    return 1;
}

struct struct_map {const char *str;  u16 offset; u16 defval;};
#define MAPSIZE(x)  (sizeof(x) / sizeof(struct struct_map))
#define OFFSET(s,v) (((long)(&s.v) - (long)(&s)) | ((sizeof(s.v)-1) << 13))
#define OFFSETS(s,v) (((long)(&s.v) - (long)(&s)) | ((sizeof(s.v)+3) << 13))
#define OFFSET_SRC(s,v) (((long)(&s.v) - (long)(&s)) | (2 << 13))
#define OFFSET_BUT(s,v) (((long)(&s.v) - (long)(&s)) | (6 << 13))
static const struct struct_map _secnone[] =
{
    {PERMANENT_TIMER, OFFSET(Model, permanent_timer), 0},
};
static const struct struct_map _secradio[] = {
    {RADIO_NUM_CHANNELS, OFFSET(Model, num_channels), 0},
    {RADIO_FIXED_ID,     OFFSET(Model, fixed_id), 0},
#if HAS_VIDEO
    {RADIO_VIDEOSRC,     OFFSET_SRC(Model, videosrc), 0},
    {RADIO_VIDEOCH,      OFFSET(Model, videoch), 0},
    {RADIO_VIDEOCONTRAST,OFFSET(Model, video_contrast), 0},
    {RADIO_VIDEOBRIGHTNESS,OFFSET(Model, video_brightness), 0},
#endif
};
static const struct struct_map _secmixer[] = {
    {MIXER_SWITCH, OFFSET_SRC(Model.mixers[0], sw), 0},
    {MIXER_SCALAR, OFFSETS(Model.mixers[0], scalar), 100},
    {MIXER_OFFSET, OFFSETS(Model.mixers[0], offset), 0},
};
static const struct struct_map _seclimit[] = {
    {CHAN_LIMIT_SAFETYSW,  OFFSET_SRC(Model.limits[0], safetysw), 0},
    {CHAN_LIMIT_SAFETYVAL, OFFSETS(Model.limits[0], safetyval), 0},
    {CHAN_LIMIT_MAX,       OFFSET(Model.limits[0], max), DEFAULT_SERVO_LIMIT},
    {CHAN_LIMIT_SPEED,     OFFSET(Model.limits[0], speed), 0},
    {CHAN_SCALAR,          OFFSET(Model.limits[0], servoscale), 100},
    {CHAN_SCALAR_NEG,      OFFSET(Model.limits[0], servoscale_neg), 0},
    {CHAN_SUBTRIM,         OFFSETS(Model.limits[0], subtrim), 0},
};
static const struct struct_map _sectrim[] = {
    {TRIM_SOURCE, OFFSET_SRC(Model.trims[0], src), 0xFFFF},
    {TRIM_POS,    OFFSET_BUT(Model.trims[0], pos), 0},
    {TRIM_NEG,    OFFSET_BUT(Model.trims[0], neg), 0},
    {TRIM_STEP,   OFFSET(Model.trims[0], step), 1},
};
static const struct struct_map _secswash[] = {
    {SWASH_AILMIX, OFFSET(Model, swashmix[0]), 60},
    {SWASH_ELEMIX, OFFSET(Model, swashmix[1]), 60},
    {SWASH_COLMIX, OFFSET(Model, swashmix[2]), 60},
};
static const struct struct_map _sectimer[] = {
    {TIMER_TIME,     OFFSET(Model.timer[0], timer), 0xFFFF},
    {TIMER_VAL,      OFFSET(Model.timer[0], val), 0xFFFF},
    {TIMER_SOURCE,   OFFSET_SRC(Model.timer[0], src), 0},
    {TIMER_RESETSRC, OFFSET_SRC(Model.timer[0], resetsrc), 0},
};
static const struct struct_map _secppm[] = {
    {PPMIN_CENTERPW, OFFSET(Model, ppmin_centerpw), 0},
    {PPMIN_DELTAPW,  OFFSET(Model, ppmin_deltapw), 0},
    {PPMIN_SWITCH,   OFFSET_SRC(Model, train_sw), 0xFFFF},
};
static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    int value_int = atoi(value);
    struct Model *m = (struct Model *)user;
int assign_int(void* ptr, const struct struct_map *map, int map_size)
{
    for(s32 i = 0; i < map_size; i++) {
        if(MATCH_KEY(map[i].str)) {
            int size = map[i].offset >> 13;
            int offset = map[i].offset & 0x1FFF;
            switch(size) {
                case 0:
                    *((u8 *)((long)ptr + offset)) = value_int; break;
                case 1:
                    *((u16 *)((long)ptr + offset)) = value_int; break;
                case 2:
                    *((u8 *)((long)ptr + offset)) = get_source(section, value); break;
                case 3:
                    *((u32 *)((long)ptr + offset)) = value_int; break;
                case 4:
                    *((s8 *)((long)ptr + offset)) = value_int; break;
                case 5:
                    *((s16 *)((long)ptr + offset)) = value_int; break;
                case 6:
                    *((u8 *)((long)ptr + offset)) = get_button(section, value); break;
                case 7:
                    *((s32 *)((long)ptr + offset)) = value_int; break;
            }
            return 1;
        }
    }
    return 0;
}
    CLOCK_ResetWatchdog();
    u32 i;
    if (MATCH_SECTION("")) {
        if(MATCH_KEY(MODEL_NAME)) {
            strlcpy(m->name, value, sizeof(m->name)-1);
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
        if(assign_int(&Model, _secnone, MAPSIZE(_secnone)))
            return 1;
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
        if (MATCH_KEY(MODEL_MIXERMODE)) {
            for(i = 0; i < 2; i++) {
                if(MATCH_VALUE(STDMIXER_ModeName(i)))
                    m->mixer_mode = i;
            }
            return 1;
        }
    }
    if (MATCH_SECTION(SECTION_RADIO)) {
        if (MATCH_KEY(RADIO_PROTOCOL)) {
            for (i = 0; i < NUM_STR_ELEMS(RADIO_PROTOCOL_VAL); i++) {
                if (MATCH_VALUE(RADIO_PROTOCOL_VAL[i])) {
                    m->protocol = i;
                    PROTOCOL_Load(1);
                    return 1;
                }
            }
            printf("Unknown protocol: %s\n", value);
            return 1;
        }
        if(assign_int(&Model, _secradio, MAPSIZE(_secradio)))
            return 1;
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
    if (MATCH_SECTION(SECTION_PROTO_OPTS)) {
        const char **opts = PROTOCOL_GetOptions();
        if (!opts || ! *opts)
            return 1;
        return handle_proto_opts(m, name, value, opts);
    }
    if (MATCH_START(section, SECTION_MIXER)) {
        u32 idx;
        for (idx = 0; idx < NUM_MIXERS; idx++) {
            if(m->mixers[idx].src == 0)
                break;
        }
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
        if(assign_int(&m->mixers[idx], _secmixer, MAPSIZE(_secmixer)))
            return 1;
        if (MATCH_KEY(MIXER_USETRIM)) {
            MIXER_SET_APPLY_TRIM(&m->mixers[idx], value_int);
            return 1;
        }
        if (MATCH_KEY(MIXER_MUXTYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(MIXER_MUXTYPE_VAL); i++) {
                if (MATCH_VALUE(MIXER_MUXTYPE_VAL[i])) {
                    MIXER_SET_MUX(&m->mixers[idx], i);
                    return 1;
                }
            }
            printf("%s: Unknown Mux type: %s\n", section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_CURVETYPE)) {
            for (i = 0; i < NUM_STR_ELEMS(MIXER_CURVETYPE_VAL); i++) {
                if (MATCH_VALUE(MIXER_CURVETYPE_VAL[i])) {
                    CURVE_SET_TYPE(&m->mixers[idx].curve, i);
                    return 1;
                }
            }
            printf("%s: Unknown Curve type: %s\n", section, value);
            return 1;
        }
        if (MATCH_KEY(MIXER_CURVE_POINTS)) {
            int count = parse_int_list(value, m->mixers[idx].curve.points, MAX_POINTS, S8);
            if (count > MAX_POINTS) {
                printf("%s: Too many points (max points = %d\n", section, MAX_POINTS);
                return 0;
            }
            return 1;
        }
        if (MATCH_KEY(MIXER_CURVE_SMOOTH)) {
            CURVE_SET_SMOOTHING(&m->mixers[idx].curve, value_int);
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
       
        if(assign_int(&m->limits[idx], _seclimit, MAPSIZE(_seclimit)))
            return 1;
        if (MATCH_KEY(CHAN_LIMIT_MIN)) {
            m->limits[idx].min = -value_int;
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
        if (MATCH_KEY(VCHAN_NAME)) {
            strlcpy(m->virtname[idx - NUM_OUT_CHANNELS], value, sizeof(m->virtname[0]));
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
        if(assign_int(&m->trims[idx], _sectrim, MAPSIZE(_sectrim)))
            return 1;
        if (MATCH_KEY(TRIM_SWITCH)) {
            for (i = 0; i <= NUM_SOURCES; i++) {
                char cmp[10];
                if(mapstrcasecmp(INPUT_SourceNameAbbrevSwitchReal(cmp, i), value) == 0) {
                    m->trims[idx].sw = i;
                    return 1;
                }
            }
            return 1;
        }
        if (MATCH_KEY(TRIM_VALUE)) {
            parse_int_list(value, m->trims[idx].value, 6, S8);
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
        if(assign_int(m, _secswash, MAPSIZE(_secswash)))
            return 1;
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
        if(assign_int(&m->timer[idx], _sectimer, MAPSIZE(_sectimer)))
            return 1;
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
            unsigned last = TELEMETRY_GetNumTelemSrc();
            for(i = 1; i <= last; i++) {
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
#if HAS_DATALOG
    if (MATCH_SECTION(SECTION_DATALOG)) {
        if (MATCH_KEY(DATALOG_SWITCH)) {
            m->datalog.enable = get_source(section, value);
        } else if (MATCH_KEY(DATALOG_RATE)) {
            for (i = 0; i < DLOG_RATE_LAST; i++) {
                if(mapstrcasecmp(DATALOG_RateString(i), value) == 0) {
                    m->datalog.rate = i;
                    break;
                }
            }
        } else if (MATCH_KEY(DATALOG_SOURCE)) {
            char cmp[10];
            for (i = 0; i < DLOG_LAST; i++) {
                if(mapstrcasecmp(DATALOG_Source(cmp, i), value) == 0) {
                    m->datalog.source[DATALOG_BYTE(i)] |= 1 << DATALOG_POS(i);
                    break;
                }
            }
        }
        return 1;
    }
#endif //HAS_DATALOG
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
    if (MATCH_START(section, "gui-")) {
        return layout_ini_handler(user, section, name, value);
    }
    if (MATCH_SECTION(SECTION_PPMIN)) {
        if (MATCH_KEY(PPMIN_NUM_CHANNELS)) {
            m->num_ppmin = (m->num_ppmin & 0xC0) | atoi(value);
            return 1;
        }
        if (MATCH_KEY(PPMIN_MODE)) {
            for(i = 0; i < 4; i++) {
                if(mapstrcasecmp(PPMIN_MODE_VALUE[i], value) == 0) {
                    m->num_ppmin = (m->num_ppmin & 0x3F) | (i << 6);
                    return 1;
                }
            }
            return 1;
        }
        if(assign_int(m, _secppm, MAPSIZE(_secppm)))
            return 1;
        if (MATCH_START(name, PPMIN_MAP)) {
            u8 idx = atoi(name + sizeof(PPMIN_MAP)-1) -1;
            if (idx < MAX_PPM_IN_CHANNELS) {
                m->ppm_map[idx]  = get_source(section, value);
                if (PPMin_Mode() == PPM_IN_TRAIN1) {
                    m->ppm_map[idx] =  (m->ppm_map[idx] <= NUM_INPUTS)
                                       ? -1
                                       : m->ppm_map[idx] - (NUM_INPUTS + 1);
                }
            }
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

void write_int(FILE *fh, void* ptr, const struct struct_map *map, int map_size)
{
    char tmpstr[20];
    for(s32 i = 0; i < map_size; i++) {
        int size = map[i].offset >> 13;
        int offset = map[i].offset & 0x1FFF;
        int value;
        if (map[i].defval == 0xffff)
            continue;
        switch(size) {
            case 0: 
            case 2: //SRC
            case 6: //BUTTON
                    value = *((u8 *)((long)ptr + offset)); break;
            case 1: value = *((u16 *)((long)ptr + offset)); break;
            case 3: value = *((u32 *)((long)ptr + offset)); break;
            case 4: value = *((s8 *)((long)ptr + offset)); break;
            case 5: value = *((s16 *)((long)ptr + offset)); break;
            case 7: value = *((s32 *)((long)ptr + offset)); break;
            default: continue;
        }
        if(WRITE_FULL_MODEL || value != map[i].defval) {
            if (2 == (size & 0x03)) //2, 6
                fprintf(fh, "%s=%s\n", map[i].str, size == 2 ? INPUT_SourceNameReal(tmpstr, value) : INPUT_ButtonName(value));
            else
                fprintf(fh, "%s=%d\n", map[i].str, value);
        }
    }
}

u8 write_mixer(FILE *fh, struct Model *m, u8 channel)
{
    u32 idx;
    s32 i;
    char tmpstr[20];
    u8 changed = 0;
    for(idx = 0; idx < NUM_MIXERS; idx++) {
        if (! WRITE_FULL_MODEL && (m->mixers[idx].src == 0 || m->mixers[idx].dest != channel))
            continue;
        changed = 1;
        fprintf(fh, "[%s]\n", SECTION_MIXER);
        fprintf(fh, "%s=%s\n", MIXER_SOURCE, INPUT_SourceNameReal(tmpstr, m->mixers[idx].src));
        fprintf(fh, "%s=%s\n", MIXER_DEST, INPUT_SourceNameReal(tmpstr, m->mixers[idx].dest + NUM_INPUTS + 1));
        write_int(fh, &m->mixers[idx], _secmixer, MAPSIZE(_secmixer));
        if(WRITE_FULL_MODEL || ! MIXER_APPLY_TRIM(&m->mixers[idx]))
            fprintf(fh, "%s=%d\n", MIXER_USETRIM, MIXER_APPLY_TRIM(&m->mixers[idx]) ? 1 : 0);
        if(WRITE_FULL_MODEL || MIXER_MUX(&m->mixers[idx]))
            fprintf(fh, "%s=%s\n", MIXER_MUXTYPE, MIXER_MUXTYPE_VAL[MIXER_MUX(&m->mixers[idx])]);
        if(WRITE_FULL_MODEL || CURVE_TYPE(&m->mixers[idx].curve)) {
            fprintf(fh, "%s=%s\n", MIXER_CURVETYPE, MIXER_CURVETYPE_VAL[CURVE_TYPE(&m->mixers[idx].curve)]);
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
            if (CURVE_SMOOTHING(&m->mixers[idx].curve))
                fprintf(fh, "%s=%d\n", MIXER_CURVE_SMOOTH, CURVE_SMOOTHING(&m->mixers[idx].curve) ? 1 : 0);
        }
    }
    return changed;
}

static void write_proto_opts(FILE *fh, struct Model *m)
{
    const char **opts = PROTOCOL_GetOptions();
    if (!opts || ! *opts)  // bug fix: must check NULL  ptr
        return;
    int idx = 0;
    fprintf(fh, "[%s]\n", SECTION_PROTO_OPTS);
    while(*opts) {
        int start = exact_atoi(opts[1]);
        int end = exact_atoi(opts[2]);
        int is_num = ((start != 0 || end != 0) && (opts[3] == 0 || (opts[4] == 0 && exact_atoi(opts[3]) != 0))) ? 1 : 0;
        if (is_num) {
            fprintf(fh, "%s=%d\n",*opts, m->proto_opts[idx]);
        } else {
            fprintf(fh, "%s=%s\n",*opts, opts[m->proto_opts[idx]+1]);
        }
        opts++;
        while(*opts) {
            opts++;
        }
        opts++;
        idx++;
    }
    fprintf(fh, "\n");
}

u8 CONFIG_WriteModel(u8 model_num) {
    char file[20];
    FILE *fh;
    u32 idx;
    struct Model *m = &Model;


    get_model_file(file, model_num);
    fh = fopen(file, "w");
    if (! fh) {
        printf("Couldn't open file: %s\n", file);
        return 0;
    }
    CONFIG_EnableLanguage(0);
    fprintf(fh, "%s=%s\n", MODEL_NAME, m->name);
    write_int(fh, m, _secnone, MAPSIZE(_secnone));
    fprintf(fh, "%s=%s\n", MODEL_MIXERMODE, STDMIXER_ModeName(m->mixer_mode));
    if(m->icon[0] != 0)
        fprintf(fh, "%s=%s\n", MODEL_ICON, m->icon + 9);
    if(WRITE_FULL_MODEL || m->type != 0)
        fprintf(fh, "%s=%s\n", MODEL_TYPE, MODEL_TYPE_VAL[m->type]);
    fprintf(fh, "[%s]\n", SECTION_RADIO);
    fprintf(fh, "%s=%s\n", RADIO_PROTOCOL, RADIO_PROTOCOL_VAL[m->protocol]);
    write_int(fh, m, _secradio, MAPSIZE(_secradio));
    fprintf(fh, "%s=%s\n", RADIO_TX_POWER, RADIO_TX_POWER_VAL[m->tx_power]);
    fprintf(fh, "\n");
    write_proto_opts(fh, m);
    struct Limit default_limit;
    memset(&default_limit, 0, sizeof(default_limit));
    MIXER_SetDefaultLimit(&default_limit);
    for(idx = 0; idx < NUM_OUT_CHANNELS; idx++) {
        if(!WRITE_FULL_MODEL
           && memcmp(&m->limits[idx], &default_limit, sizeof(default_limit)) == 0
           && m->templates[idx] == 0)
        {
            if (write_mixer(fh, m, idx))
                fprintf(fh, "\n");
            continue;
        }
        fprintf(fh, "[%s%d]\n", SECTION_CHANNEL, idx+1);
        if(WRITE_FULL_MODEL || (m->limits[idx].flags & CH_REVERSE))
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_REVERSE, (m->limits[idx].flags & CH_REVERSE) ? 1 : 0);
        write_int(fh, &m->limits[idx], _seclimit, MAPSIZE(_seclimit));
        if(WRITE_FULL_MODEL || (m->limits[idx].flags & CH_FAILSAFE_EN)) {
            if(m->limits[idx].flags & CH_FAILSAFE_EN) {
                fprintf(fh, "%s=%d\n", CHAN_LIMIT_FAILSAFE, m->limits[idx].failsafe);
            } else {
                fprintf(fh, "%s=Off\n", CHAN_LIMIT_FAILSAFE);
            }
        }
        if(WRITE_FULL_MODEL || m->limits[idx].min != DEFAULT_SERVO_LIMIT)
            fprintf(fh, "%s=%d\n", CHAN_LIMIT_MIN, -(int)m->limits[idx].min);
        if(WRITE_FULL_MODEL || m->templates[idx] != 0)
            fprintf(fh, "%s=%s\n", CHAN_TEMPLATE, CHAN_TEMPLATE_VAL[m->templates[idx]]);
        write_mixer(fh, m, idx);
        fprintf(fh, "\n");
    }
    for(idx = 0; idx < NUM_VIRT_CHANNELS; idx++) {
        if(WRITE_FULL_MODEL || m->templates[idx+NUM_OUT_CHANNELS] != 0 || m->virtname[idx][0]) {
            fprintf(fh, "[%s%d]\n", SECTION_VIRTCHAN, idx+1);
            if(m->virtname[idx][0])
                fprintf(fh, "%s=%s\n", VCHAN_NAME, m->virtname[idx]);
            if(WRITE_FULL_MODEL || m->templates[idx+NUM_OUT_CHANNELS] != 0)
                fprintf(fh, "%s=%s\n", VCHAN_TEMPLATE, VCHAN_TEMPLATE_VAL[m->templates[idx+NUM_OUT_CHANNELS]]);
        }
        if (write_mixer(fh, m, idx+NUM_OUT_CHANNELS))
            fprintf(fh, "\n");
    }
    if (PPMin_Mode()) {
        fprintf(fh, "[%s]\n", SECTION_PPMIN);
        fprintf(fh, "%s=%s\n", PPMIN_MODE, PPMIN_MODE_VALUE[PPMin_Mode()]);
        fprintf(fh, "%s=%d\n", PPMIN_NUM_CHANNELS, (m->num_ppmin & 0x3F));
        if (PPMin_Mode() != PPM_IN_SOURCE) {
            fprintf(fh, "%s=%s\n", PPMIN_SWITCH, INPUT_SourceNameReal(file, m->train_sw));
        }
        write_int(fh, m, _secppm, MAPSIZE(_secppm));
        //fprintf(fh, "%s=%d\n", PPMIN_CENTERPW, m->ppmin_centerpw);
        //fprintf(fh, "%s=%d\n", PPMIN_DELTAPW, m->ppmin_deltapw);
        if (PPMin_Mode() != PPM_IN_SOURCE) {
            int offset = (PPMin_Mode() == PPM_IN_TRAIN1) ? NUM_INPUTS + 1: 0;
            for(idx = 0; idx < MAX_PPM_IN_CHANNELS; idx++) {
                if (m->ppm_map[idx] == -1)
                    continue;
                fprintf(fh, "%s%d=%s\n", PPMIN_MAP, idx + 1, INPUT_SourceNameReal(file, m->ppm_map[idx] + offset));
            }
        }
        fprintf(fh, "\n");
    }
    for(idx = 0; idx < NUM_TRIMS; idx++) {
        if (! WRITE_FULL_MODEL && m->trims[idx].src == 0)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TRIM, idx+1);
        fprintf(fh, "%s=%s\n", TRIM_SOURCE,
             m->trims[idx].src >= 1 && m->trims[idx].src <= 4 
             ? tx_stick_names[m->trims[idx].src-1]
             : INPUT_SourceNameReal(file, m->trims[idx].src));
        write_int(fh, &m->trims[idx], _sectrim, MAPSIZE(_sectrim));
        if(WRITE_FULL_MODEL || m->trims[idx].sw)
            fprintf(fh, "%s=%s\n", TRIM_SWITCH, INPUT_SourceNameAbbrevSwitchReal(file, m->trims[idx].sw));
        if(WRITE_FULL_MODEL || m->trims[idx].value[0] || m->trims[idx].value[1] || m->trims[idx].value[2]
                            || m->trims[idx].value[3] || m->trims[idx].value[4] || m->trims[idx].value[5])
            fprintf(fh, "%s=%d,%d,%d,%d,%d,%d\n", TRIM_VALUE,
                               m->trims[idx].value[0], m->trims[idx].value[1], m->trims[idx].value[2],
                               m->trims[idx].value[3], m->trims[idx].value[4], m->trims[idx].value[5]);
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
        write_int(fh, m, _secswash, MAPSIZE(_secswash));
    }
    for(idx = 0; idx < NUM_TIMERS; idx++) {
        if (! WRITE_FULL_MODEL && m->timer[idx].src == 0 && m->timer[idx].type == TIMER_STOPWATCH)
            continue;
        fprintf(fh, "[%s%d]\n", SECTION_TIMER, idx+1);
        if (WRITE_FULL_MODEL || m->timer[idx].type != TIMER_STOPWATCH)
            fprintf(fh, "%s=%s\n", TIMER_TYPE, TIMER_TYPE_VAL[m->timer[idx].type]);
        write_int(fh, &m->timer[idx], _sectimer, MAPSIZE(_sectimer));
        if (WRITE_FULL_MODEL || ((m->timer[idx].type == TIMER_COUNTDOWN || m->timer[idx].type == TIMER_COUNTDOWN_PROP) && m->timer[idx].timer))
            fprintf(fh, "%s=%d\n", TIMER_TIME, m->timer[idx].timer);
        if (WRITE_FULL_MODEL || (m->timer[idx].val != 0 && m->timer[idx].type == TIMER_PERMANENT))
            fprintf(fh, "%s=%d\n", TIMER_VAL, m->timer[idx].val);
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
#if HAS_DATALOG
    fprintf(fh, "[%s]\n", SECTION_DATALOG);
    fprintf(fh, "%s=%s\n", DATALOG_SWITCH, INPUT_SourceNameReal(file, m->datalog.enable));
    fprintf(fh, "%s=%s\n", DATALOG_RATE, DATALOG_RateString(m->datalog.rate));
    for(idx = 0; idx < DLOG_LAST; idx++) {
        if(m->datalog.source[DATALOG_BYTE(idx)] & (1 << DATALOG_POS(idx)))
            fprintf(fh, "%s=%s\n", DATALOG_SOURCE, DATALOG_Source(file, idx));
    }
#endif //HAS_DATALOG
    fprintf(fh, "[%s]\n", SECTION_SAFETY);
    for(u32 i = 0; i < NUM_SOURCES + 1; i++) {
        if (WRITE_FULL_MODEL || m->safety[i]) {
            fprintf(fh, "%s=%s\n", i == 0 ? "Auto" : INPUT_SourceNameReal(file, i), SAFETY_VAL[m->safety[i]]);
        }
    }
    fprintf(fh, "[%s]\n", SECTION_GUI);
    for(idx = 0; idx < NUM_ELEMS; idx++) {
        if (! ELEM_USED(Model.pagecfg2.elem[idx]))
            break;
        int src = Model.pagecfg2.elem[idx].src;
        int x = ELEM_X(Model.pagecfg2.elem[idx]);
        int y = ELEM_Y(Model.pagecfg2.elem[idx]);
        int type = ELEM_TYPE(Model.pagecfg2.elem[idx]);
        const char *elename = GetElemName(type);
        switch(type) {
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
                fprintf(fh, "%s=%d,%d,%s\n", elename, x, y, GetBoxSourceReal(file, src));
                break;
            case ELEM_BAR:
                src += NUM_INPUTS;
                fprintf(fh, "%s=%d,%d,%s\n", elename, x, y, INPUT_SourceNameReal(file, src));
                break;
            case ELEM_TOGGLE:
                fprintf(fh, "%s=%d,%d,%d,%d,%d,%s\n", elename, x, y,
                        Model.pagecfg2.elem[idx].extra[0],
                        Model.pagecfg2.elem[idx].extra[1],
                        INPUT_NumSwitchPos(src) == 2 ? 0 : Model.pagecfg2.elem[idx].extra[2],
                        INPUT_SourceNameAbbrevSwitchReal(file, src));
                break;
            case ELEM_HTRIM:
            case ELEM_VTRIM:
                fprintf(fh, "%s=%d,%d,%d\n", elename, x, y, src);
                break;
            default:
                fprintf(fh, "%s=%d,%d\n", elename, x, y);
                break;
        }
    }
    for(idx = 0; idx < NUM_QUICKPAGES; idx++) {
        if (WRITE_FULL_MODEL || m->pagecfg2.quickpage[idx]) {
            u8 val = m->pagecfg2.quickpage[idx];
            fprintf(fh, "%s%d=%s\n", GUI_QUICKPAGE, idx+1, PAGE_GetName(val));
        }
    }
    CONFIG_EnableLanguage(1);
    fclose(fh);
    return 1;
}

void clear_model(u8 full)
{
    u32 i;
    if (full) {
        memset(&Model, 0, sizeof(Model));
    } else {
        memset(Model.mixers,   0, sizeof(Model.mixers));
        memset(Model.templates, 0, sizeof(Model.templates));
        memset(Model.trims,    0, sizeof(Model.trims));
        Model.swash_type = SWASH_TYPE_NONE;
        Model.swash_invert = 0;
    }
    Model.swashmix[0] = 60;
    Model.swashmix[1] = 60;
    Model.swashmix[2] = 60;
    for(i = 0; i < NUM_MIXERS; i++) {
        Model.mixers[i].scalar = 100;
        MIXER_SET_APPLY_TRIM(&Model.mixers[i], 1);
    }
    for(i = 0; i < NUM_OUT_CHANNELS; i++) {
        MIXER_SetDefaultLimit(&Model.limits[i]);
    }
    for (i = 0; i < NUM_TRIMS; i++) {
        Model.trims[i].step = 1;
    }
    for (i = 0; i < MAX_PPM_IN_CHANNELS; i++) {
        Model.ppm_map[i] = -1;
    }
    Model.ppmin_centerpw = 1500;
    Model.ppmin_deltapw = 400;
}

u8 CONFIG_ReadModel(u8 model_num) {
    crc32 = 0;
    Transmitter.current_model = model_num;
    clear_model(1);

    char file[20];
    auto_map = 0;
    get_model_file(file, model_num);
    if (CONFIG_IniParse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
    }
    if (! ELEM_USED(Model.pagecfg2.elem[0]))
        CONFIG_ReadLayout("layout/default.ini");
    if(! PROTOCOL_HasPowerAmp(Model.protocol))
        Model.tx_power = TXPOWER_150mW;
    MIXER_SetMixers(NULL, 0);
    if(auto_map)
        RemapChannelsForProtocol(EATRG);
    TIMER_Init();
    MIXER_RegisterTrimButtons();
    crc32 = Crc(&Model, sizeof(Model));
    if(! Model.name[0])
        sprintf(Model.name, "Model%d", model_num);
    if (PPMin_Mode())
        PPMin_Start();
    else
        PPMin_Stop();

    STDMIXER_Preset(); // bug fix: this must be invoked in all modes
    return 1;
}

u8 CONFIG_IsModelChanged() {
    u32 newCrc = Crc(&Model, sizeof(Model));
    return (crc32 != newCrc);
}

u8 CONFIG_SaveModelIfNeeded() {
    if (CONFIG_IsModelChanged())
        CONFIG_WriteModel(Transmitter.current_model);
    return 1;
}

void CONFIG_ResetModel()
{
    u8 model_num = Transmitter.current_model;
    PROTOCOL_DeInit();
    CONFIG_ReadModel(0);
    Transmitter.current_model = model_num;
    sprintf(Model.name, "Model%d", model_num);
}

u8 CONFIG_GetCurrentModel() {
    return Transmitter.current_model;
}

const char *CONFIG_GetIcon(enum ModelType type) {
    const char *const icons[] = {
       "modelico/heli" IMG_EXT,
       "modelico/plane" IMG_EXT,
       "modelico/multi" IMG_EXT,
    };
    return icons[type];
}

const char *CONFIG_GetCurrentIcon() {
    if(Model.icon[0]) {
        return fexists(Model.icon) ? Model.icon : UNKNOWN_ICON;
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
    u32 i;
    for (i = 0; i < NUM_STR_ELEMS(MODEL_TYPE_VAL); i++) {
        if (MATCH_VALUE(MODEL_TYPE_VAL[i])) {
            return i;
        }
    }
    printf("Unknown model type: %s\n", value);
    return 0;
}

u8 CONFIG_ReadTemplateByIndex(u8 template_num) {
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
                break;
            }
        }
    }
    FS_CloseDir();
    if(template_num) {
        printf("Failed to find template #%d\n", template_num);
        return 0;
    }
    return CONFIG_ReadTemplate(filename);
}

u8 CONFIG_ReadTemplate(const char *filename) {
    char file[25];

    sprintf(file, "template/%s", filename);
    clear_model(0);
    auto_map = 0;
    if (CONFIG_IniParse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
        return 0;
    }
    if(auto_map)
        RemapChannelsForProtocol(EATRG);
    MIXER_RegisterTrimButtons();
    STDMIXER_Preset(); // bug fix: this must be invoked in all modes
    if (Model.mixer_mode == MIXER_STANDARD)
        STDMIXER_SetChannelOrderByProtocol();
    return 1;
}

u8 CONFIG_ReadLayout(const char *filename) {
    memset(&Model.pagecfg2, 0, sizeof(Model.pagecfg2));
    if (CONFIG_IniParse(filename, layout_ini_handler, &Model)) {
        printf("Failed to parse Layout file: %s\n", filename);
        return 0;
    }
    return 1;
}
