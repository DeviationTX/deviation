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
/*
unsigned char __attribute__((section(".crc"))) CRC[] = {
        0x66,0x58,0xC4,0x19,
        0x04,0xA6,0xBB,0xD1,
        0x55,0x52,0x98,0x28,
        0x15,0xAC,0x87,0xA6,
        0xF2,0x86,0x42,0x5E };
*/
unsigned char __attribute__((section(".crc"))) CRC[] = {
        0x63,0x4A,0x73,0xC5,
        0x03,0xB4,0x02,0xB9,
        0xB4,0x17,0x53,0xD1,
        0xB2,0xE7,0x22,0xCD,
        0x22,0x7C,0x41,0xAE };
