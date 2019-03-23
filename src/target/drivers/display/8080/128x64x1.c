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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/fsmc.h"

#define LCD_CONTRAST_FUNC(contrast) (0x20 + contrast * 0xC / 10)

#define LCD_CMD_ADDR ((uint32_t)FSMC_BANK1_BASE) /* Register Address */
#define LCD_DATA_ADDR ((uint32_t)FSMC_BANK1_BASE + 0x10000) /* Data Address */

#define LCD_CMD *(volatile uint8_t *)(LCD_CMD_ADDR)
#define LCD_DATA *(volatile uint8_t *)(LCD_DATA_ADDR)

inline static void LCD_Cmd(unsigned cmd)
{
    LCD_CMD = cmd;
}

inline static void LCD_Data(unsigned data)
{
    LCD_DATA = data;
}
#define LCD_CONTRAST_FUNC(contrast) (0x20 + contrast * 0xC / 10)

static void lcd_init_ports()
{
    _fsmc_init(
        8,
        0x10000,  /*only bit 16 of addr */
        FSMC_NOE | FSMC_NWE | FSMC_NE1,  /* Not connected */
        FSMC_BANK1,
        /* Normal mode, write enable, 8 bit access, SRAM, bank enabled */
        FSMC_BCR_FACCEN | FSMC_BCR_WREN | FSMC_BCR_MBKEN,
        /* Data Setup > 90ns, Address Setup = 2xHCLK to ensure no output collision in 6800
           mode since LCD E and !CS always active */
        FSMC_BTR_DATASTx(7) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(2),
        0);
}

#include "../spi/128x64x1_common.h"
