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

const char UTX_Version[33] = HGVERSION;
volatile u8 priority_ready;

extern void utx_devo();
extern void utx_ppm();

//Main routine for bidi-USART
//Code here uses PPM pin as bidi USART.  This is intended for using the X9D with Deviation
//Protocol parameters are from the transmitter, but the protocol code itself is via the UniversalTx module
void utx_uart()
{
}

//Main routine for USB control
//Code here receives commands from USB interface, but is otherwise similar to the USART mode above
void utx_usb()
{
}

int main(void)
{
    PWR_Init();
    CLOCK_Init();
    PACTL_Init();
    UART_Initialize();

    TARGET();
    return 0;
}
