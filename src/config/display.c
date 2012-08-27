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
#include "gui/gui.h"
#include "display.h"
#include <stdlib.h>
#include <string.h>

static const char FONT[] = "font";
static const char * const FONT_VAL[] = { "default", "modelname", "title", "bigbox", "smallbox", "battery", "batt_alarm", "tiny", "bold", "narrow", "small", "bigboxneg", "smallboxneg", "dialogtitle", "dialogbody"};
static const char COLOR[] = "color";
static const char BG_COLOR[] = "bg_color";
static const char FG_COLOR[] = "fg_color";
static const char FG_COLOR_POS[] = "fg_color_pos";
static const char FG_COLOR_NEG[] = "fg_color_neg";
static const char FG_COLOR_ZERO[] = "fg_color_zero";
static const char OUTLINE_COLOR[] = "outline_color";
static const char FONT_COLOR[] = "font_color";
static const char IS_TRANSPARENT[] = "transparent";
static const char * const BARGRAPH_VAL[] = { "bargraph", "trim" };
static const char BOX[] = "box_type";
static const char * const BOX_VAL[] = { "none", "center", "fill", "outline" };


#define MATCH_SECTION(s) strcasecmp(section, s) == 0
#define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
#define MATCH_KEY(s)     strcasecmp(name,    s) == 0
#define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
#define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))
#define SET_FLAG(var, value, flag) ((value) ? ((var) | (flag)) : ((var) & ~(flag)))
extern u8 FONT_GetFromString(const char *);
struct display_settings Display;

u16 get_color(const char *value) {
    u8 r, g, b;
    u32 color = strtol(value, NULL, 16);
    r = 0xff & (color >> 16);
    g = 0xff & (color >> 8);
    b = 0xff & (color >> 0);
    return RGB888_to_RGB565(r, g, b);
}

static int handle_label(struct LabelDesc *label, const char *name, const char *value)
{
    if(MATCH_KEY(FONT)) {
        label->font = FONT_GetFromString(value);
        return 1;
    }
    if(MATCH_KEY(FONT_COLOR)) {
        label->font_color = get_color(value);
        return 1;
    }
    if(MATCH_KEY(BG_COLOR)) {
        label->fill_color = get_color(value);
        return 1;
    }
    if(MATCH_KEY(OUTLINE_COLOR)) {
        label->outline_color = get_color(value);
        return 1;
    }
    if(MATCH_KEY(BOX)) {
        u8 idx;
        for (idx = 0; idx < NUM_STR_ELEMS(BOX_VAL); idx++) {
            if(MATCH_VALUE(BOX_VAL[idx])) {
                label->style = idx;
            }
        }
    }
    return 0;
}

static int handle_bargraph(struct display_settings *d, u8 idx, const char *name, const char *value)
{
    struct disp_bargraph *graph;
    enum DispFlags flag;
    if (idx == 0) {
        graph = &d->bargraph;
        flag = BAR_TRANSPARENT;
    } else {
        graph = &d->trim;
        flag = TRIM_TRANSPARENT;
    }
    if(MATCH_KEY(BG_COLOR)) {
        graph->bg_color = get_color(value);
        return 1;
    }
    if(MATCH_KEY(FG_COLOR)) {
        graph->fg_color_pos = graph->fg_color_neg = graph->fg_color_zero = get_color(value);
        return 1;
    }
    if(MATCH_KEY(FG_COLOR_POS)) {
        graph->fg_color_pos = get_color(value);
        return 1;
    }
    if(MATCH_KEY(FG_COLOR_NEG)) {
        graph->fg_color_neg = get_color(value);
        return 1;
    }
    if(MATCH_KEY(FG_COLOR_ZERO)) {
        graph->fg_color_zero = get_color(value);
        return 1;
    }
    if(MATCH_KEY(OUTLINE_COLOR)) {
        graph->outline_color = get_color(value);
        return 1;
    }
    if(MATCH_KEY(IS_TRANSPARENT)) {
        d->flags = SET_FLAG(d->flags, atoi(value), flag);
        return 1;
    }
    return 0;
}

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    u8 idx;
    struct display_settings *d = (struct display_settings *)user;
    if(MATCH_START(section, FONT) && strlen(section) > sizeof(FONT)) {
        for (idx = 0; idx < NUM_STR_ELEMS(FONT_VAL); idx++) {
            if (0 == strcasecmp(section + sizeof(FONT), FONT_VAL[idx])) {
                handle_label(&d->font[idx], name, value);
                return 1;
            }
        }
        printf("Couldn't parse font: %s\n", section);
        return 0;
    }
    if(MATCH_START(section, "general")) {
        if(MATCH_KEY("bat_icon")) {
            d->flags = SET_FLAG(d->flags, atoi(value), SHOW_BAT_ICON);
            return 1;
        }
    }
    if(MATCH_START(section, "select")) {
        if(MATCH_KEY(COLOR)) {
            d->select_color = get_color(value);
            return 1;
        }
        if(MATCH_KEY("width")) {
            d->select_width = atoi(value);
            return 1;
        }
    }
    if(MATCH_START(section, "keyboard")) {
        if(MATCH_KEY(FONT)) {
            d->keyboard.font = FONT_GetFromString(value);
            return 1;
        }
        if(MATCH_KEY("bg_key1")) {
            d->keyboard.bg_key1 = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_key1")) {
            d->keyboard.fg_key1 = get_color(value);
            return 1;
        }
        if(MATCH_KEY("bg_key2")) {
            d->keyboard.bg_key2 = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_key2")) {
            d->keyboard.fg_key2 = get_color(value);
            return 1;
        }
        if(MATCH_KEY("bg_key3")) {
            d->keyboard.bg_key3 = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_key3")) {
            d->keyboard.fg_key3 = get_color(value);
            return 1;
        }
        if(MATCH_KEY(BG_COLOR)) {
            d->keyboard.fill_color = get_color(value);
            return 1;
        }
    }
    if(MATCH_START(section, "listbox")) {
        if(MATCH_KEY(FONT)) {
            d->listbox.font = FONT_GetFromString(value);
            return 1;
        }
        if(MATCH_KEY(BG_COLOR)) {
            d->listbox.bg_color = get_color(value);
            return 1;
        }
        if(MATCH_KEY(FG_COLOR)) {
            d->listbox.fg_color = get_color(value);
            return 1;
        }
        if(MATCH_KEY("bg_select")) {
            d->listbox.bg_select = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_select")) {
            d->listbox.fg_select = get_color(value);
            return 1;
        }
    }
    if(MATCH_START(section, "scrollbar")) {
        if(MATCH_KEY(BG_COLOR)) {
            d->scrollbar.bg_color = get_color(value);
            return 1;
        }
        if(MATCH_KEY(FG_COLOR)) {
            d->scrollbar.fg_color = get_color(value);
            return 1;
        }
    }
    for (idx = 0; idx < NUM_STR_ELEMS(BARGRAPH_VAL); idx++) {
        if(MATCH_SECTION(BARGRAPH_VAL[idx])) {
            handle_bargraph(d, idx, name, value);
            return 1;
        }
    }
    printf("Could not handle [%s] %s=%s\n", section, name, value);
    return 0;
}

u8 CONFIG_ReadDisplay()
{
    memset(&Display, 0, sizeof(Display));
    DEFAULT_FONT.font = 7;
    DEFAULT_FONT.font_color = 0xffff;
    return CONFIG_IniParse("media/config.ini", ini_handler, (void *)&Display);
}
