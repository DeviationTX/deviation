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
#include "target/drivers/mcu/stm32/spi.h"

void MMC_Init()
{
    rcc_periph_clock_enable(get_rcc_from_pin(MMC_SPI.csn));
    GPIO_setup_output(MMC_SPI.csn, OTYPE_PUSHPULL);
    GPIO_pin_set(MMC_SPI.csn);
    if (HAS_PIN(MMC_PRESENT)) {
        rcc_periph_clock_enable(get_rcc_from_pin(MMC_PRESENT));
        GPIO_setup_input(MMC_PRESENT, ITYPE_PULLUP);
    }
    if (HAS_PIN(MMC_PROTECT)) {
        rcc_periph_clock_enable(get_rcc_from_pin(MMC_PROTECT));
        GPIO_setup_input(MMC_PROTECT, ITYPE_PULLUP);
    }
    _spi_init(MMC_SPI_CFG);
    rcc_periph_clock_enable(get_rcc_from_port(MMC_RX_DMA.dma));
}
