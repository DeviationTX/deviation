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

    This file is partially based upon the CooCox ILI9341S driver
    http://www.coocox.org/driver_repo/305488dd-734b-4cce-a8a4-39dcfef8cc66/html/group___i_l_i9341_s.html
*/

void st7796_draw_start(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    // setup x
    LCD_REG = 0x2a;
    LCD_DATA = (u8)(x0 >> 8);
    LCD_DATA = (u8)(x0);
    LCD_DATA = (u8)(x1 >> 8);
    LCD_DATA = (u8)(x1);

    // setup y
    LCD_REG = 0x2b;
    LCD_DATA = (u8)(y0 >> 8);
    LCD_DATA = (u8)(y0);
    LCD_DATA = (u8)(y1 >> 8);
    LCD_DATA = (u8)(y1);

    LCD_REG = 0x2c;
}

void st7796_set_pos(unsigned int x0, unsigned int y0)
{
    st7796_draw_start(x0, y0, x0, y0);
}

void st7796_sleep()
{
}

static const struct lcdtype st7796_type = {
    st7796_set_pos,
    st7796_draw_start,
    st7796_sleep,
};

void st7796_init() {
    LCD_REG = 0x11;
    _msleep(120);

    LCD_REG = 0xf0;
    LCD_DATA = 0xc3;

    LCD_REG = 0xf0;
    LCD_DATA = 0x96;

    LCD_REG = 0x36;
    LCD_DATA = 0x28;

    LCD_REG = 0x3a;
    LCD_DATA = 0x5;

    LCD_REG = 0xb4;
    LCD_DATA = 0x01;

    LCD_REG = 0xe8;
    LCD_DATA = 0x40;
    LCD_DATA = 0x8a;
    LCD_DATA = 0x00;
    LCD_DATA = 0x00;
    LCD_DATA = 0x29;
    LCD_DATA = 0x19;
    LCD_DATA = 0xa5;
    LCD_DATA = 0x33;

    LCD_REG = 0xc1;
    LCD_DATA = 0x06;

    LCD_REG = 0xc2;
    LCD_DATA = 0xa7;

    LCD_REG = 0xc5;
    LCD_DATA = 0x18;

    LCD_REG = 0xe0;
    LCD_DATA = 0xf0;
    LCD_DATA = 0x09;
    LCD_DATA = 0x0b;
    LCD_DATA = 0x06;
    LCD_DATA = 0x04;
    LCD_DATA = 0x15;
    LCD_DATA = 0x2f;
    LCD_DATA = 0x54;
    LCD_DATA = 0x42;
    LCD_DATA = 0x3c;
    LCD_DATA = 0x17;
    LCD_DATA = 0x14;
    LCD_DATA = 0x18;
    LCD_DATA = 0x1b;

    LCD_REG = 0xe1;
    LCD_DATA = 0xf0;
    LCD_DATA = 0x09;
    LCD_DATA = 0x0b;
    LCD_DATA = 0x06;
    LCD_DATA = 0x04;
    LCD_DATA = 0x03;
    LCD_DATA = 0x2d;
    LCD_DATA = 0x43;
    LCD_DATA = 0x42;
    LCD_DATA = 0x3b;
    LCD_DATA = 0x16;
    LCD_DATA = 0x14;
    LCD_DATA = 0x17;
    LCD_DATA = 0x1b;

    LCD_REG = 0xf0;
    LCD_DATA = 0x3c;

    LCD_REG = 0xf0;
    LCD_DATA = 0x69;

    _msleep(120);

    LCD_REG = 0x29;

    LCD_REG = 0x2a;
    LCD_DATA = 0;
    LCD_DATA = 0;
    LCD_DATA = 1;
    LCD_DATA = 0x3F;

    LCD_REG = 0x2b;
    LCD_DATA = 0;
    LCD_DATA = 0;
    LCD_DATA = 0;
    LCD_DATA = 0xEF;

    disp_type = &st7796_type;
}
