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
#ifndef __FONT_H__
#define __FONT_H__

#define CHAR_BUF_SIZE 80

extern void char_read(u8 *font, u32 c, u8 *width);
extern u8 get_width(u32 c);
extern u8 get_height();
extern void close_font();
extern u8 open_font(const char* font);
#endif
