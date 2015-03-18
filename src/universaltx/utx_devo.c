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

static void spi_cb(u8 *ptr, unsigned length);

//Main routine for acting as a DEVO slave.
//Code here converts universal TX into a simple switch and Devo TX does all the work
void utx_devo()
{
    SPI_ProtoMasterSlaveInit(&spi_cb);  //Switch SPI to slave mode
    while(1) ;
}

//The following is called whenever an SPI transfer targetted at the UniversalTx is completed
//It is called in interrupt context, so keep it short!
void spi_cb(u8 *ptr, unsigned length)
{
    int module = TX_MODULE_LAST;
    if (length < 1)
        return;
    int command = ptr[0];
    if (command == CHANGE_MODULE) {
        if (length != 2)
            return;
        module = ptr[1];
    }
    MULTIMOD_SwitchCommand(module, command);
}


