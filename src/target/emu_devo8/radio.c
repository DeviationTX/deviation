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

#include "target.h"
#include "protocol/interface.h"

void A7105_Initialize()
{
}

void A7105_WriteReg(u8 addr, u8 value)
{
    (void)addr;
    (void)value;
    //send(addr);
    //send(value);
}

void A7105_WriteData(u8 *data, u8 len, u8 channel)
{
    (void)data;
    (void)len;
    (void)channel;
}

u8 A7105_ReadReg(u8 addr)
{
    //send(0x40 | addr)
    //return read();
    return addr;
}

void A7105_Reset()
{
    //send(0x00);
    //send(0x00);
}

void A7105_WriteID(u32 id)
{
    (void)id;
    //send(0x06);
    //send(id >> 24);
    //send(0xff & (id >> 16));
    //send(0xff & (id >> 8));
    //send(0xff & id);
}

void A7105_Strobe(enum A7105_State state)
{
    (void)state;
    //send(state)//
}
