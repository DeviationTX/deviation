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
#include "autodimmer.h"

static buttonAction_t *buttonHEAD = NULL;
static buttonAction_t *buttonPressed = NULL;
#define DEBOUNCE_WAIT_MS 20

void exec_callbacks(u32, enum ButtonFlags);

unsigned BUTTON_RegisterCallback(buttonAction_t *action, u32 button, unsigned flags,
                 unsigned (*callback)(u32 button, unsigned flags, void *data), void *data)
{
    buttonAction_t *ptr;
    //Ensure 'action' is not already in the linked list
    if (buttonHEAD == action) {
        buttonHEAD = action->next;
    } else if(buttonHEAD) {
        for(ptr = buttonHEAD; ptr->next; ptr = ptr->next) {
            if(ptr->next == action) {
                ptr->next = ptr->next->next;
                break;
            }
        }
    }
    ptr = buttonHEAD;
    if (! (flags & BUTTON_PRIORITY)) {
        while(ptr) {
            if((ptr->button & button) && (ptr->flags & flags) && ! (ptr->flags & BUTTON_PRIORITY)) {
                printf("WARNING: Button %08x with flags %d is already assigned\n", (unsigned int)button, flags);
                //memset(action, 0, sizeof(buttonAction_t));
                //return 0;
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
static u8 interrupt_longpress = 0;
void BUTTON_Handler()
{
    static u32 last_buttons = 0;
    static u32 last_buttons_pressed = 0;

    static u32 long_press_at = 0;
    static u8  longpress_release = 0;
    static u32 last_button_time = 0;

    u32 ms = CLOCK_getms();
    //debounce
    if (ms < last_button_time)
        return;
    u32 buttons = ScanButtons();

    u32 buttons_pressed=   buttons  & (~last_buttons);
    u32 buttons_released=(~buttons) &   last_buttons;

    if (buttons != last_buttons)
        last_button_time = ms;

    if(buttons_pressed && !longpress_release) {
        //printf("pressed: %08d\n", buttons_pressed);
        AUTODIMMER_Check();
        exec_callbacks(buttons_pressed, BUTTON_PRESS);
        last_buttons_pressed = buttons_pressed;
        long_press_at = ms+500;
        longpress_release = 0;
        interrupt_longpress = 0;
    }
    
    if(buttons_released) {
        //printf("release: %08d\n", buttons_released);
        interrupt_longpress = 0;
        longpress_release = 0;
        if(!longpress_release) {
            exec_callbacks(buttons_released, BUTTON_RELEASE);
        } else {
            exec_callbacks(buttons_released, BUTTON_RELEASE | BUTTON_HAD_LONGPRESS);
        }
    }

    if(buttons && (buttons == last_buttons) && !interrupt_longpress) {
        if(ms > long_press_at) {
            //printf("long_press: %08d\n", buttons_released);
            exec_callbacks(last_buttons_pressed, BUTTON_LONGPRESS);
            longpress_release=1;
            long_press_at += 100;
        }
    }

    last_buttons=buttons;    
}

void BUTTON_InterruptLongPress()
{
    //printf("interrupt \n");
    interrupt_longpress = 1;
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
