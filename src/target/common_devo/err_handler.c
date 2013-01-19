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
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/f1/scb.h>
#include "common.h"
#include "petit_fat.h"
#include "petit_io.h"

void hard_fault_handler()
{
    asm(
"    TST LR, #4\n"
"    ITE EQ\n"
"    MRSEQ R0, MSP\n"
"    MRSNE R0, PSP\n"
"    B hard_fault_handler_c\n");
}
static u32 debug_addr = 0;
void write_byte(u8 x) {
    if(debug_addr)
        disk_writep(&x, 1);
    usart_send_blocking(USART1,(x));
}
void fault_printf(char *str, unsigned int val)
{
    while(*str) {
        write_byte(*str);
        str++;
    }
    for(int i = 7; i >= 0; i--) {
        u8 v = 0x0f & (val >> (4 * i));
        if (v < 10)
            write_byte('0' + v);
        else
            write_byte('a' + v - 10);
    }
    write_byte('\r');
    write_byte('\n');
}
// From Joseph Yiu, minor edits by FVH
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file xxx.s
void hard_fault_handler_c (unsigned int * hardfault_args)
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
 
  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);
 
  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);

  if(debug_addr)
      disk_writep(0, debug_addr); 
  fault_printf ("\n\n[Hard fault handler", 0);
  fault_printf ("R0 = ", stacked_r0);
  fault_printf ("R1 = ", stacked_r1);
  fault_printf ("R2 = ", stacked_r2);
  fault_printf ("R3 = ", stacked_r3);
  fault_printf ("R12 = ", stacked_r12);
  fault_printf ("LR [R14] (subroutine call return address) = ", stacked_lr);
  fault_printf ("PC [R15] (program counter) = ", stacked_pc);
  fault_printf ("PSR = ", stacked_psr);
  fault_printf ("BFAR = ", (*((volatile unsigned long *)(0xE000ED38))));
  fault_printf ("CFSR = ", (*((volatile unsigned long *)(0xE000ED28))));
  fault_printf ("HFSR = ", (*((volatile unsigned long *)(0xE000ED2C))));
  fault_printf ("DFSR = ", (*((volatile unsigned long *)(0xE000ED30))));
  fault_printf ("AFSR = ", (*((volatile unsigned long *)(0xE000ED3C))));
  fault_printf ("SCB_SHCSR = ", SCB_SHCSR);
  fault_printf ("impure_ptr = ", (*((volatile unsigned long *)(0x20000148))));
  fault_printf ("             ", (*((volatile unsigned long *)(0x2000014c))));
  fault_printf ("             ", (*((volatile unsigned long *)(0x20000150))));
  fault_printf ("             ", (*((volatile unsigned long *)(0x20000154))));
  
 
  while (1);
}

void init_err_handler() {
    //This is a hack to get the memory address of a file
    //we can't use 'fopen' because it masks the structure we need
    FATFS fat;
    if(pf_mount(&fat) != FR_OK)
        return;
    if(pf_open("errors.txt") != FR_OK) {
        pf_mount(0);
        return;
    }
    pf_maximize_file_size();
    if(pf_lseek(1) != FR_OK) {  //Seeking to a non-zero address causes petitfat to calculatethe disk-address
        pf_mount(0);
        return;
    }
    debug_addr = fat.dsect;
    pf_mount(0);
}
