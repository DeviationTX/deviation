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

#ifndef LCD_CONTRAST_FUNC
    #define LCD_CONTRAST_FUNC(contrast) ((contrast) * 12 + 76)
#endif

inline static unsigned _lcd_contrast_func(unsigned contrast) {
    return LCD_CONTRAST_FUNC(contrast);
    // Devo contrast function
}

inline static void _lcd_reset()
{
    LCD_Cmd(0xE2);  // Reset
}

inline static void _lcd_init()
{
    volatile int i;
    LCD_Cmd(0xA6);      // Normal display
    LCD_Cmd(0xA4);      // All Points Normal
    LCD_Cmd(0xA0);      // Set SEG Direction (Normal)
    if (HAS_LCD_FLIPPED) {
        LCD_Cmd(0xC8);  // Set COM Direction (Reversed)
        LCD_Cmd(0xA2);  // Set The LCD Display Driver Voltage Bias Ratio (1/9)
    } else {
        LCD_Cmd(0xEA);  // ??
        LCD_Cmd(0xC4);  // Common Output Mode Scan Rate
    }
    LCD_Cmd(0x2C);      // Power Controller:Booster ON
    i = 0x8000;
    while (i) i--;
    LCD_Cmd(0x2E);      // Power Controller: VReg ON
    i = 0x8000;
    while (i) i--;
    LCD_Cmd(0x2F);      // Power Controller: VFollower ON
    i = 0x8000;
    while (i) i--;
    if (HAS_LCD_FLIPPED)
        LCD_Cmd(0x26);  // Select Internal Resistor Rate (Rb/Ra)
}
