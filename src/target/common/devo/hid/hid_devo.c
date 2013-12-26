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

#ifdef MODULAR
  #pragma long_calls
  //This is otherwise included in the main binary
  #include "usb_regs.c"
#endif

#include <string.h>
#include "usb_lib.h"
#include "usb_pwr.h"

/* These are mostly defined in usb_devo8.c */
extern void (*pEpInt_IN[7])(void);
extern void (*pEpInt_OUT[7])(void);
extern void (* const HID_pEpInt_IN[7])(void);
extern void (* const HID_pEpInt_OUT[7])(void);
extern DEVICE_PROP HID_Device_Property;
extern USER_STANDARD_REQUESTS HID_User_Standard_Requests;
extern void USB_Enable(u8 type, u8 use_interrupt);
extern void USB_Disable();

extern volatile u8 PrevXferComplete;
void HID_Write(s8 *packet, u8 num_channels)
{
    if (bDeviceState != CONFIGURED)
        return;
    PrevXferComplete = 0;
    //printf("sending\n");
    USB_SIL_Write(EP1_IN, (u8 *)packet, num_channels);
    SetEPTxValid(ENDP1);
}

void HID_Init() {
    memcpy(pEpInt_IN, HID_pEpInt_IN, sizeof(pEpInt_IN));
    memcpy(pEpInt_OUT, HID_pEpInt_OUT, sizeof(pEpInt_OUT));
    Device_Property = &HID_Device_Property;
    User_Standard_Requests = &HID_User_Standard_Requests;
}

extern void (*_HID_Init)();
void HID_Enable() {
#ifdef MODULAR
     _HID_Init = HID_Init;
#endif
    USB_Enable(1, 1);
}

void HID_Disable() {
#ifdef MODULAR
    _HID_Init = NULL;
#endif
    USB_Disable();
}
