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
#include "music.h"

void PAGE_Test();

static u8 warned = 0;
static u32 next_battery_warning = 0;
u8 BATTERY_Check()
{
    u16 battery = PWR_ReadVoltage();
    u32 ms = CLOCK_getms();
    // If battery is low or , was low and till under low + 200mV
    if (battery < Transmitter.batt_alarm && !(warned & BATTERY_LOW)) {
        warned |= BATTERY_LOW; // Bat was low...
        next_battery_warning = ms;
    } else if (battery > Transmitter.batt_alarm + 200) {
        warned &= ~BATTERY_LOW; // Bat OK... reset  'was low' and counter..
    }
    if ((warned & BATTERY_LOW) && ms >= next_battery_warning) {
#if HAS_EXTENDED_AUDIO
        MUSIC_PlayValue(MUSIC_BATT_ALARM, battery/10,MUSIC_UNIT_VOLT,2);
#else
        MUSIC_Play(MUSIC_BATT_ALARM);
#endif
        next_battery_warning = ms + Transmitter.batt_warning_interval * 1000;
    }
    if (battery < Transmitter.batt_critical && ! (warned & BATTERY_CRITICAL)) {
        PAGE_Test();
        CONFIG_SaveModelIfNeeded();
        CONFIG_SaveTxIfNeeded();
        SPI_FlashBlockWriteEnable(0); //Disable writing to all banks of SPIFlash
        warned |= BATTERY_CRITICAL;
        PAGE_ShowLowBattDialog();
    } else if (battery > Transmitter.batt_critical + 200) {
        warned &= ~BATTERY_CRITICAL;
        SPI_FlashBlockWriteEnable(1); //Disable writing to all banks of SPIFlash
    }
    return warned;
}
