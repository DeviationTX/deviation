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
#include "telemetry.h"

#include "_utx_pactl.c"
#include "_utx_multimod.c"

extern volatile u8 ppmin_num_channels;     //  the ppmin_num_channels for mixer.c 
struct Telemetry Telemetry;

//Main routine for PPM module
//Code here translates PPM into protocol commands entirely in the UniversalTX module
//Control is via bluetooth
void utx_ppm()
{
    SetModule(TX_MODULE_LAST);
    BT_Initialize();

    SPI_ProtoInit();
    SPI_ProtoMasterSlaveInit(NULL);  //Switch SPI to master mode
    PPMin_TIM_Init();

    printf("Power Up\n");
    printf("A7105: %s\n", A7105_Reset() ? "Found" : "Not found");
    printf("NRF24L01: %s\n", NRF24L01_Reset() ? "Found" : "Not found");
    printf("CC2500: %s\n", CC2500_Reset() ? "Found" : "Not found");
    printf("CYRF6936: %s\n", CYRF_Reset() ? "Found" : "Not found");
    //BT_Test();
    printf("Done\n");
    PPMin_Start();
    //BT_Test();
    Model.ppmin_centerpw = 1100;
    Model.ppmin_deltapw = 400;
    Model.proto_opts[0] = 3; //Radio => CYRF6936
    Model.proto_opts[1] = 7; //Tx Power => 0
    Model.proto_opts[2] = 20; //RF Channel => 1
    Model.proto_opts[3] = 10; //Rate(ms) => 20
    TESTRF_Cmds(PROTOCMD_INIT);
    CLOCK_SetMsecCallback(LOW_PRIORITY, LOW_PRIORITY_MSEC);
    int i = 0;
    while (1) {
        if (priority_ready & (1 << LOW_PRIORITY)) {
            priority_ready = 0;
            BT_HandleInput();
            i = (i + 1) & 0x7F;
            if(i == 0) {
                printf("#Ch: %d", ppmin_num_channels);
                if (ppmin_num_channels) {
                   for(int j = 0; j < ppmin_num_channels; j++) {
                       printf(" %d:%d", j, Channels[j]);
                   }
                }
                printf("\n");
            }
        }
    }
}

void SetModule(unsigned module) {
    Model.module =  module;
}
