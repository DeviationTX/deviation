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
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/iwdg.h>

#include "common.h"
#include <fcntl.h>

/* This code will write a stack-trace to disk should a fault happen
 * Since when it runs, memory may be corrupted, we want to use as
 * few functions or static memory addresses as possible.
 * The code bypasses the entire file I/O system, and writes directly
 * to the disk address.
 */
extern int disk_writep (const u8*, u32);
//Memory start is constant for all Tx
#define MEMORY_START  ((unsigned int *)0x20000000)
#define MEMORY_END    (&_stack)
#define NO_VALUE 0xFABACEAE //Unique ID

#define NONE      0
#define BACKTRACE 1
#define DUMP      2

#define MEMORY_DUMP BACKTRACE
extern unsigned _stack; //Defined in devo.ld

/*
    asm(
"    TST LR, #4\n"
"    ITE EQ\n"
"    MRSEQ R0, MSP\n"
"    MRSNE R0, PSP\n"
"    B fault_handler_c\n");
*/

void hard_fault_handler()
{
    asm(
"    MRS   R0, MSP\n"
"    MOVS  R1, #0\n"
"    B fault_handler_c\n");
}
void exti2_isr()
{
    asm(
"    MRS   R0, MSP\n"
"    MOVS  R1, #1\n"
"    B fault_handler_c\n");
}
static u32 debug_addr = 0;
void write_byte(u8 x) {
    if(debug_addr)
        disk_writep(&x, 1);
    usart_send_blocking(USART1,(x));
}
void write_long(unsigned int val)
{
    for(int i = 7; i >= 0; i--) {
        u8 v = 0x0f & (val >> (4 * i));
        if (v < 10)
            write_byte('0' + v);
        else
            write_byte('a' + v - 10);
    }
}

void fault_printf(const char *str, unsigned int val)
{
    while(*str) {
        write_byte(*str);
        str++;
    }
    if (val != NO_VALUE) { //Unique ID
        write_long(val);
    }
    write_byte('\r');
    write_byte('\n');
}
// From Joseph Yiu, minor edits by FVH
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file xxx.s
void fault_handler_c (unsigned int * hardfault_args, unsigned int fault_type)
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
  if (fault_type) {
      fault_printf ("\n\n[Soft fault]", NO_VALUE);
  } else {
      fault_printf ("\n\n[Hard fault]", NO_VALUE);
  }
  fault_printf (DeviationVersion, NO_VALUE);
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
  fault_printf ("Top of Stack:", (unsigned int)(&_stack));
  fault_printf ("Stack Detect:", (unsigned int)hardfault_args);

#if MEMORY_DUMP == BACKTRACE
  int count = 0;
  unsigned int *ptr = hardfault_args;
  fault_printf ("Backtrace:", NO_VALUE);
  while (ptr < (&_stack) && count < 20) {
    //iwdg_reset();
    if (
        ((*ptr & 0xFFF00001) == 0x08000001)
#ifdef MODULAR
        || ((*ptr & 0xFFFFF001) == (MODULAR+1))
#endif
       ) {
        //This looks like it may be a return address
        write_long((unsigned int)ptr);
        fault_printf(" : ", *ptr);
        count++;
    }
    ptr = (unsigned int *)((unsigned int)ptr + 1);
  }  
  fault_printf("Done", NO_VALUE);
#elif MEMORY_DUMP == DUMP
  if(debug_addr) {
      fault_printf ("Memory Dump:", NO_VALUE);
      for (unsigned int *i = MEMORY_START; i < MEMORY_END; i++) {
          disk_writep((const u8 *)i, 4);
          iwdg_reset();
      }
  }
#endif
  while (1);
}

void init_err_handler() {
#if defined USE_DEVOFS && USE_DEVOFS == 1
    //This is a hack to get the memory address of a file
    //we can't use 'fopen' because it masks the structure we need
    FATFS fat;
    if(fs_mount(&fat) != FR_OK)
        return;
    if(fs_open("errors.txt", O_WRONLY) != FR_OK) {
        fs_mount(0);
        return;
    }
    fs_maximize_file_size();
    if(fs_lseek(1) != FR_OK) {  //Seeking to a non-zero address causes petitfat to calculatethe disk-address
        fs_mount(0);
        return;
    }
    debug_addr = fat.dsect;
    fs_mount(0);
#endif
}
