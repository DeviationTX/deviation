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
#include "config.h"
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
#define SET_FLAG(var, value, flag) ((value) ? ((var) | (flag)) : ((var) & ~(flag)))

struct display_settings Display;

static const struct struct_map _seclabel[] =
{
    {FONT,                  OFFSET_FON(struct LabelDesc, font)},
    {FONT_COLOR,            OFFSET_COL(struct LabelDesc, font_color)},
    {BG_COLOR,              OFFSET_COL(struct LabelDesc, fill_color)},
    {OUTLINE_COLOR,         OFFSET_COL(struct LabelDesc, outline_color)},
    {ALIGN,                 OFFSET_STRLIST(struct LabelDesc, align, ALIGN_VAL, ARRAYSIZE(ALIGN_VAL))},
    {BOX,                   OFFSET_STRLIST(struct LabelDesc, style, BOX_VAL, ARRAYSIZE(BOX_VAL))}
};

static const struct struct_map _secgeneral[] =
{
    {"header_height",        OFFSET(struct disp_metrics, header_height)},
    {"header_widget_height", OFFSET(struct disp_metrics, header_widget_height)},
    {"line_height",          OFFSET(struct disp_metrics, line_height)},
    {"line_space",           OFFSET(struct disp_metrics, line_space)},
};
static const struct struct_map _secselect[] =
{
    {"width",                OFFSET(struct display_settings, select_width)},
    {COLOR,                  OFFSET_COL(struct display_settings, select_color)},
};
static const struct struct_map _seckeybd[] =
{
    {FONT,                   OFFSET_FON(struct disp_keyboard, font)},
    {"bg_key1",              OFFSET_COL(struct disp_keyboard, bg_key1)},
    {"fg_key1",              OFFSET_COL(struct disp_keyboard, fg_key1)},
    {"bg_key2",              OFFSET_COL(struct disp_keyboard, bg_key2)},
    {"fg_key2",              OFFSET_COL(struct disp_keyboard, fg_key2)},
    {"bg_key3",              OFFSET_COL(struct disp_keyboard, bg_key3)},
    {"fg_key3",              OFFSET_COL(struct disp_keyboard, fg_key3)},
    {BG_COLOR,               OFFSET_COL(struct disp_keyboard, fill_color)},
};
static const struct struct_map _seclistbox[] =
{
    {FONT,                   OFFSET_FON(struct disp_listbox, font)},
    {BG_COLOR,               OFFSET_COL(struct disp_listbox, bg_color)},
    {FG_COLOR,               OFFSET_COL(struct disp_listbox, fg_color)},
    {"bg_select",            OFFSET_COL(struct disp_listbox, bg_select)},
    {"fg_select",            OFFSET_COL(struct disp_listbox, fg_select)},
};
static const struct struct_map _secscroll[] =
{
    {BG_COLOR,               OFFSET_COL(struct disp_scrollbar, bg_color)},
    {FG_COLOR,               OFFSET_COL(struct disp_scrollbar, fg_color)},
};
static const struct struct_map _secxygraph[] =
{
    {BG_COLOR,               OFFSET_COL(struct disp_xygraph, bg_color)},
    {FG_COLOR,               OFFSET_COL(struct disp_xygraph, fg_color)},
    {XY_AXIS_COLOR,          OFFSET_COL(struct disp_xygraph, axis_color)},
    {XY_GRID_COLOR,          OFFSET_COL(struct disp_xygraph, grid_color)},
    {XY_POINT_COLOR,         OFFSET_COL(struct disp_xygraph, point_color)},
    {OUTLINE_COLOR,          OFFSET_COL(struct disp_xygraph, outline_color)},
};
static const struct struct_map _secbargraph[] =
{
    {BG_COLOR,               OFFSET_COL(struct disp_bargraph, bg_color)},
    {FG_COLOR_POS,           OFFSET_COL(struct disp_bargraph, fg_color_pos)},
    {FG_COLOR_NEG,           OFFSET_COL(struct disp_bargraph, fg_color_neg)},
    {FG_COLOR_ZERO,          OFFSET_COL(struct disp_bargraph, fg_color_zero)},
    {OUTLINE_COLOR,          OFFSET_COL(struct disp_bargraph, outline_color)},
};
static const struct struct_map _secbargraph_legacy[] =
{
    {FG_COLOR,           OFFSET_COL(struct disp_bargraph, fg_color_pos)},
};
#if (LCD_WIDTH == 480) || (LCD_WIDTH == 320)
static const struct struct_map _secbackground[] =
{
    {"drawn_background",     OFFSET(struct disp_background, drawn_background)},
    {"bg_color",             OFFSET_COL(struct disp_background, bg_color)},
    {"hd_color",             OFFSET_COL(struct disp_background, hd_color)},
};
#endif

static int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    u8 idx;
    struct display_settings *d = (struct display_settings *)user;
    int value_int = atoi(value);

    if(MATCH_START(section, FONT) && strlen(section) > sizeof(FONT)) {
        for (idx = 0; idx < ARRAYSIZE(FONT_VAL); idx++) {
            if (FONT_VAL[idx] && 0 == strcasecmp(section + sizeof(FONT), FONT_VAL[idx])) {
                return assign_int(&d->font[idx], _seclabel, ARRAYSIZE(_seclabel), name, value);
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
        if (assign_int(&d->metrics, _secgeneral, ARRAYSIZE(_secgeneral), name, value))
            return 1;
    }
    if(MATCH_START(section, "select")) {
        if (assign_int(d, _secselect, ARRAYSIZE(_secselect), name, value))
            return 1;
    }
    if(MATCH_START(section, "keyboard")) {
        if (assign_int(&d->keyboard, _seckeybd, ARRAYSIZE(_seckeybd), name, value))
            return 1;
    }
    if(MATCH_START(section, "listbox")) {
        if (assign_int(&d->listbox, _seclistbox, ARRAYSIZE(_seclistbox), name, value))
            return 1;
    }
    if(MATCH_START(section, "scrollbar")) {
        if (assign_int(&d->scrollbar, _secscroll, ARRAYSIZE(_secscroll), name, value))
            return 1;
    }
    if(MATCH_SECTION("xygraph")) {
        if (assign_int(&d->xygraph, _secxygraph, ARRAYSIZE(_secxygraph), name, value))
            return 1;
    }
#if (LCD_WIDTH == 480) || (LCD_WIDTH == 320)
    if(MATCH_SECTION("background")) {
        if (assign_int(&d->background, _secbackground, ARRAYSIZE(_secbackground), name, value))
            return 1;
    }
#endif
    for (idx = 0; idx < ARRAYSIZE(BARGRAPH_VAL); idx++) {
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

            if (assign_int(graph, _secbargraph, ARRAYSIZE(_secbargraph), name, value))
                return 1;

            if (assign_int(graph, _secbargraph_legacy, ARRAYSIZE(_secbargraph_legacy), name, value)) {
                graph->fg_color_neg = graph->fg_color_zero = graph->fg_color_pos;
                return 1;
            }

            return 1;
        }
    }
    printf("Could not handle [%s] %s=%s\n", section, name, value);
    return 1;
}

static void convert_legacy()
{
    for (int i = 0; i < NUM_LABELS; i++) {
        struct LabelDesc *label = &Display.font[i];
        // For compatibility reasons,
        // use the old alignment values as default
        // if the new one is not yet set:
        if (label->align == 0) {
            switch (label->style) {
                case LABEL_CENTER: label->align = ALIGN_CENTER; break;
                case LABEL_LEFT:   label->align = ALIGN_LEFT;   break;
                case LABEL_RIGHT:  label->align = ALIGN_RIGHT;  break;
                default: break;
            }
        }
    }
}

u8 CONFIG_ReadDisplay()
{
    memset(&Display, 0, sizeof(Display));
    DEFAULT_FONT.font = 7;
    DEFAULT_FONT.font_color = 0xffff;
    #ifdef _DEVO12_TARGET_H_
    static char filename[] = "media/config.ini\0\0\0"; // placeholder for longer folder name
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
    #else
    char filename[] = "media/config.ini";
    #endif
    if (CONFIG_IniParse(filename, ini_handler, (void *)&Display)) {
        convert_legacy();
        return 1;
    }

    return 0;
}
