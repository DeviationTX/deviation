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

static const u8 char_mapping[] = {
           // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
/* 0 */    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
/* 1 */    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
/* 2 */     ' ',  '!',  '"',  '#',  '$',  '%',  '&', 0x27,  '(',  ')', 0x82,  '+',  ',',  '-',  '.',  '/',
/* 3 */     '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
/* 4 */     '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
/* 5 */     'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '[', 0x80,  ']',  '?',  '-',
/* 6 */     '?',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
/* 7 */     'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '(',  '!',  ')', 0x86, 0x7F,
/* 8 */    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
/* 9 */    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
/* A */     ' ', 0x69, 0x85, 0x8B,  '?', 0x1F,  '!',  '?',  '?',  'C',  '?',  '<',  '?',  '-', 0x87,  '?',
/* B */    0x89,  '?',  '?',  '?',  '?',  'm',  '?', 0x00,  ',',  '?',  '?',  '>',  '?', 0x8A,  '?', 0x84,
/* C */    0xAF, 0xC3, 0xB4, 0xB9, 0xBE, 0xC9,  '?',  'C', 0xB0, 0xC4, 0xB5, 0xBF, 0xB1, 0xC5, 0xB6, 0xC0,
/* D */     '?', 0x7D, 0xB2, 0xC6, 0xB7, 0xBC, 0xC1,  'x',  'O', 0xB3, 0xC7, 0xB8, 0xC2,  'Y',  '?',  's',
/* E */    0x9B, 0x2A, 0xA0, 0xA5, 0xAA, 0xC8,  '?', 0x7B, 0x9C, 0x5C, 0xA1, 0xAB, 0x9D, 0x5E, 0xA2, 0xAC,
/* F */     '?', 0x7E, 0x9E, 0x5F, 0xA3, 0xA8, 0xAD, 0x7C,  'o', 0x9F, 0x60, 0xA4, 0xAE,  'y',  'p',  'y'
};

u32 TW8816_map_char(u32 c)
{
    if(c < 0x300) {           //from 0x300 start RAM fonts
                              //c = Unicode character code U+0XXX
        if(c < 0x100) {
            return char_mapping[c];
        }
        else {
            //Hungarian workaround (replace letters be lacking at ROM font)
            if(c == 0x150)    //Latin Capital Letter O with double acute
                return 0xBC;  //Latin Capital letter O with tilde
            if(c == 0x151)    //Latin Small Letter O with double acute
                return 0xA8;  //Latin Small Letter O with tilde
            if(c == 0x170)    //Latin Capital Letter U with double acute
                return 0xB8;  //Latin Capital Letter U with circumflex
            if(c == 0x171)    //Latin Small Letter U with double acute
                return 0xA4;  //Latin Small Letter U with circumflex
            //Romanian workaround (replace letters be lacking at ROM font)
            if(c == 0x102)    //Latin Capital Letter A with breve
                return 0xB9;  //Latin Capital letter A with tilde
            if(c == 0x103)    //Latin Small Letter A with breve
                return 0xA5;  //Latin Small Letter A with tilde
            if(c == 0x15E)    //Latin Capital Letter S with cedilla
                return 'S';   //Latin Capital letter S
            if(c == 0x15F)    //Latin Small Letter A with cedilla
                return 's';   //Latin Small Letter S
            if(c == 0x162)    //Latin Capital Letter T with cedilla
                return 'T';   //Latin Capital letter T
            if(c == 0x163)    //Latin Small Letter T with cedilla
                return 't';   //Latin Small Letter T
            if(c == 0x218)    //Latin Capital Letter S with comma
                return 'S';   //Latin Capital letter S
            if(c == 0x219)    //Latin Small Letter S with comma
                return 's';   //Latin Small Letter S
            if(c == 0x21A)    //Latin Capital Letter T with comma
                return 'T';   //Latin Capital letter T
            if(c == 0x21B)    //Latin Small Letter T with comma
                return 't';   //Latin Small Letter T
            //Czech workaround (replace letters be lacking at ROM font)
            if(c == 0x10C)    //Latin Capital Letter C with caron
                return 'C';   //Latin Capital letter C
            if(c == 0x10D)    //Latin Small Letter C with caron
                return 'c';   //Latin Small Letter C
            if(c == 0x10E)    //Latin Capital Letter D with caron
                return 'D';   //Latin Capital letter D
            if(c == 0x10F)    //Latin Small Letter D with caron
                return 'd';   //Latin Small Letter D
            if(c == 0x11A)    //Latin Capital Letter E with caron
                return 'E';   //Latin Capital letter E
            if(c == 0x11B)    //Latin Small Letter E with caron
                return 'e';   //Latin Small Letter E
            if(c == 0x147)    //Latin Capital Letter N with caron
                return 'N';   //Latin Capital letter N
            if(c == 0x148)    //Latin Small Letter N with caron
                return 'n';   //Latin Small Letter N
            if(c == 0x158)    //Latin Capital Letter R with caron
                return 'R';   //Latin Capital letter R
            if(c == 0x159)    //Latin Small Letter R with caron
                return 'r';   //Latin Small Letter R
            if(c == 0x160)    //Latin Capital Letter S with caron
                return 'S';   //Latin Capital letter S
            if(c == 0x161)    //Latin Small Letter S with caron
                return 's';   //Latin Small Letter S
            if(c == 0x164)    //Latin Capital Letter T with caron
                return 'T';   //Latin Capital letter T
            if(c == 0x165)    //Latin Small Letter T with caron
                return 't';   //Latin Small Letter T
            if(c == 0x16E)    //Latin Capital Letter U with ring above
                return 'U';   //Latin Capital letter U
            if(c == 0x16F)    //Latin Small Letter U with ring above
                return 'u';   //Latin Small Letter U
            if(c == 0x17D)    //Latin Capital Letter Z with caron
                return 'Z';   //Latin Capital letter Z
            if(c == 0x17E)    //Latin Small Letter Z with caron
                return 'z';   //Latin Small Letter Z
            //Other languages workarounds
            //FIXME
            return '?';
        }
    }
    return c;
}
