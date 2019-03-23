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

#if defined(HAS_EXTRA_SWITCHES) && HAS_EXTRA_SWITCHES
u32 global_extra_switches = 0;

#define SW_ENABLED(y) (((~Transmitter.ignore_src) & (y)) == ((srcsize_t)y))
#define TO_PIN(...) (struct mcu_pin) { __VA_ARGS__ }

s32 ADDON_ReadRawInput(int channel)
{
    #define CHAN_INVERT !
    #define CHAN_NONINV
    #define OPT_2WAY(ch, pin, inv, cond) \
        if (channel == ch##0 && SW_ENABLED(cond)) { return !inv GPIO_pin_get(TO_PIN pin); } \
        if (channel == ch##1 && SW_ENABLED(cond)) { return inv GPIO_pin_get(TO_PIN pin); }
    #define OPT_3WAY(ch, pin0, pin2, inv, cond) \
        if (channel == ch##0 && SW_ENABLED(cond)) { return inv GPIO_pin_get(TO_PIN pin0); } \
        if (channel == ch##1 && SW_ENABLED(cond)) { return !(inv GPIO_pin_get(TO_PIN pin0) || inv GPIO_pin_get(TO_PIN pin2)); } \
        if (channel == ch##2 && SW_ENABLED(cond)) { return inv GPIO_pin_get(TO_PIN pin2); }

    #define EXTRA_3WAY(ch, mask0, mask2, inv, cond) \
        if (channel == ch##0 && SW_ENABLED(cond)) { return inv (global_extra_switches & mask0); } \
        if (channel == ch##1 && SW_ENABLED(cond)) { return inv !(global_extra_switches & (mask0 | mask2)); } \
        if (channel == ch##2 && SW_ENABLED(cond)) { return inv (global_extra_switches & mask2); }
    #define EXTRA_2WAY(ch, mask0, mask2, inv, cond) \
        if (channel == ch##0 && SW_ENABLED(cond)) { return inv !(global_extra_switches & mask0); } \
        if (channel == ch##1 && SW_ENABLED(cond)) { return inv (global_extra_switches & (mask0 | mask2)); }

    // OPT_SWITCHES: Optional switches connected directly to unused MCU pins
    #if defined OPT_SWITCHES
    OPT_SWITCHES
    #endif

    // EXTRA_SWITCHES: Addon switches connected to button matrix
    #if defined EXTRA_SWITCHES
    if (SW_ENABLED(SWITCH_STOCK)) {
        EXTRA_SWITCHES
    }
    #endif
    #if defined REPLACEMENT_SWITCHES
    if (!SW_ENABLED(SWITCH_STOCK)) {
        REPLACEMENT_SWITCHES
    }
    #endif
    return 0;
}

void CHAN_SetSwitchCfg(const char *str)
{
    #define ADDON_SWITCH(x, and_mask, or_mask) \
    if (strcmp(str, x) == 0) {                 \
        Transmitter.ignore_src &= ~(and_mask); \
        Transmitter.ignore_src |= (or_mask);   \
        return;                                \
    }
    ADDON_SWITCH_CFG;
    #undef ADDON_SWITCH
    u32 mask = 0;
    #define ADDON_SWITCH(x, and_mask, or_mask) mask |= (and_mask);
    ADDON_SWITCH_CFG;
    Transmitter.ignore_src = mask;
}

#endif  // HAS_EXTRA_SWITCHES
