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
#include "config/tx.h"

#if defined(EXTRA_SWITCHES)

extern u32 global_extra_switches;

#define SW_ENABLED(y) (((~Transmitter.ignore_src) & (y)) == ((srcsize_t)y))

// This function allows addons on 2 two-way, 1 3-way, or 2 3-way switches
// The modification adds diodes between the row and column lines that block
// Current in the normal-scan path, but allow current when the path is reversed
// values can only be scanned if not other buttons are pressed
void _scan_extra_switches()
{
    // Write to C.6, read B
    GPIO_setup_output(EXTRA_SWITCH_COL_OD, OTYPE_OPENDRAIN);
    GPIO_pin_clear(EXTRA_SWITCH_COL_OD);
    u32 port = GPIO_pin_get(BUTTON_MATRIX_ROW_OD);
    GPIO_setup_input(EXTRA_SWITCH_COL_OD, ITYPE_PULLUP);
    if SW_ENABLED(SWITCH_3x2) {
        global_extra_switches  = (~(port>>5))&0xf;
    } else if (SW_ENABLED(SWITCH_3x1)) {
        global_extra_switches = (((~port) >> 4) & 0x04) | (((~port) >> 5) & 0x08);
    } else {
        global_extra_switches  = (port>>6)&0x05;
    }
}

uint32_t ADDON_Handle_ExtraSwitches(u32 result) {
    uint32_t button_mask = 0;
    #define BUTTONDEF(x) button_mask |= (1 << ((BUT_ ## x) - 1));
    #include "capabilities.h"
    #undef BUTTONDEF
    if (!(result & button_mask)) {
        if ((SW_ENABLED(SWITCH_2x2) || SW_ENABLED(SWITCH_3x1)) && SW_ENABLED(SWITCH_STOCK)) {
            _scan_extra_switches();
        }
        if (!SW_ENABLED(SWITCH_STOCK)) {
            // The extra lines can only be used if the original switches were removed
            global_extra_switches = result;
        }
    }
    return result & button_mask;
}

#endif  // EXTRA_SWITCHES
