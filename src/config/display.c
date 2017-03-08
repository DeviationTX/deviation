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
#define font_index(x) (&(x) - Display.font)
static const char FONT[] = "font";
static const char * const FONT_VAL[] = {
    [font_index(DEFAULT_FONT)]      = "default",
    [font_index(MODELNAME_FONT)]    = "modelname",
    [font_index(TITLE_FONT)]        = "title",
    [font_index(BIGBOX_FONT)]       = "bigbox",
    [font_index(SMALLBOX_FONT)]     = "smallbox",
    [font_index(BATTERY_FONT)]      = "battery",
    [font_index(BATTALARM_FONT)]    = "batt_alarm",
    [font_index(TINY_FONT)]         = "tiny",
    [font_index(LIST_FONT)]         = "list",
    [font_index(LABEL_FONT)]        = "label",
    [font_index(NARROW_FONT)]       = "narrow",
    [font_index(SMALL_FONT)]        = "small",
    [font_index(BIGBOXNEG_FONT)]    = "bigboxneg",
    [font_index(SMALLBOXNEG_FONT)]  = "smallboxneg",
    [font_index(DIALOGTITLE_FONT)]  = "dialogtitle",
    [font_index(DIALOGBODY_FONT)]   = "dialogbody",
    [font_index(NORMALBOX_FONT)]    = "normalbox",
    [font_index(NORMALBOXNEG_FONT)] = "normalboxneg",
    [font_index(SECTION_FONT)]      = "section",
    [font_index(TEXTSEL_FONT)]      = "textsel", 
    [font_index(BUTTON_FONT)]       = "button",
    [font_index(MENU_FONT)]         = "menu",
    [font_index(LISTBOX_FONT)]      = "listbox"
    };
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
static const char XY_AXIS_COLOR[] = "axis_color";
static const char XY_GRID_COLOR[] = "grid_color";
static const char XY_POINT_COLOR[] = "point_color";
static const char BOX[] = "box_type";
static const char * const BOX_VAL[] = {
    [LABEL_NO_BOX]      = "none",
    [LABEL_CENTER]      = "center",    // Left for compatibility only
    [LABEL_LEFT]        = "left",      // Left for compatibility only
    [LABEL_RIGHT]       = "right",     // Left for compatibility only
    [LABEL_FILL]        = "fill",
    [LABEL_BOX]         = "outline",
    [LABEL_UNDERLINE]   = "underline",
#if LCD_DEPTH == 1
    [LABEL_SQUAREBOX]   = "squarebox",
    [LABEL_INVERTED]    = "inverted"
#else
    [LABEL_LISTBOX]     = "listbox"
#endif
    };
static const char ALIGN[] = "align";
static const char * const ALIGN_VAL[] = {
    [ALIGN_CENTER]      = "center",
    [ALIGN_LEFT]        = "left",
    [ALIGN_RIGHT]       = "right",
    };


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
            if(BOX_VAL[idx] && MATCH_VALUE(BOX_VAL[idx])) {
                label->style = idx;
                // For compatibility reasons,
                // use the old alignment values as default
                // if the new one is not yet set:
                if(label->align == 0) {
                    switch(idx) {
                        case LABEL_CENTER: label->align = ALIGN_CENTER; break;
                        case LABEL_LEFT:   label->align = ALIGN_LEFT;   break;
                        case LABEL_RIGHT:  label->align = ALIGN_RIGHT;  break;
                    }
                }
            }
        }
    }
    if(MATCH_KEY(ALIGN)) {
        u8 idx;
        for (idx = 0; idx < NUM_STR_ELEMS(ALIGN_VAL); idx++) {
            if(ALIGN_VAL[idx] && MATCH_VALUE(ALIGN_VAL[idx])) {
                label->align = idx;
            }
        }
    }
    return 0;
}

struct struct_map {const char *str;  u16 offset;};
#define MAPSIZE(x)  (sizeof(x) / sizeof(struct struct_map))
#define OFFSET(s,v) (((long)(&s.v) - (long)(&s)) | ((sizeof(s.v)-1) << 13))
#define OFFSETS(s,v) (((long)(&s.v) - (long)(&s)) | ((sizeof(s.v)+3) << 13))
#define OFFSET_COL(s,v) (((long)(&s.v) - (long)(&s)) | (2 << 13))
#define OFFSET_FON(s,v) (((long)(&s.v) - (long)(&s)) | (6 << 13))
static const struct struct_map _secgeneral[] =
{
    {"header_height",        OFFSET(Display.metrics, header_height)},
    {"header_widget_height", OFFSET(Display.metrics, header_widget_height)},
    {"line_height",          OFFSET(Display.metrics, line_height)},
    {"line_space",           OFFSET(Display.metrics, line_space)},
};
static const struct struct_map _secselect[] =
{
    {"width",                OFFSET(Display, select_width)},
    {COLOR,                  OFFSET_COL(Display, select_color)},
};
static const struct struct_map _seckeybd[] =
{
    {FONT,                   OFFSET_FON(Display.keyboard, font)},
    {"bg_key1",              OFFSET_COL(Display.keyboard, bg_key1)},
    {"fg_key1",              OFFSET_COL(Display.keyboard, fg_key1)},
    {"bg_key2",              OFFSET_COL(Display.keyboard, bg_key2)},
    {"fg_key2",              OFFSET_COL(Display.keyboard, fg_key2)},
    {"bg_key3",              OFFSET_COL(Display.keyboard, bg_key3)},
    {"fg_key3",              OFFSET_COL(Display.keyboard, fg_key3)},
    {BG_COLOR,               OFFSET_COL(Display.keyboard, fill_color)},
};
static const struct struct_map _seclistbox[] =
{
    {FONT,                   OFFSET_FON(Display.listbox, font)},
    {BG_COLOR,               OFFSET_COL(Display.listbox, bg_color)},
    {FG_COLOR,               OFFSET_COL(Display.listbox, fg_color)},
    {"bg_select",            OFFSET_COL(Display.listbox, bg_select)},
    {"fg_select",            OFFSET_COL(Display.listbox, fg_select)},
};
static const struct struct_map _secscroll[] =
{
    {BG_COLOR,               OFFSET_COL(Display.scrollbar, bg_color)},
    {FG_COLOR,               OFFSET_COL(Display.scrollbar, fg_color)},
};
static const struct struct_map _secxygraph[] =
{
    {BG_COLOR,               OFFSET_COL(Display.xygraph, bg_color)},
    {FG_COLOR,               OFFSET_COL(Display.xygraph, fg_color)},
    {XY_AXIS_COLOR,          OFFSET_COL(Display.xygraph, axis_color)},
    {XY_GRID_COLOR,          OFFSET_COL(Display.xygraph, grid_color)},
    {XY_POINT_COLOR,         OFFSET_COL(Display.xygraph, point_color)},
    {OUTLINE_COLOR,          OFFSET_COL(Display.xygraph, outline_color)},
};
static const struct struct_map _secbargraph[] =
{
    {BG_COLOR,               OFFSET_COL(Display.bargraph, bg_color)},
    {FG_COLOR_POS,           OFFSET_COL(Display.bargraph, fg_color_pos)},
    {FG_COLOR_NEG,           OFFSET_COL(Display.bargraph, fg_color_neg)},
    {FG_COLOR_ZERO,          OFFSET_COL(Display.bargraph, fg_color_zero)},
    {OUTLINE_COLOR,          OFFSET_COL(Display.bargraph, outline_color)},
};

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    u8 idx;
    struct display_settings *d = (struct display_settings *)user;
    int value_int = atoi(value);

    int assign_int(void* ptr, const struct struct_map *map, int map_size)
    {
        for(int i = 0; i < map_size; i++) {
            if(MATCH_KEY(map[i].str)) {
                int size = map[i].offset >> 13;
                int offset = map[i].offset & 0x1FFF;
                switch(size) {
                    case 0:
                        *((u8 *)((long)ptr + offset)) = value_int; break;
                    case 1:
                        *((u16 *)((long)ptr + offset)) = value_int; break;
                    case 2:
                        *((u16 *)((long)ptr + offset)) = get_color(value); break;
                    case 3:
                        *((u32 *)((long)ptr + offset)) = value_int; break;
                    case 6:
                        *((u8 *)((long)ptr + offset)) = FONT_GetFromString(value);
                }
                return 1;
            }
        }
        return 0;
    }

    if(MATCH_START(section, FONT) && strlen(section) > sizeof(FONT)) {
        for (idx = 0; idx < NUM_STR_ELEMS(FONT_VAL); idx++) {
            if (FONT_VAL[idx] && 0 == strcasecmp(section + sizeof(FONT), FONT_VAL[idx])) {
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
        if(MATCH_KEY("header_time")) {
#if HAS_RTC
            d->flags = SET_FLAG(d->flags, atoi(value), SHOW_TIME);
#endif
            return 1;
        }
        if(assign_int(&d->metrics, _secgeneral, MAPSIZE(_secgeneral)))
            return 1;
    }
    if(MATCH_START(section, "select")) {
        if(assign_int(d, _secselect, MAPSIZE(_secselect)))
            return 1;
    }
    if(MATCH_START(section, "keyboard")) {
        if(assign_int(&d->keyboard, _seckeybd, MAPSIZE(_seckeybd)))
            return 1;
    }
    if(MATCH_START(section, "listbox")) {
        if(assign_int(&d->listbox, _seclistbox, MAPSIZE(_seclistbox)))
            return 1;
    }
    if(MATCH_START(section, "scrollbar")) {
        if(assign_int(&d->scrollbar, _secscroll, MAPSIZE(_secscroll)))
            return 1;
    }
    if(MATCH_SECTION("xygraph")) {
        if(assign_int(&d->xygraph, _secxygraph, MAPSIZE(_secxygraph)))
            return 1;
    }
    for (idx = 0; idx < NUM_STR_ELEMS(BARGRAPH_VAL); idx++) {
        if(MATCH_SECTION(BARGRAPH_VAL[idx])) {
            struct disp_bargraph *graph;
            enum DispFlags flag;
            if (idx == 0) {
                graph = &d->bargraph;
                flag = BAR_TRANSPARENT;
            } else {
                graph = &d->trim;
                flag = TRIM_TRANSPARENT;
            }
            if(MATCH_KEY(IS_TRANSPARENT)) {
                d->flags = SET_FLAG(d->flags, value_int, flag);
                return 1;
            }
            if(MATCH_KEY(FG_COLOR)) {
                graph->fg_color_pos = graph->fg_color_neg = graph->fg_color_zero = get_color(value);
                return 1;
            }
            assign_int(graph, _secbargraph, MAPSIZE(_secbargraph));
            return 1;
        }
    }
    printf("Could not handle [%s] %s=%s\n", section, name, value);
    return 1;
}

u8 CONFIG_ReadDisplay()
{
    memset(&Display, 0, sizeof(Display));
    DEFAULT_FONT.font = 7;
    DEFAULT_FONT.font_color = 0xffff;
    char filename[] = "media/config.ini\0\0\0"; // placeholder for longer folder name
    #ifdef _DEVO12_TARGET_H_
    static u8 checked;
        if(!checked) {
            FILE *fh;
            fh = fopen("mymedia/config.ini", "r");
            if(fh) {
                sprintf(filename, "mymedia/config.ini");
                fclose(fh);
            }
            checked = 1;
        }
    #endif
    return CONFIG_IniParse(filename, ini_handler, (void *)&Display);
}
