/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include "config/model.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include "ports.h"

struct model Model;
struct Transmitter Transmitter;

int main(void)
{
    PWR_Init();
    PACTL_Init();
    UART_Initialize();
    BT_Initialize();
    SPI_ProtoInit();

    printf("Power Up\n");
    printf("NRF24L01: %s\n", NRF24L01_Reset() ? "Found" : "Not found");
    printf("A7105: %s\n", A7105_Reset() ? "Found" : "Not found");
    printf("CC2500: %s\n", CC2500_Reset() ? "Found" : "Not found");
    printf("CYRF6936: %s\n", CYRF_Reset() ? "Found" : "Not found");
printf("Here\n");
    BT_Test();        
    printf("Done\n");
    while (1) {
        BT_HandleInput();
    }

    return 0;
}
