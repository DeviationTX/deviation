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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
//#include "../common/devo/devo.h"
#include "config/tx.h"
//#include "protocol/interface.h"
//#include <stdlib.h>
#include "target/drivers/mcu/stm32/spi.h"
#include "target/drivers/mcu/stm32/rcc.h"

void SPI_ProtoInit()
{
    _spi_init(PROTO_SPI_CFG);
    if (HAS_PIN(PROTO_RST_PIN))
        GPIO_pin_clear(PROTO_RST_PIN);
    GPIO_pin_clear(PROTO_RST_PIN);
    GPIO_pin_set(PROTO_SPI.csn);
}

void MCU_InitModules()
{
}

int MCU_SetPin(struct mcu_pin *port, const char *name) {
//    (void)port;
    (void)name;
    port->port = 1;
    return 1;
}
