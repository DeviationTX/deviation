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
u32 TW8816_map_char(u32 c)
{
    if(c < 0x300) {           //from 0x300 start RAM fonts
                              //c = Unicode character code U+0XXX
        if(c >= 0x100) {
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
            //Other languages workarounds
            //FIXME
            return c;
        }
        switch(c) {
            //...
            case 0x2A: return 0x82; //Asterisk
            //...
            case 0x5C: return 0x80; //Backslash
            //...
            case 0x5E: return '?';  //-Circumflex accent
            case 0x5F: return '-';  //'_' -Low line
            case 0x60: return '?';  //-Grave accent
            //...
            case 0x7B: return '(';  //-Left Curly Bracket
            case 0x7C: return '!';  //-Vertical bar
            case 0x7D: return ')';  //-Right Curly Bracket
            case 0x7E: return 0x86; //Tilde
            //...Control codes 0x80 to 0x9F
            case 0xA0: return ' ';  //Non-breaking space
            case 0xA1: return 0x69; //Inverted Exclamation Mark
            case 0XA2: return 0x85; //Cent sign
            case 0xA3: return 0x8B; //Pound sign
            case 0xA4: return '?';  //-Currency sign
            case 0xA5: return 0x1F; //Yen sign
            case 0xA6: return '!';  //-Broken bar
            case 0xA7: return '?';  //-Section sign
            case 0xA8: return '?';  //-Diaeresis (Umlaut)
            case 0xA9: return 'C';  //-Copyright sign
            case 0xAA: return '?';  //-Feminine Ordinal Indicator
            case 0xAB: return '<';  //-Left-pointing double angle quotation mark
            case 0xAC: return '?';  //-Not sign
            case 0xAD: return '-';  //-Soft hyphen
            case 0xAE: return 0x87; //Registered sign
            case 0xAF: return '?';  //-Macron
            case 0xB0: return 0x89; //Degree symbol
            case 0xB1: return '?';  //-Plus-minus sign
            case 0xB2: return '?';  //-Superscript two
            case 0xB3: return '?';  //-Superscript three
            case 0xB4: return '?';  //-Acute accent
            case 0xB5: return 'm';  //-Micro sign
            case 0xB6: return '?';  //-Pilcrow sign
            case 0xB7: return 0x00; //Middle dot
            case 0xB8: return ',';  //-Cedilla
            case 0xB9: return '?';  //-Superscript one
            case 0xBA: return '?';  //-Masculine ordinal indicator
            case 0xBB: return '>';  //-Right-pointing double angle quotation mark
            case 0xBC: return '?';  //-Vulgar fraction one quarter
            case 0xBD: return 0x8A; //Vulgar fraction one half
            case 0xBE: return '?';  //-Vulgar fraction three quarters
            case 0xBF: return 0x84; //Inverted Question Mark
            case 0xC0: return 0xAF; //Latin Capital Letter A with grave
            case 0xC1: return 0xC3; //Latin Capital letter A with acute
            case 0xC2: return 0xB4; //Latin Capital letter A with circumflex
            case 0xC3: return 0xB9; //Latin Capital letter A with tilde
            case 0xC4: return 0xBE; //Latin Capital letter A with diaeresis
            case 0xC5: return 0xC9; //Latin Capital letter A with ring above
            case 0xC6: return '?';  //-Latin Capital letter AE
            case 0xC7: return 'C';  //-Latin Capital letter C with cedilla
            case 0xC8: return 0xB0; //Latin Capital letter E with grave
            case 0xC9: return 0xC4; //Latin Capital letter E with acute
            case 0xCA: return 0xB5; //Latin Capital letter E with circumflex
            case 0xCB: return 0xBF; //Latin Capital letter E with diaeresis
            case 0xCC: return 0xB1; //Latin Capital letter I with grave
            case 0xCD: return 0xC5; //Latin Capital letter I with acute
            case 0xCE: return 0xB6; //Latin Capital letter I with circumflex
            case 0xCF: return 0xC0; //Latin Capital letter I with diaeresis
            case 0xD0: return '?';  //-Latin Capital letter Eth
            case 0xD1: return 0x7D; //Latin Capital letter N with tilde
            case 0xD2: return 0xB2; //Latin Capital letter O with grave
            case 0xD3: return 0xC6; //Latin Capital letter O with acute
            case 0xD4: return 0xB7; //Latin Capital letter O with circumflex
            case 0xD5: return 0xBC; //Latin Capital letter O with tilde
            case 0xD6: return 0xC1; //Latin Capital letter O with diaeresis
            case 0xD7: return 'x';  //Multiplication sign
            case 0xD8: return 'O';  //-Latin Capital letter O with stroke
            case 0xD9: return 0xB3; //Latin Capital letter U with grave
            case 0xDA: return 0xC7; //Latin Capital letter U with acute
            case 0xDB: return 0xB8; //Latin Capital Letter U with circumflex
            case 0xDC: return 0xC2; //Latin Capital Letter U with diaeresis
            case 0xDD: return 'Y';  //-Latin Capital Letter Y with acute
            case 0xDE: return '?';  //-Latin Capital Letter Thorn
            case 0xDF: return 's';  //-Latin Small Letter sharp S
            case 0xE0: return 0x9B; //Latin Small Letter A with grave
            case 0xE1: return 0x2A; //Latin Small Letter A with acute
            case 0xE2: return 0xA0; //Latin Small Letter A with circumflex
            case 0xE3: return 0xA5; //Latin Small Letter A with tilde
            case 0xE4: return 0xAA; //Latin Small Letter A with diaeresis
            case 0xE5: return 0xC8; //Latin Small Letter A with ring above
            case 0xE6: return '?';  //-Latin Small Letter AE
            case 0xE7: return 0x7B; //Latin Small Letter C with cedilla
            case 0xE8: return 0x9C; //Latin Small Letter E with grave
            case 0xE9: return 0x5C; //Latin Small Letter E with acute
            case 0xEA: return 0xA1; //Latin Small Letter E with circumflex
            case 0xEB: return 0xAB; //Latin Small Letter E with diaeresis
            case 0xEC: return 0x9D; //Latin Small Letter I with grave
            case 0xED: return 0x5E; //Latin Small Letter I with acute
            case 0xEE: return 0xA2; //Latin Small Letter I with circumflex
            case 0xEF: return 0xAC; //Latin Small Letter I with diaeresis
            case 0xF0: return '?';  //-Latin Small Letter Eth
            case 0xF1: return 0x7E; //Latin Small Letter N with tilde
            case 0xF2: return 0x9E; //Latin Small Letter O with grave
            case 0xF3: return 0x5F; //Latin Small Letter O with acute
            case 0xF4: return 0xA3; //Latin Small Letter O with circumflex
            case 0xF5: return 0xA8; //Latin Small Letter O with tilde
            case 0xF6: return 0xAD; //Latin Small Letter O with diaeresis
            case 0xF7: return 0x7C; //Division sign
            case 0xF8: return 'o';  //-Latin Small Letter O with stroke
            case 0xF9: return 0x9F; //Latin Small Letter U with grave
            case 0xFA: return 0x60; //Latin Small Letter U with acute
            case 0xFB: return 0xA4; //Latin Small Letter U with circumflex
            case 0xFC: return 0xAE; //Latin Small Letter U with diaeresis
            case 0xFD: return 'y';  //-Latin Small Letter Y with acute
            case 0xFE: return 'p';  //-Latin Small Letter Thorn
            case 0xFF: return 'y';  //-Latin Small Letter Y with diaeresis

            default : return c;
        }
    }
    return c;
}
