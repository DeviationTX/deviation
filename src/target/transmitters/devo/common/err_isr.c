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
#include <libopencm3/cm3/nvic.h>

/*
    asm(
"    TST LR, #4\n"
"    ITE EQ\n"
"    MRSEQ R0, MSP\n"
"    MRSNE R0, PSP\n"
"    B fault_handler_c\n");
*/

void __attribute__((__used__)) hard_fault_handler()
{
    asm(
"    MRS   R0, MSP\n"
"    MOVS  R1, #0\n"
"    B fault_handler_c\n");
}

void __attribute__((__used__)) exti2_isr()
{
    asm(
"    MRS   R0, MSP\n"
"    MOVS  R1, #1\n"
"    B fault_handler_c\n");
}