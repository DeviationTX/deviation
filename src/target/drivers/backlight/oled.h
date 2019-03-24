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

/* These are defined in 128x64x1_oled_ssd1306.h */
void OLED_Backlight();
void OLED_Backlight_Brightness(unsigned brightness);

static void _oled_backlight_init()
{
    OLED_Backlight();
}

static void _oled_backlight_brightness(unsigned brightness)
{
    OLED_Backlight_Brightness(brightness);
}
