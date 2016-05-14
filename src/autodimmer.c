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
#include "autodimmer.h"
#include "config/tx.h"

static u32 last_time;  // last_time must be u32
static AutoDimmerState auto_dimmer_state = AUTODIMMER_STOP;  //backlight on

void AUTODIMMER_Init()
{
    last_time = CLOCK_getms(); // DON'T turn off backlight
    auto_dimmer_state = AUTODIMMER_STOP;
}
/**
 * Check if need to turn back on backlight
 */
void AUTODIMMER_Check()
{
    last_time = CLOCK_getms();
    if (Transmitter.auto_dimmer.timer == 0) // no auto dimmer set, return ASAP
        return;
    if (Transmitter.backlight <= Transmitter.auto_dimmer.backlight_dim_value)
        return; // dimmer target is even higher than current brightness, no need to make backlight dimmer
    if (auto_dimmer_state == AUTODIMMER_STOP)
        return; //backlight is already on, no need to turn it on again
    BACKLIGHT_Brightness(Transmitter.backlight); // Turn on the back-light immediately ,don't wait to next loop
    auto_dimmer_state = AUTODIMMER_STOP;
}

void AUTODIMMER_Update()
{
    if (Transmitter.auto_dimmer.timer == 0) // no auto dimmer set, return ASAP
        return;
    if (Transmitter.backlight <= Transmitter.auto_dimmer.backlight_dim_value)
        return; // dimmer target is even higher than current brightness, no need to make backlight dimmer
    if (auto_dimmer_state == AUTODIMMER_START)
        return; //backlight is already dimmer, no need to re-dim it
    u32 current_time = CLOCK_getms();
    if (current_time - last_time < Transmitter.auto_dimmer.timer)
        return;
    last_time = current_time;
    BACKLIGHT_Brightness(Transmitter.auto_dimmer.backlight_dim_value);
    auto_dimmer_state = AUTODIMMER_START;
}
