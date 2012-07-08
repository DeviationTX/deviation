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
#include "ini.h"
#include <stdlib.h>
#include <string.h>

struct Model Model;

/* Section: Radio */
static const char SECTION_RADIO[]   = "radio";

static const char RADIO_PROTOCOL[] = "protocol";
static const char * const RADIO_PROTOCOL_VAL[] = { "none", "devo", "dsm2", "j6pro" };

static const char RADIO_NUM_CHANNELS[] = "num_channels";
static const char RADIO_FIXED_ID[] = "fixed_id";

static const char RADIO_TX_POWER[] = "tx_power";
static const char * const RADIO_TX_POWER_VAL[] = { "300uw", "1mw", "3mw", "10mw", "30mw", "100mw" };

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
static const char MIXER_CURVE_POINT[] = "curvepoint";

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

static const char TRIM_SRC[]  = "src";
static const char TRIM_POS[]  = "pos";
static const char TRIM_NEG[]  = "neg";
static const char TRIM_STEP[] = "step";
/* End */

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct Model *m = (struct Model *)user;
    u16 i;
    #define MATCH_SECTION(s) strcasecmp(section, s) == 0
    #define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
    #define MATCH_KEY(s)     strcasecmp(name,    s) == 0
    #define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
    #define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))
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
            m->mixers[idx].src = value_int;
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
        if (MATCH_START(name, MIXER_CURVE_POINT)) {
            u8 point = atoi(name + sizeof(MIXER_CURVE_POINT) - 1);
            if (point > MAX_POINTS) {
                printf("%s: Curve point %s is not valid (maxpoints = %d\n", section, name, MAX_POINTS);
                return 0;
            }
            if (value_int > 100 || value_int < 100) {
                printf("%s: Curve point %s is out of range %s is not within -100 <= x <= 100\n", section, name, value);
                value_int  = 0;
            }
            m->mixers[idx].curve.points[point] = value_int;
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
            m->limits[idx].reverse = value_int;
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
        if (MATCH_KEY(TRIM_SRC)) {
            m->trims[idx].src = value_int;
            return 1;
        }
        if (MATCH_KEY(TRIM_POS)) {
            m->trims[idx].pos = value_int;
            return 1;
        }
        if (MATCH_KEY(TRIM_NEG)) {
            m->trims[idx].neg = value_int;
            return 1;
        }
        if (MATCH_KEY(TRIM_STEP)) {
            m->trims[idx].step = value_int;
            return 1;
        }
        printf("%s: Unknown trim setting: %s\n", section, name);
        return 0;
    }
    return 0;
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

u8 CONFIG_ReadModel(const char *file) {
    clear_model();
    if (ini_parse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
        return 0;
    }
    return 1;
}
