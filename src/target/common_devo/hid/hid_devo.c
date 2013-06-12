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

#include "usb_lib.h"
#include "usb_pwr.h"

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
