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
#include "buttons.h"

static buttonAction_t *buttonHEAD = NULL;
static buttonAction_t *buttonPressed = NULL;

void exec_callbacks(u32, enum ButtonFlags);

u8 BUTTON_RegisterCallback(buttonAction_t *action, u32 button, u8 flags,
                 u8 (*callback)(u32 button, u8 flags, void *data), void *data)
{
    buttonAction_t *ptr = buttonHEAD;

    if (! (flags & BUTTON_PRIORITY)) {
        while(ptr) {
            if((ptr->button & button) && (ptr->flags & flags) & ! (ptr->flags & BUTTON_PRIORITY)) {
                printf("WARNING: Button %08x with flags %d is already assigned\n", (unsigned int)button, flags);
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

void BUTTON_Handler()
{
    static u32 last_buttons = 0;
    static u32 last_buttons_pressed = 0;

    static u32 long_press_at = 0;
    static u8  longpress_release = 0;

    u32 buttons = ScanButtons();

    u32 buttons_pressed=   buttons  & (~last_buttons);
    u32 buttons_released=(~buttons) &   last_buttons;

    if(buttons_pressed) {
        exec_callbacks(buttons_pressed, BUTTON_PRESS);
        last_buttons_pressed = buttons_pressed;
        long_press_at = CLOCK_getms()+500;
        longpress_release = 0;
    }
    
    if(buttons_released) {
        if(!longpress_release) {
            exec_callbacks(buttons_released, BUTTON_RELEASE);
        } else {
            exec_callbacks(buttons_released, BUTTON_RELEASE | BUTTON_HAD_LONGPRESS);
        }
    }

    if(buttons && (buttons == last_buttons)) {
        if(CLOCK_getms()>long_press_at) {
           exec_callbacks(last_buttons_pressed, BUTTON_LONGPRESS);
           longpress_release=1;
           long_press_at += 100;
        }
    }

    last_buttons=buttons;    
}    

void exec_callbacks(u32 buttons, enum ButtonFlags flags) {
    buttonAction_t *ptr = buttonHEAD;

    while(ptr) {
        if ((ptr->button & buttons) && (ptr->flags & flags)) {
            if(!(flags & BUTTON_RELEASE) || buttonPressed == ptr) {
                //We only send a release to the button that accepted a press
                if(ptr->callback(buttons, flags, ptr->data)) {
                    //Exit after the 1st action accepts the button
                    if (flags & BUTTON_PRESS)
                        buttonPressed = ptr;
                    return;
                }
            }
        }
        ptr = ptr->next;
    }

}  
