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
    along with Deviation.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "target/drivers/mcu/stm32/rcc.h"

void lcd_display(uint8_t on);

inline static unsigned _oled_contrast_func(unsigned contrast) {
    return contrast * contrast * 255 / 100;  // contrast should range from 0 to 255
}

inline static void _oled_reset()
{
    // No reset for OLED
}

inline static void _oled_init()
{
    LCD_Cmd(0xD5);  // Set Display Clock Divide Ratio / OSC Frequency
    LCD_Cmd(0x80);  // Display Clock Divide Ratio / OSC Frequency
    LCD_Cmd(0xA8);  // Set Multiplex Ratio
    LCD_Cmd(0x3F);  // Multiplex Ratio for 128x64 (LCD_HEIGHT - 1)
    LCD_Cmd(0xD3);  // Set Display Offset
    LCD_Cmd(0x00);  // Display Offset (0)
    LCD_Cmd(0x40);  // Set Display Start Line (0)
    LCD_Cmd(0x8D);  // Set Charge Pump
    LCD_Cmd(0x10);  // Charge Pump (0x10 External, 0x14 Internal DC/DC)
    LCD_Cmd(0xA1);  // Set Segment Re-Map (Reversed)
    LCD_Cmd(0xC8);  // Set Com Output Scan Direction (Reversed)
    LCD_Cmd(0xDA);  // Set COM Hardware Configuration
    LCD_Cmd(0x12);  // COM Hardware Configuration
    LCD_Cmd(0xD9);  // Set Pre-Charge Period
    LCD_Cmd(0x4F);  // Set Pre-Charge Period (A[7:4]:Phase 2, A[3:0]:Phase 1)
    LCD_Cmd(0xDB);  // Set VCOMH Deselect Level
    LCD_Cmd(0x20);  // VCOMH Deselect Level (0x00 ~ 0.65 x VCC, 0x10 ~ 0.71 x VCC, 0x20 ~ 0.77 x VCC, 0x30 ~ 0.83 x VCC)
    LCD_Cmd(0xA4);  // Disable Entire Display On
    LCD_Cmd(0xA6);  // Set Normal Display (not inverted)
    LCD_Cmd(0x2E);  // Deactivate scroll
}

/* These are called by backlight.c */
void OLED_Backlight() {
    rcc_periph_clock_enable(get_rcc_from_pin(BACKLIGHT_TIM.pin));
    // Turn off backlight
    GPIO_setup_input(BACKLIGHT_TIM.pin, ITYPE_FLOAT);
}

void OLED_Backlight_Brightness(unsigned brightness)
{
    LCD_Contrast(brightness);
    if (brightness == 0) {
        lcd_display(0);    // Display Off
        // Charge Pump disable
        // LCD_Cmd(0x8D);  // Set Charge Pump
        // LCD_Cmd(0x10);  // Charge Pump (0x10 External, 0x14 Internal DC/DC)
    } else {
        // Charge Pump enable
        // LCD_Cmd(0x8D);  // Set Charge Pump
        // LCD_Cmd(0x14);  // Charge Pump (0x10 External, 0x14 Internal DC/DC)
        lcd_display(1);    // Display On
    }
}
