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
#include "display.h"
#include <stdlib.h>
#include <string.h>

static const char FONT[] = "font";
static const char * const FONT_VAL[] = { "default", "throttle", "battery", "misc1"};
static const char COLOR[] = "color";

#define MATCH_SECTION(s) strcasecmp(section, s) == 0
#define MATCH_START(x,y) strncasecmp(x, y, sizeof(y)-1) == 0
#define MATCH_KEY(s)     strcasecmp(name,    s) == 0
#define MATCH_VALUE(s)   strcasecmp(value,   s) == 0
#define NUM_STR_ELEMS(s) (sizeof(s) / sizeof(char *))

extern const char * const FontNames[10];

struct display_settings Display;
u8 get_font(const char *value) {
    u8 i;
    for (i = 0; i < NUM_STR_ELEMS(FontNames); i++) {
       if(MATCH_VALUE(FontNames[i])) {
           return i + 1;
       }
    }
    printf("Unknown font: %s\n", value);
    return 0;
}

u16 get_color(const char *value) {
    u8 r, g, b;
    u32 color = strtol(value, NULL, 16);
    r = 0xff & (color >> 16);
    g = 0xff & (color >> 8);
    b = 0xff & (color >> 0);
    return RGB888_to_RGB565(r, g, b);
}

static int handle_label(struct FontDesc *label, const char *name, const char *value)
{
    if(MATCH_KEY(FONT)) {
        label->font = get_font(value);
        return 1;
    }
    if(MATCH_KEY(COLOR)) {
        label->color = get_color(value);
        return 1;
    }
    return 0;
}

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct display_settings *d = (struct display_settings *)user;
    if(MATCH_START(section, FONT) && strlen(section) > sizeof(FONT)) {
        u8 idx;
        for (idx = 0; idx < NUM_STR_ELEMS(FONT_VAL); idx++) {
            if (0 == strcasecmp(section + sizeof(FONT), FONT_VAL[idx])) {
                handle_label(&d->font[idx], name, value);
                return 1;
            }
        }
        printf("Couldn't parse font: %s\n", section);
        return 0;
    }
    if(MATCH_START(section, "keyboard")) {
        if(MATCH_KEY(FONT)) {
            d->keyboard.font = get_font(value);
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
        if(MATCH_KEY("fill_color")) {
            d->keyboard.fill_color = get_color(value);
            return 1;
        }
    }
    if(MATCH_START(section, "listbox")) {
        if(MATCH_KEY(FONT)) {
            d->listbox.font = get_font(value);
            return 1;
        }
        if(MATCH_KEY("bg_color")) {
            d->listbox.bg_color = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_color")) {
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
        if(MATCH_KEY("bg_bar")) {
            d->listbox.bg_bar = get_color(value);
            return 1;
        }
        if(MATCH_KEY("fg_bar")) {
            d->listbox.fg_bar = get_color(value);
            return 1;
        }
    }
    return 0;
}

u8 CONFIG_ReadDisplay()
{
    memset(&Display, 0, sizeof(Display));
    DEFAULT_FONT.font = 7;
    DEFAULT_FONT.color = 0xffff;
    return ini_parse("images/config.ini", ini_handler, (void *)&Display);
}
