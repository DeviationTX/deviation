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

static struct usb_page * const up = &pagemem.u.usb_page;

static void _draw_page(u8 enable);

void wait_release();
void wait_press();

unsigned usb_cb(u32 button, unsigned flags, void *data)
{
    (void)button;
    (void)data;
    if(flags == BUTTON_RELEASE) {
        _draw_page(1);
        GUI_RefreshScreen();
        CONFIG_SaveModelIfNeeded();
        USB_Enable(0, 1);
        wait_release();
        wait_press();
        wait_release();
        USB_Disable();
        CONFIG_ReadModel(Transmitter.current_model);
        _draw_page(0);
    }
    return 1;
}

void PAGE_USBInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    _draw_page(0);
    BUTTON_RegisterCallback(&up->action, CHAN_ButtonMask(BUT_ENTER), BUTTON_PRESS | BUTTON_RELEASE | BUTTON_PRIORITY, usb_cb, NULL);
}

void PAGE_USBExit()
{
    BUTTON_UnregisterCallback(&up->action);
}
