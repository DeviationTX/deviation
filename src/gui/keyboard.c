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
#include <stdlib.h>

#include "common.h"
#include "music.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

#include "_keyboard.c"
#define FLAG_CAPS 0x01
#define FLAG_BUTTON 0x02
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
          "-/:#()$&@\"",
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

guiObject_t *GUI_CreateKeyboard(guiKeyboard_t *keyboard, enum KeyboardType type, char *text, s32 max_size,
        void (*CallBack)(struct guiObject *obj, void *data), void *cb_data)
{
    struct guiObject   *obj = (guiObject_t *)keyboard;
    CLEAR_OBJ(keyboard);

    obj->Type = Keyboard;
    OBJ_SET_MODAL(obj, 1);
    connect_object(obj);

    keyboard->type = type;
    keyboard->text = text;
    keyboard->flags = FLAG_BUTTON;
    keyboard->max_size = max_size;
    keyboard->last_row = 0;
    keyboard->last_col = 0;
    keyboard->lastchar = array[keyboard->type][0][0];
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

    LCD_SetFont(Display.keyboard.font);
    LCD_GetCharDimensions('A', &w, &h);
    LCD_FillRoundRect(TEXTBOX_X_OFFSET, TEXTBOX_Y_OFFSET,
                      LCD_WIDTH - 2 * TEXTBOX_X_OFFSET,
                      TEXTBOX_HEIGHT,
                      TEXTBOX_ROUND,
                      TEXTBOX_BG_COLOR);  // clear the backgroup firstly
    if(TEXTBOX_OUTLINE)
        LCD_DrawRoundRect(TEXTBOX_X_OFFSET, TEXTBOX_Y_OFFSET,
                          LCD_WIDTH - 2 * TEXTBOX_X_OFFSET,
                          TEXTBOX_HEIGHT,
                          TEXTBOX_ROUND,
                          TEXTBOX_OUTLINE);
    LCD_SetXY(TEXTBOX_X_OFFSET + 2, (TEXTBOX_HEIGHT - h) / 2 + TEXTBOX_Y_OFFSET);
    LCD_SetFontColor(TEXTBOX_COLOR);
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
    if (keyboard->type == KEYBOARD_NUM) {
        s32 val = atoi(keyboard->text);
        val = val * 10 + (ch - '0');
        if (val > keyboard->max_size)
            return;
        sprintf(keyboard->text, "%d", (int)val);
        kb_draw_text(keyboard->text);
        return;
    }
    if (len >= keyboard->max_size) {
        MUSIC_Play(MUSIC_MAXLEN);
        return;
    }
    if(! (keyboard->flags & FLAG_CAPS) && ch >= 'A' && ch <= 'Z') {
        ch = (ch - 'A') + 'a';
    }
    keyboard->text[len] = ch;
    keyboard->text[len+1] = 0;
    kb_draw_text(keyboard->text);
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
            bg_color = Display.keyboard.fg_key3;
            fg_color = Display.keyboard.bg_key3;
        } else {
            bg_color = Display.keyboard.bg_key3;
            fg_color = Display.keyboard.fg_key3;
        }
    } else if (c < ' ') { //CAPS, DEL, NUMPAD
        if      (c == '\x09') str = caps_str;
        else if (c == '\x08') str = del_str;
        else if (c == '\x01') str = mix_str;
        else if (c == '\x02') str = char_str;

        if (pressed) {
            bg_color = Display.keyboard.fg_key2;
            fg_color = Display.keyboard.bg_key2;
        } else {
            bg_color = Display.keyboard.bg_key2;
            fg_color = Display.keyboard.fg_key2;
        }
    } else {
        ch[0] = c;
        ch[1] = 0;
        str = ch;
        if (pressed) {
            bg_color = Display.keyboard.fg_key1;
            fg_color = Display.keyboard.bg_key1;
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
    LCD_SetFont(Display.keyboard.font);
    _draw_key_bg(box, pressed, bg_color);
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
    char lastchar = keyboard->lastchar;
    struct guiBox box;
    const char * const *keys = array[keyboard->type];

    u8 row, i;
    char ch = '\0';
    for(row = 0; row < 4; row++) {
        const char *ptr = keys[row];
        u8 first = get_first_char(ptr);
        u8 last  = get_last_char(ptr);
        u8 num_chars = 1 + last - first;
        u16 x_off = (LCD_WIDTH - KEY_W1 * num_chars) / 2;
        u16 y_off = Y_OFFSET + KEY_H * row;

        for(i = 0; i < strlen(ptr); i++) {
            char c = ptr[i];
            if (c == '\x09') { //CAPS
                _make_box(&box, 0, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H);
            } else if (c == '\x08') { //DEL
                _make_box(&box, LCD_WIDTH - KEY_W2 -1, Y_OFFSET + KEY_H * 2, KEY_W2, KEY_H);
            } else if (c == '\x01' || c == '\x02') { //change KB type
                _make_box(&box, 0, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H);
            } else if (c == '\x06') { //DONE
                _make_box(&box, LCD_WIDTH - KEY_W3 -1, Y_OFFSET + KEY_H * 3, KEY_W3, KEY_H);
            } else if (c == ' ') { //SPACE
                _make_box(&box, KEY_W3, Y_OFFSET + KEY_H * 3, LCD_WIDTH - 2 * KEY_W3 -1, KEY_H);
            } else {
                _make_box(&box, x_off + (i - first) * KEY_W1, y_off, KEY_W1, KEY_H);
            }
            if (cmd == KB_DRAW) {
                if ((c == '\x01' || c == '\x02') && (lastchar == '\x01' || lastchar == '\x02'))
                    keyboard->lastchar = c;
                u8 pressed = c == keyboard->lastchar;
                kb_draw_key(&box, set_case(c, keyboard->flags & FLAG_CAPS), pressed);
            } else if (cmd == KB_RELEASE || cmd == KB_PRESS) {
                if(c == lastchar) {
                    kb_draw_key(&box, set_case(c, keyboard->flags & FLAG_CAPS), cmd == KB_PRESS);
                    return '\0';
                }
            } else if(cmd == KB_COMPARE_AND_PRESS) {
                if (coords_in_box(&box, coords)) {
                    kb_draw_key(&box, set_case(c, keyboard->flags & FLAG_CAPS), 1);
                    ch = c;
                } else if(c == lastchar) {
                    kb_draw_key(&box, set_case(c, keyboard->flags & FLAG_CAPS), 0);
                }
            }
        }
    }
    return ch;
}

void GUI_DrawKeyboard(struct guiObject *obj)
{
#define FILL  Display.keyboard.fill_color  //RGB888_to_RGB565(0x6b, 0x73, 0x80)
    struct guiKeyboard *keyboard = (struct guiKeyboard *)obj;

    LCD_Clear(FILL);
    keyboard_cmd(KB_DRAW, keyboard, NULL);
    kb_draw_text(keyboard->text);
    return;
}

u8 GUI_TouchKeyboard(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiKeyboard *keyboard = (struct guiKeyboard *)obj;
    if (coords && ! press_type && (! keyboard->lastchar || (keyboard->flags & FLAG_BUTTON))) {
        char c = keyboard_cmd(KB_COMPARE_AND_PRESS, keyboard, coords);
        keyboard->flags &= ~FLAG_BUTTON;
        keyboard->lastchar = c;
    } else if (press_type == -1 && keyboard->lastchar && ! (keyboard->flags & FLAG_BUTTON)) {
        //Key Release
        if (keyboard->lastchar == '\x06') { //DONE
            keyboard->lastchar = '\0';
            if (keyboard->CallBack)
                keyboard->CallBack(obj, keyboard->cb_data);
            //After DONE it is possible that obj and keyboard are invalid
            return 1;
        } else if (keyboard->lastchar == '\x09') { //CAPS
            keyboard->flags ^= FLAG_CAPS;
            keyboard_cmd(KB_DRAW, keyboard, NULL);
        } else if (keyboard->lastchar == '\x01' || keyboard->lastchar == '\x02') { //Numpad
            keyboard->type = keyboard->type == KEYBOARD_ALPHA ? KEYBOARD_SPECIAL : KEYBOARD_ALPHA;
            OBJ_SET_DIRTY(obj, 1);
        } else {
            keyboard_cmd(KB_RELEASE, keyboard, NULL);
            kb_update_string(keyboard, keyboard->lastchar);
        }
        keyboard->lastchar = '\0';
    } else if (press_type == 1 && keyboard->lastchar == '\x08') {
        //DEL Long Press erases whole string
        keyboard->text[0] = '\0';
        kb_draw_text(keyboard->text);
    }
    return 1;
}

static void navigate_item(struct guiKeyboard *keyboard, short leftRight, short upDown) {
    const char * const *keys = array[keyboard->type];
    MUSIC_Play(MUSIC_KEY_PRESSING);
    short i = keyboard->last_row;
    short j = 0;
    u8 col_len = 0;
    if (! (keyboard->flags & FLAG_BUTTON)) {
        keyboard->lastchar = array[keyboard->type][0][0];
        keyboard->flags |= FLAG_BUTTON;
    }
    if (leftRight != 0) {
        const char *ptr = keys[i];
        col_len = strlen(ptr);
        if (i < 3 || keyboard->type == KEYBOARD_NUM) {
            j = keyboard->last_col;
            j += leftRight;
            if (j < 0) j = col_len -1;
            if (j >= col_len) j = 0;
            keyboard->last_col = j;
        } else {  // when row = 3, don't keep track of its col
            for (j = 0; j < col_len; j++) {
                if (ptr[j] == keyboard->lastchar) {
                    break;
                }
            }
            j += leftRight;
            if (j < 0) j = col_len -1;
            if (j >= col_len) j = 0;
        }
    } else {
        i += upDown;
        if (i < 0) i = 3;
        if (i >= 4) i = 0;
        keyboard->last_row = i;
        j = keyboard->last_col;
        if (j >= (short)strlen(keys[i])) {
            j = strlen(keys[i]) - 1;
        }
    }
    keyboard_cmd(KB_RELEASE, keyboard, NULL);
    keyboard->lastchar = keys[i][j];
    keyboard_cmd(KB_PRESS, keyboard, NULL);
}

static u8 press_cb(u32 button, u8 flags, void *data)
{
    struct guiObject *obj = (struct guiObject *)data;
    struct guiKeyboard *keyboard = (struct guiKeyboard *)obj;
    (void)data;
    if (flags & BUTTON_PRESS || flags & BUTTON_LONGPRESS) {
        if ( flags & BUTTON_LONGPRESS && CHAN_ButtonIsPressed(button, BUT_ENTER) && keyboard->lastchar == '\x08') {
            //DEL Long Press erases whole string
            keyboard->text[0] = '\0';
            kb_draw_text(keyboard->text);
        }
        else if (CHAN_ButtonIsPressed(button, BUT_EXIT)) { // allow user to press the EXT key to discard changes
            if (keyboard->CallBack) {
                if (keyboard->cb_data != NULL) {
                    int *result = (int *) keyboard->cb_data;
                    *result = 0;
                }
                BUTTON_UnregisterCallback(&keyboard->action);
                keyboard->CallBack(obj, keyboard->cb_data );
            }
            //After DONE it is possible that obj and keyboard are invalid
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            navigate_item(keyboard, 1 , 0);
        }  else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
            navigate_item(keyboard, -1, 0);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_item(keyboard, 0 , -1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_item(keyboard, 0, 1);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            if (keyboard->lastchar == '\x09') { //CAPS
                keyboard->flags ^= FLAG_CAPS;
                keyboard_cmd(KB_DRAW, keyboard, NULL);
            } else if (keyboard->lastchar == '\x01' || keyboard->lastchar == '\x02') { //Numpad
                keyboard->type = keyboard->type == KEYBOARD_ALPHA ? KEYBOARD_SPECIAL : KEYBOARD_ALPHA;
                OBJ_SET_DIRTY(obj, 1);
            } else if (keyboard->lastchar == '\x06') { //DONE
                if (keyboard->CallBack) {
                    if (keyboard->cb_data != NULL) {
                        int *result = (int *) keyboard->cb_data;
                        if (*result <= 0) // to avoid loosing the channel number when renaming virtual channels on b/w-screens
                        	*result = 1;
                    }
                    BUTTON_UnregisterCallback(&keyboard->action);
                    keyboard->CallBack(obj, keyboard->cb_data );
                }
                //After DONE it is possible that obj and keyboard are invalid
            } else {
                kb_update_string(keyboard, keyboard->lastchar);
            }
        }
        return 1;
    }
    return 1;
}

