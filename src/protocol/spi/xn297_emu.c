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

//static u8 packet[32];

u8 xn297_crc;
u8 xn297_scramble_enabled; // should be 1 by default ... but can't init static in modular 

const u8 xn297_scramble[] = {
    0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
    0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
    0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc,
    0x1b, 0x5d, 0x19, 0x10, 0x24, 0xd3, 0xdc, 0x3f,
    0x8e, 0xc5, 0x2f};
    
const u16 xn297_crc_xorout_scrambled[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C,
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401,
    0x2138, 0x129F, 0xB3A0, 0x2988};
    
const u16 xn297_crc_xorout[] = {
    0x0000, 0x3d5f, 0xa6f1, 0x3a23, 0xaa16, 0x1caf,
    0x62b2, 0xe0eb, 0x0821, 0xbe07, 0x5f1a, 0xaf15,
    0x4f0a, 0xad24, 0x5e48, 0xed34, 0x068c, 0xf2c9,
    0x1852, 0xdf36, 0x129d, 0xb17c, 0xd5f5, 0x70d7,
    0xb798, 0x5133, 0x67db, 0xd94e};

#if defined(__GNUC__) && defined(__ARM_ARCH_ISA_THUMB) && (__ARM_ARCH_ISA_THUMB==2)
// rbit instruction works on cortex m3
static u32 __RBIT_(u32 in)
{
    u32 out=0;
    __asm volatile ("rbit %0, %1" : "=r" (out) : "r" (in) );
    return(out);
}

u8 bit_reverse(u8 b_in)
{
    return __RBIT_( (unsigned int) b_in)>>24;
}
#else
u8 bit_reverse(u8 b_in)
{
    u8 b_out = 0;
    for (int i = 0; i < 8; ++i) {
        b_out = (b_out << 1) | (b_in & 1);
        b_in >>= 1;
    }
    return b_out;
}
#endif

static const uint16_t polynomial = 0x1021;

u16 crc16_update(u16 crc, u8 a, u8 bits)
{
    crc ^= a << 8;
    while (bits--) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ polynomial;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

void XN297_SetScrambledMode(const u8 mode)
{
    xn297_scramble_enabled = mode;
}
