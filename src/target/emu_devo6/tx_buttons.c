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
#include "../common/emu/fltk.h"

#define BUTTON_LEFT_UP     (1 << BUT_TRIM_L_POS)
#define BUTTON_LEFT_DOWN   (1 << BUT_TRIM_L_NEG)
#define BUTTON_LEFT_BOTH   (1 << BUT_TRIM_L_POS | 1 << BUT_TRIM_L_NEG)
#define BUTTON_RIGHT_UP    (1 << BUT_TRIM_R_POS)
#define BUTTON_RIGHT_DOWN  (1 << BUT_TRIM_R_NEG)
#define BUTTON_RIGHT_BOTH  (1 << BUT_TRIM_R_POS | 1 << BUT_TRIM_R_NEG)
#define BUTTON_ALL         (1 << BUT_TRIM_R_POS | 1 << BUT_TRIM_R_NEG \
                          | 1 << BUT_TRIM_L_POS | 1 << BUT_TRIM_L_NEG)


void CHAN_SetButtonCfg(const char *str)
{
    if(strcmp(str, "trim-left-both") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_BOTH;
    } else if(strcmp(str, "trim-left-up") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_UP;
    } else if(strcmp(str, "trim-left-down") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_DOWN;
    } else if(strcmp(str, "trim-right-both") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_BOTH;
    } else if(strcmp(str, "trim-right-up") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_UP;
    } else if(strcmp(str, "trim-right-down") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_DOWN;
    } else if(strcmp(str, "trim-all") == 0) {
        Transmitter.ignore_buttons = 0;
    } else {
        Transmitter.ignore_buttons = BUTTON_ALL;
    }
}
