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
#include "config/display.h"

enum DrawCmds {
    KB_DRAW,
    KB_COMPARE_AND_PRESS,
    KB_PRESS,
    KB_RELEASE,
};


static u8 press_cb(u32 button, u8 flags, void *data);
static const char * const alpha[] = {
          "QWERTYUIOP",
          "ASDFGHJKL",
    "\x09" "ZXCVBNM" "\x08",
    "\x01" " " "\x06",
};
static const char * const mixed[] = {
          "1234567890",
          "-/:;()$&@\"",
            ".,?!'"    "\x08",
       "\x02" " "      "\x06",
};
static const char * const numpad[] = {
          "123",
          "456",
          "789" "\x08",
           "0" "\x06",
};
static const char * const * const array[] = { alpha, numpad, mixed };

guiObject_t *GUI_CreateKeyboard(enum KeyboardType type, char *text, u8 num_chars,
        void (*CallBack)(struct guiObject *obj, void *data), void *cb_data)
{
    struct guiObject   *obj = GUI_GetFreeObj();
    struct guiKeyboard *keyboard;

    if (obj == NULL)
        return NULL;

    obj->Type = Keyboard;
    OBJ_SET_MODAL(obj, 1);
    connect_object(obj);

    keyboard = &obj->o.keyboard;
    keyboard->type = type;
    keyboard->lastchar = '\0';
    keyboard->text = text;
    keyboard->caps = 0;
    keyboard->num_chars = num_chars;
    BUTTON_RegisterCallback(&keyboard->action,
         CHAN_ButtonMask(BUT_LEFT)
         | CHAN_ButtonMask(BUT_RIGHT)
         | CHAN_ButtonMask(BUT_UP)
         | CHAN_ButtonMask(BUT_DOWN)
         | CHAN_ButtonMask(BUT_ENTER)
         | CHAN_ButtonMask(BUT_EXIT),
         BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_PRIORITY,
         press_cb, obj);
    keyboard->CallBack = CallBack;
    keyboard->cb_data = cb_data;
    
    return obj;
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
    if(ch == '\x08') {
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
    keyboard->text[len] = ch;
    keyboard->text[len+1] = 0;
    kb_draw_text(keyboard->text);
}

static void make_box(struct guiBox *box, u16 x, u16 y, u16 width, u16 height)
{
    #define X_SPACE 3
    #define Y_SPACE 7
    box->x = x + X_SPACE;
    box->y = y + Y_SPACE;
    box->width = width - 2 * X_SPACE;
    box->height = height - 2 * Y_SPACE;
}

static void kb_draw_key(struct guiBox *box, char c, u8 pressed)
{
    u16 w, h;
    u16 bg_color;
    u16 fg_color;
    char ch[2];
    const char *str = NULL;
    if ( c == '\x06') { //DONE
        str = "DONE";
        if (pressed) {
            bg_color = Display.keyboard.bg_key2;
            fg_color = Display.keyboard.fg_key2;
        } else {
            bg_color = Display.keyboard.bg_key3;
            fg_color = Display.keyboard.fg_key3;
        }
    } else if (c < ' ') { //CAPS, DEL, NUMPAD
        if      (c == '\x09') str = "CAPS";
        else if (c == '\x08') str = "DEL";
        else if (c == '\x01') str = ".?123";
        else if (c == '\x02') str = "ABC";
        if (pressed) {
            bg_color = Display.keyboard.bg_key3;
            fg_color = Display.keyboard.fg_key3;
        } else {
            bg_color = Display.keyboard.bg_key2;
            fg_color = Display.keyboard.fg_key2;
        }
    } else {
        ch[0] = c;
        str = ch;
        if (pressed) {
            bg_color = Display.keyboard.bg_key2;
            fg_color = Display.keyboard.fg_key2;
        } else {
            bg_color = Display.keyboard.bg_key1;
            fg_color = Display.keyboard.fg_key1;
        }
    }
    /*
    printf("(%dx%dx%dx%d)", box->x, box->y, box->width, box->height);
    if(coords1)
        printf(" coords1: %dx%d", coords1->x, coords1->y);
    if(coords2)
        printf(" coords2: %dx%d", coords2->x, coords2->y);
    printf(" Draw: %d\n", draw);
    */
    LCD_FillRoundRect(box->x, box->y, box->width, box->height, 3, bg_color);
    LCD_GetStringDimensions((const u8 *)str, &w, &h);
    LCD_SetXY(box->x + (box->width - w) / 2, box->y + (box->height - h) / 2);
    LCD_SetFontColor(fg_color);
    LCD_PrintString((const char *)str);
}

static u8 get_first_char(const char *str)
{
    const char *ptr = str;
    while(*ptr < 32)
        ptr++;
    return ptr - str;
}
static u8 get_last_char(const char *str)
{
    const char *ptr = str + strlen(str);
    while(*ptr < 32)
        ptr--;
    return ptr - str;
}

static char set_case(char c, u8 caps)
{
    if (! caps && c >= 'A' && c <= 'Z')
        return (c - 'A' + 'a');
    return c;
}

static char keyboard_cmd(enum DrawCmds cmd, struct guiKeyboard *keyboard, struct touch *coords)
{
#define Y_OFFSET 30
#define KEY_H 52
#define KEY_W1 32
#define KEY_W2 (32 + 12)
#define KEY_W3 (32 + 18)
    char lastchar = keyboard->lastchar;
    struct guiBox box;
    const char * const *keys = array[keyboard->type];

    u8 row, i;
    for(row = 0; row < 4; row++) {
        const char *ptr = keys[row];
        u8 first = get_first_char(ptr);
        u8 last  = get_last_char(ptr);
        u8 num_chars = 1 + last - first;
        u16 x_off = (320 - KEY_W1 * num_chars) / 2;
        u16 y_off = Y_OFFSET + KEY_H * row;

        for(i = 0; i < strlen(ptr); i++) {
            char c = ptr[i];
            if (c == '\x09') { //CAPS
                make_box(&box, 0, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H);
            } else if (c == '\x08') { //DEL
                make_box(&box, 320 - KEY_W2, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H);
            } else if (c == '\x01' || c == '\x02') { //change KB type
                make_box(&box, 0, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H);
            } else if (c == '\x06') { //DONE
                make_box(&box, 320 - KEY_W3, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H);
            } else if (c == ' ') { //SPACE
                make_box(&box, KEY_W3, Y_OFFSET + KEY_H * 3, 320 - 2 * KEY_W3, KEY_H);
            } else {
                make_box(&box, x_off + (i - first) * KEY_W1, y_off, KEY_W1, KEY_H);
            }
            if (cmd == KB_DRAW) {
                //CAPS is a toggle, so handle differently
                u8 pressed = (c == '\x09') ? keyboard->caps : (c == lastchar);
                kb_draw_key(&box, set_case(c, keyboard->caps), pressed);
            } else if (cmd == KB_RELEASE || cmd == KB_PRESS) {
                if(c == lastchar) {
                    kb_draw_key(&box, set_case(c, keyboard->caps), cmd == KB_PRESS);
                    return '\0';
                }
            } else if(cmd == KB_COMPARE_AND_PRESS) {
                if (coords_in_box(&box, coords)) {
                    kb_draw_key(&box, set_case(c, keyboard->caps), 1);
                    return c;
                }
            }
        }
    }
    return '\0';
}

void GUI_DrawKeyboard(struct guiObject *obj)
{
#define FILL  Display.keyboard.fill_color  //RGB888_to_RGB565(0x6b, 0x73, 0x80)
    struct guiKeyboard *keyboard = &obj->o.keyboard;

    LCD_FillRect(0, 0, 320, 240, FILL);
    keyboard_cmd(KB_DRAW, keyboard, NULL);
    kb_draw_text(keyboard->text);
    return;
}

u8 GUI_TouchKeyboard(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiKeyboard *keyboard = &obj->o.keyboard;
    if (coords && ! press_type && ! keyboard->lastchar) {
        char c = keyboard_cmd(KB_COMPARE_AND_PRESS, keyboard, coords);
        keyboard->lastchar = c;
    } else if (press_type == -1 && keyboard->lastchar) {
        //Key Release
        if (keyboard->lastchar == '\x09') { //CAPS
            keyboard->caps = ! keyboard->caps;
            keyboard_cmd(KB_DRAW, keyboard, NULL);
        } else if (keyboard->lastchar == '\x01' || keyboard->lastchar == '\x02') { //Numpad
            keyboard->type = keyboard->type == KEYBOARD_ALPHA ? KEYBOARD_SPECIAL : KEYBOARD_ALPHA;
            OBJ_SET_DIRTY(obj, 1);
        } else {
            keyboard_cmd(KB_RELEASE, keyboard, NULL);
            kb_update_string(keyboard, keyboard->lastchar);
        }
        if (keyboard->lastchar == '\x06') { //DONE
            keyboard->lastchar = '\0';
            if (keyboard->CallBack)
                keyboard->CallBack(obj, keyboard->cb_data);
            //After DONE it is possible that obj and keyboard are invalid
            return 1;
        }
        keyboard->lastchar = '\0';
    } else if (press_type == 1 && keyboard->lastchar == '\x08') {
        //DEL Long Press erases whole string
        keyboard->text[0] = '\0';
        kb_draw_text(keyboard->text);
    }
    return 1;
}

static void find_row_col(const char * const *keys, char c, u8 *row, u8 *col)
{
    u8 i, j;
    for(j = 0; j < 4; j++) {
        for(i = 0; i < strlen(keys[j]); i++) {
            if(keys[j][i] == c) {
                *row = j;
                *col = i;
                return;
            }
        }
    }
}

static u8 calc_column(const char *old_row, const char *new_row, s8 col)
{
    u8 old_first = get_first_char(old_row);
    u8 old_last  = get_last_char(old_row);
    u8 old_len = 1 + old_last - old_first;
    u8 new_first = get_first_char(new_row);
    u8 new_last  = get_last_char(new_row);
    u8 new_len = 1 + new_last - new_first;
    col -= old_first + old_len / 2;
    col = new_first + new_len / 2 + col;
    if (col < new_first) {
        col = new_first;
    } else if (col > new_last) {
        col = new_last;
    }
    return col;
}

static u8 press_cb(u32 button, u8 flags, void *data)
{
    struct guiObject *obj = (struct guiObject *)data;
    struct guiKeyboard *keyboard = &obj->o.keyboard;
    const char * const *keys;
    if (CHAN_ButtonIsPressed(button, BUT_EXIT))
        return 1;
    if(flags & BUTTON_PRESS) {
        if(! keyboard->lastchar) {
            keyboard->lastchar = keyboard->type == KEYBOARD_ALPHA ? 'Q' : '1';
            keyboard_cmd(KB_PRESS, keyboard, NULL);
            return 1;
        }
        u8 row, col;
        keys = array[keyboard->type];
        find_row_col(keys, keyboard->lastchar, &row, &col);
        if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            if (col < strlen(keys[row]) - 1)
                col++;
        } else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
            if (col > 0)
                col--;
        } else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            if (row < 3) {
                col = calc_column(keys[row], keys[row+1], col);
                row++;
            }
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            if (row > 0) {
                col = calc_column(keys[row], keys[row-1], col);
                row--;
            }
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            if (keyboard->lastchar == '\x09') { //CAPS
                keyboard->caps = ! keyboard->caps;
                keyboard_cmd(KB_DRAW, keyboard, NULL);
            } else if (keyboard->lastchar == '\x01' || keyboard->lastchar == '\x02') { //Numpad
                keyboard->type = keyboard->type == KEYBOARD_ALPHA ? KEYBOARD_SPECIAL : KEYBOARD_ALPHA;
                OBJ_SET_DIRTY(obj, 1);
            } else {
                kb_update_string(keyboard, keyboard->lastchar);
            }
            if (keyboard->lastchar == '\x06') { //DONE
                if (keyboard->CallBack)
                    keyboard->CallBack(obj, keyboard->cb_data);
                    //After DONE it is possible that obj and keyboard are invalid
            }
            return 1;
        }
        if (keys[row][col] != keyboard->lastchar) {
            if (keyboard->lastchar)
                keyboard_cmd(KB_RELEASE, keyboard, NULL);
            keyboard->lastchar = keys[row][col];
            keyboard_cmd(KB_PRESS, keyboard, NULL);
            return 1;
        }
    }
    return 1;
}

