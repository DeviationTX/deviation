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

#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/tim.h"
#include "oled.h"
#include "lcd.h"

void BACKLIGHT_Init()
{
    if (HAS_OLED_DISPLAY) {
        _oled_backlight_init();
    } else {
        _lcd_backlight_init();
    }
}

void BACKLIGHT_Brightness(unsigned brightness)
{
    if (HAS_OLED_DISPLAY) {
        _oled_backlight_brightness(brightness);
    } else {
        _lcd_backlight_brightness(brightness);
    }
}
