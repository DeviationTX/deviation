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
#include "buttons.h"

static buttonAction_t *buttonHEAD = NULL;
u32 last_buttons;
u8 long_press;

static buttonAction_t *buttonPressed = NULL;
u16 button_time;

u8 BUTTON_RegisterCallback(buttonAction_t *action, u32 button, u8 flags,
                 u8 (*callback)(u32 button, u8 flags, void *data), void *data)
{
    buttonAction_t *ptr = buttonHEAD;

    if (! (flags & BUTTON_PRIORITY)) {
        while(ptr) {
            if((ptr->button & button) && (ptr->flags & flags) & ! (ptr->flags & BUTTON_PRIORITY)) {
                printf("Button %08x with flags %d is already assigned\n", (unsigned int)button, flags);
                memset(action, 0, sizeof(buttonAction_t));
                return 0;
            }
            if (! ptr->next)
                break;
            ptr = ptr->next;
        }
        if (ptr)
            ptr->next = action;
        else
            buttonHEAD = action;
        action->next = NULL;
    } else {
       action->next = buttonHEAD;
       buttonHEAD = action;
    }
    action->button = button;
    action->flags = flags;
    action->callback = callback;
    action->data = data;
    return 1;
}

void BUTTON_UnregisterCallback(buttonAction_t *action)
{
    buttonAction_t *ptr = buttonHEAD;
    if (buttonHEAD == action) {
        buttonHEAD = action->next;
        return;
    }
    while(ptr) {
        if(ptr->next == action) {
            ptr->next = action->next;
            return;
        }
        ptr = ptr->next;
    }
}

void print_buttons(u32 buttons)
{
    char buttonstring[33];
    int i;
    for(i = 0; i < 32; i++)
        buttonstring[i] = (buttons & (1 << i)) ? '1' : '0';
    buttonstring[32] = 0;
    printf("Buttons: %s\n",buttonstring);
}

u32 BUTTON_Handler(u8 flags)
{
    buttonAction_t *ptr = buttonHEAD;
    u32 testbutton;
    u8 testflags;
    if(! flags) {
        u32 buttons = ScanButtons();
        if(buttons != last_buttons) {
            button_time = 0;
            print_buttons(buttons);
            if (buttons) {
                testbutton = buttons;
                testflags = BUTTON_PRESS;
            } else {
                testbutton = last_buttons;
                testflags = BUTTON_RELEASE | (long_press ? BUTTON_HAD_LONGPRESS : 0);
            }
            long_press = 0;
            last_buttons = buttons;
        } else {
            return buttons;
        }
    } else if (flags == BUTTON_LONGPRESS) {
        if (last_buttons && button_time++ >= 4 && (button_time & 0x01)) {
            testbutton = last_buttons;
            testflags = BUTTON_LONGPRESS;
            long_press = 1;
        } else {
            return 0;
        }
    }
    while(ptr) {
        if ((ptr->button & testbutton) && (ptr->flags & testflags)) {
            if(!(testflags & BUTTON_RELEASE) || buttonPressed == ptr) {
                if(ptr->callback(testbutton, testflags, ptr->data)) {
                    if (testflags & BUTTON_PRESS)
                        buttonPressed = ptr;
                    return 0;
                }
            }
        }
        ptr = ptr->next;
    }
    return testbutton;
}
