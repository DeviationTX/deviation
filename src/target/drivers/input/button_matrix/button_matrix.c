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
#include "target/drivers/mcu/stm32/rcc.h"

enum {
  SW_01 = 23,
  SW_02,
  SW_03,
  SW_04,
  SW_05,
  SW_06,
  SW_07,
  SW_08,
  SW_09,
  SW_10
};

static const u8 buttonmap[] = BUTTON_MATRIX;

extern uint32_t ADDON_Handle_ExtraSwitches(u32 result);
void Initialize_ButtonMatrix()
{
    /* Enable AFIO */
    rcc_periph_clock_enable(get_rcc_from_pin(BUTTON_MATRIX_ROW_OD));
    rcc_periph_clock_enable(get_rcc_from_pin(BUTTON_MATRIX_COL_PU));

    GPIO_setup_output(BUTTON_MATRIX_ROW_OD, OTYPE_OPENDRAIN);
    GPIO_pin_set(BUTTON_MATRIX_ROW_OD);
    GPIO_setup_input(BUTTON_MATRIX_COL_PU, ITYPE_PULLUP);
}

u32 ScanButtons()
{
    unsigned idx = 0;
    u32 result = 0;
    GPIO_pin_set(BUTTON_MATRIX_ROW_OD);
    for (unsigned r = 0; r < 16; r++) {
        struct mcu_pin rpin = {BUTTON_MATRIX_ROW_OD.port, BUTTON_MATRIX_ROW_OD.pin & (1 << r)};
        if (!rpin.pin)
            continue;
        GPIO_pin_clear(rpin);
        u32 but = GPIO_pin_get(BUTTON_MATRIX_COL_PU);
        GPIO_pin_set(rpin);
        for (unsigned c = 0; c < 16; c++) {
            u32 cpin =  BUTTON_MATRIX_COL_PU.pin & (1 << c);
            if (!cpin)
                continue;
            if (!(but & cpin)) {
                result |= 1 << (buttonmap[idx] - 1);
            }
            idx++;
        }
    }
#if defined(EXTRA_SWITCHES)
        result = ADDON_Handle_ExtraSwitches(result);
#endif
    return result;
}
