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

#include <libopencm3/cm3/nvic.h>

void USB_Istr(void);

/*
void usb_hp_can_tx_isr()
{
        USB_Istr();
}
*/

void __attribute__((__used__)) usb_lp_can_rx0_isr()
{
        USB_Istr();
}