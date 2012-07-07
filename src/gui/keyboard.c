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
#define ENABLE_GUIOBJECT
#include "gui.h"

guiObject_t *GUI_CreateKeyboard(enum KeyboardType type, char *text, u8 num_chars,
        void (*CallBack)(struct guiObject *obj, void *data), void *cb_data)
{
    struct guiObject   *obj = GUI_GetFreeObj();
    struct guiKeyboard *keyboard;
    struct guiBox      *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    keyboard = &obj->o.keyboard;
    obj->Type = Keyboard;
    OBJ_SET_USED(obj, 1);
    OBJ_SET_MODAL(obj, 1);
    connect_object(obj);

    keyboard = &obj->o.keyboard;
    keyboard->type = type;
    keyboard->last_coords.x = 0;
    keyboard->last_coords.y = 0;
    keyboard->text = text;
    keyboard->caps = 1;
    keyboard->num_chars = num_chars;
    keyboard->CallBack = CallBack;
    keyboard->cb_data = cb_data;
    
    return obj;
}

static u8 kb_draw_key(u16 x, u16 y, u16 width, u16 height, const u8 *str,
     u16 bg_color1, u16 bg_color2, u16 txt_color, struct touch *coords1, struct touch *coords2)
{
    u16 w, h;
    u16 bg_color;
    struct guiBox box;
    u8 draw = 0;
    #define X_SPACE 3
    #define Y_SPACE 7
    box.x = x + X_SPACE;
    box.y = y + Y_SPACE;
    box.width = width - 2 * X_SPACE;
    box.height = height - 2 * Y_SPACE;

    if(coords1 && coords_in_box(&box, coords1 ) && (! coords2 || ! coords_in_box(&box, coords2))) {
        draw = 2;
        bg_color = bg_color2;
    } else if(! coords1 && coords2 && coords_in_box(&box, coords2)) {
        draw = 1;
        bg_color = bg_color1;
    } else if(! coords2&& ! coords1) {
        draw = 1;
        bg_color = bg_color1;
    }
    /*
    printf("(%dx%dx%dx%d)", box.x, box.y, box.width, box.height);
    if(coords1)
        printf(" coords1: %dx%d", coords1->x, coords1->y);
    if(coords2)
        printf(" coords2: %dx%d", coords2->x, coords2->y);
    printf(" Draw: %d\n", draw);
    */
    if(! draw)
        return 0;
    LCD_FillRoundRect(box.x, box.y, box.width, box.height, 3, bg_color);
    LCD_GetStringDimensions(str, &w, &h);
    LCD_SetXY(x + (width - w) / 2, y + (height - h) / 2);
    LCD_SetFontColor(txt_color);
    LCD_PrintString((const char *)str);
    return draw;
}

static void kb_draw_text(const char *str)
{
    u16 w, h;
    LCD_GetCharDimensions('A', &w, &h);
    LCD_FillRoundRect(5, 2, 320 - 10, 24, 3, 0xFFFF);
    LCD_SetXY(10, (24 - h) / 2 + 2);
    LCD_SetFontColor(0x0000);
    LCD_PrintString(str);
}

void kb_update_string(struct guiKeyboard *keyboard, u8 ch)
{
    u8 len = strlen(keyboard->text);
    if(ch == '') {
        if (len > 0) {
            keyboard->text[len - 1] = 0;
            kb_draw_text(keyboard->text);
        }
        return;
    }
    if (len >= keyboard->num_chars) 
        return;
    if(! keyboard->caps && ch >= 'A' && ch <= 'Z') {
        ch = (ch - 'A') + 'a';
    }
    keyboard->caps = 0;
    keyboard->text[len] = ch;
    keyboard->text[len+1] = 0;
    kb_draw_text(keyboard->text);
}
u8 GUI_DrawKeyboard(struct guiObject *obj, struct touch *coords)
{
#define Y_OFFSET 30
#define KEY_H 52
#define KEY_W1 32
#define KEY_W2 (32 + 12)
#define KEY_W3 (32 + 18)
#define BG1 RGB888_to_RGB565(0xe2, 0xe4, 0xe6)
#define BG2 RGB888_to_RGB565(0x8f, 0x95, 0xa1)
#define BG3 RGB888_to_RGB565(0x32, 0x70, 0xdf)
#define TXT1 0x0000
#define TXT2 0xffff
//#define BG3
    u8 row, i;
    struct guiKeyboard *keyboard = &obj->o.keyboard;
    struct touch *last_coords;
    u8 pressed;
    const u8 r1[] = "QWERTYUIOP";
    const u8 r2[] =  "ASDFGHJKL";
    const u8 r3[] =   "ZXCVBNM";
    const u8 caps[] = "CAPS";
    const u8 del[] = "DEL";
    const u8 num[] = ".?123";
    const u8 space[] = "SPACE";
    const u8 done[] = "DONE";
    u8 draw = 0;
    u8 ch[2];
    ch[1] = 0;
    if(keyboard->last_coords.x != 0 || keyboard->last_coords.y != 0) {
        last_coords = &keyboard->last_coords;
    } else {
        last_coords = NULL;
    }
    kb_draw_text(keyboard->text);
    for(row = 0; row < 3; row++) {
        const u8 *ptr;
        u8 num_chars;
        u16 x_off, y_off;
        if(row == 0) {
             ptr = r1; num_chars = 10;
        } else if(row == 1) {
             ptr = r2; num_chars = 9;
        } else {
             ptr = r3; num_chars = 7;
        }
        x_off = (320 - KEY_W1 * num_chars) / 2;
        y_off = Y_OFFSET + KEY_H * row;
        for(i = 0; i < num_chars; i++) {
            ch[0] = ptr[i];
            pressed = kb_draw_key(x_off + i * KEY_W1, y_off, KEY_W1, KEY_H, ch, BG1, BG2, TXT1, coords, last_coords);
            draw |= pressed;
            if (2 == pressed) {
                kb_update_string(keyboard, ptr[i]);
            }
        }
    }
    /* CAPS */
    pressed = kb_draw_key(0, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H, caps, BG2, BG3, TXT2, coords, last_coords);
    draw |= pressed;
    if (pressed == 2) {
        keyboard->caps ^= 1;
    }
    /* DEL */
    pressed = kb_draw_key(320 - KEY_W2, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H, del, BG2, BG3, TXT2, coords, last_coords);
    draw |= pressed;
    if (pressed == 2) {
        kb_update_string(keyboard, '');
    }
    /* NUMPAD */
    pressed = kb_draw_key(0, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H, num, BG2, BG3, TXT2, coords, last_coords);
    draw |= pressed;
    if (pressed == 2) {
        //redraw numpad here
    }
    /* SPACE */
    pressed = kb_draw_key(KEY_W3, Y_OFFSET + KEY_H * 3, 320 - 2 * KEY_W3, KEY_H, space, BG1, BG2, TXT1, coords, last_coords);
    draw |= pressed;
    if (pressed == 2) {
        kb_update_string(keyboard, ' ');
    }

    /* DONE */
    pressed = kb_draw_key(320 - KEY_W3, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H, done, BG3, BG3, TXT2, coords, last_coords);
    draw |= pressed;
    if (pressed == 2) {
        //Done
        if (keyboard->CallBack) {
            keyboard->CallBack(obj, keyboard->cb_data);
        }
    }
    if(draw) {
        OBJ_SET_DIRTY(obj, 1);
        if(coords) {
            keyboard->last_coords = *coords;
        }  
        else if(last_coords)
            keyboard->last_coords.x = keyboard->last_coords.y = 0;
    }
    return draw;
}
