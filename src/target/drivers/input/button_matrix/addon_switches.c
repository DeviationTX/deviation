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

// Extra switch connections
//         2x2              3x1              3x2
//         -----            -----            -----
// B.5                                        SW_B0
// B.6      SW_B1            SW_A0            SW_B2
// B.7                                        SW_A0
// B.8      SW_A1            SW_A2            SW_A2
// global_extra_switches:
//   .0 == B0
//   .1 == B2
//   .2 == A0
//   .3 == A2

#include "common.h"
#include "config/tx.h"

#if defined(HAS_EXTRA_SWITCHES) && HAS_EXTRA_SWITCHES

#if defined HAS_SWITCHES_NOSTOCK
#define IGNORE_MASK ((1 << INP_AILERON) | (1 << INP_ELEVATOR) | (1 << INP_THROTTLE) | (1 << INP_RUDDER) | (1 << INP_NONE) | (1 << INP_LAST))
#define SWITCH_3x4  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) | (1 << INP_SWD2))
#define SWITCH_3x3  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2))
#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x8  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) \
                   | (1 << INP_SWA0) | (1 << INP_SWA1))
#define SWITCH_2x7  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1))
#define SWITCH_2x6  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1))
#define SWITCH_2x5  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1))
#define SWITCH_2x4  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1))
#define SWITCH_2x3  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1))
#define SWITCH_2x2  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1))
#define SWITCH_2x1  ((1 << INP_SWH0) | (1 << INP_SWH1))

#define SWITCH_STOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) \
                    | (1 << INP_FMOD0) | (1 << INP_FMOD1))

#else  // HAS_SWITCHES_NOSTOCK

#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x2  ((1 << INP_SWA0) | (1 << INP_SWA1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1))
#define SWITCH_STOCK (0)
#endif  // HAS_SWITCHES_NOSTOCK

#define EXTRA_SWITCH_MASK 0x003FFFFF  // Mask off upper 10 bits

u32 global_extra_switches = 0;

#define SW_ENABLED(x, y) (((x) & (y)) == (y))

// This function allows addons on 2 two-way, 1 3-way, or 2 3-way switches
// The modification adds diodes between the row and column lines that block
// Current in the normal-scan path, but allow current when the path is reversed
// values can only be scanned if not other buttons are pressed
void _scan_extra_switches()
{
    // Write to C.6, read B
    u32 src = ~Transmitter.ignore_src;
    GPIO_setup_output(EXTRA_SWITCH_COL_OD, OTYPE_OPENDRAIN);
    GPIO_pin_clear(EXTRA_SWITCH_COL_OD);
    u32 port = GPIO_pin_get(BUTTON_MATRIX_ROW_OD);
    GPIO_setup_input(EXTRA_SWITCH_COL_OD, ITYPE_PULLUP);
    if (SW_ENABLED(src, SWITCH_3x1) && !SW_ENABLED(src, SWITCH_3x2)) {
        global_extra_switches = (((~port) >> 4) & 0x04) | (((~port) >> 5) & 0x08);
    } else if SW_ENABLED(src, SWITCH_2x2) {
        global_extra_switches  = (port>>6)&0x05;
    } else {
        global_extra_switches  = (~(port>>5))&0xf;
    }
}

uint32_t ADDON_Handle_ExtraSwitches(u32 result) {
    u32 src = ~Transmitter.ignore_src;
    if (!(result & EXTRA_SWITCH_MASK)) {
        if ((SW_ENABLED(src, SWITCH_2x2) || SW_ENABLED(src, SWITCH_3x1)) && SW_ENABLED(src, SWITCH_STOCK)) {
            _scan_extra_switches();
        }
#ifdef HAS_SWITCHES_NOSTOCK
        if (!SW_ENABLED(src, SWITCH_STOCK)) {
            // The extra lines can only be used if the original switches were removed
            global_extra_switches = result;
        }
#endif
    }
    return result & EXTRA_SWITCH_MASK;
}

#else  // HAS_SWITCHES_NOSTOCK
uint32_t ADDON_Handle_ExtraSwitches(u32 result) { return result; }

#endif  // HAS_SWITCHES_NOSTOCK
