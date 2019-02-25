#ifndef _DTX_STM32_FSMC_H_
#define _DTX_STM32_FSMC_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>

#define FSMC_NOE   (1 << 0)
#define FSMC_NWE   (1 << 1)
#define FSMC_NWAIT (1 << 2)
#define FSMC_NE1   (1 << 3)
#define FSMC_NE2   (1 << 4)
#define FSMC_NE3   (1 << 5)
#define FSMC_NE4   (1 << 6)
#define FSMC_NADV  (1 << 7)
#define FSMC_NBL0  (1 << 8)
#define FSMC_NBL1  (1 << 9)

#define FSMC_BANK1 0
#define FSMC_BANK2 1
#define FSMC_BANK3 2
#define FSMC_BANK4 3

static inline void _fsmc_init(uint32_t data_len, uint32_t address_mask, uint32_t ctrl_bits,
                             uint32_t bank, uint32_t bcr, uint32_t btr, uint32_t bwtr)
{
    //                      D0       D1       D2      D3
    unsigned data_portd = GPIO14 | GPIO15 | GPIO0 | GPIO1 | (data_len == 16 ?
    //                      D13      D14     D15
                          (GPIO8 | GPIO9  | GPIO10) : 0);

    //                      D4       D5       D6      D7
    unsigned data_porte =  GPIO7  | GPIO8  | GPIO9  | GPIO10 | (data_len == 16 ?
    //                      D8       D9       D10      D11       D12
                          (GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15) : 0);

    unsigned addr_portf = (address_mask & (1 << 0)  ? GPIO0  : 0) |  /* A0 */
                          (address_mask & (1 << 1)  ? GPIO1  : 0) |  /* A1 */
                          (address_mask & (1 << 2)  ? GPIO2  : 0) |  /* A2 */
                          (address_mask & (1 << 3)  ? GPIO3  : 0) |  /* A3 */
                          (address_mask & (1 << 4)  ? GPIO4  : 0) |  /* A4 */
                          (address_mask & (1 << 5)  ? GPIO5  : 0) |  /* A5 */
                          (address_mask & (1 << 6)  ? GPIO12 : 0) |  /* A6 */
                          (address_mask & (1 << 7)  ? GPIO13 : 0) |  /* A7 */
                          (address_mask & (1 << 8)  ? GPIO14 : 0) |  /* A8 */
                          (address_mask & (1 << 9)  ? GPIO15 : 0);   /* A9 */

    unsigned addr_portg = (address_mask & (1 << 10) ? GPIO0  : 0) |  /* A10 */
                          (address_mask & (1 << 11) ? GPIO1  : 0) |  /* A11 */
                          (address_mask & (1 << 12) ? GPIO2  : 0) |  /* A12 */
                          (address_mask & (1 << 13) ? GPIO3  : 0) |  /* A13 */
                          (address_mask & (1 << 14) ? GPIO4  : 0) |  /* A14 */
                          (address_mask & (1 << 15) ? GPIO5  : 0);   /* A15 */

    unsigned addr_portd = (address_mask & (1 << 16) ? GPIO11 : 0) |  /* A16 */
                          (address_mask & (1 << 17) ? GPIO12 : 0) |  /* A17 */
                          (address_mask & (1 << 18) ? GPIO13 : 0);   /* A18 */

    unsigned addr_porte = (address_mask & (1 << 19) ? GPIO3  : 0) |  /* A19 */
                          (address_mask & (1 << 20) ? GPIO4  : 0) |  /* A20 */
                          (address_mask & (1 << 21) ? GPIO5  : 0) |  /* A21 */
                          (address_mask & (1 << 22) ? GPIO6  : 0) |  /* A22 */
                          (address_mask & (1 << 23) ? GPIO2  : 0);   /* A23 */

    addr_portg |=         (address_mask & (1 << 24) ? GPIO13 : 0) |  /* A24 */
                          (address_mask & (1 << 25) ? GPIO14 : 0);   /* A25 */

    unsigned ctrl_portd = (ctrl_bits &  FSMC_NOE   ? GPIO4 : 0) |  /* NOE */
                          (ctrl_bits &  FSMC_NWE   ? GPIO5 : 0) |  /* NWE */
                          (ctrl_bits &  FSMC_NE1   ? GPIO7 : 0);   /* NE1 */

    unsigned ctrl_portg = (ctrl_bits &  FSMC_NE2   ? GPIO9  : 0) |  /* NE2 */
                          (ctrl_bits &  FSMC_NE3   ? GPIO10 : 0) |  /* NE3 */
                          (ctrl_bits &  FSMC_NE4   ? GPIO12 : 0);   /* NE4 */

    unsigned ctrl_portb = (ctrl_bits &  FSMC_NADV  ? GPIO7  : 0);   /* NADV */

    unsigned ctrl_porte = (ctrl_bits &  FSMC_NBL0  ? GPIO0  : 0) |  /* NBL0 */
                          (ctrl_bits &  FSMC_NBL1  ? GPIO1 : 0);    /* NBL1 */

    unsigned inp_portd  = (ctrl_bits &  FSMC_NWAIT ? GPIO6 : 0);    /* NWAIT */

    rcc_periph_clock_enable(RCC_FSMC);
    if (data_portd || addr_portd | ctrl_portd | inp_portd) {
        rcc_periph_clock_enable(RCC_GPIOD);
        if (data_portd || addr_portd | ctrl_portd) {
            GPIO_setup_output_af((struct mcu_pin){GPIOD, data_portd | addr_portd | ctrl_portd},
                                 OTYPE_PUSHPULL, AF_FSMC);
        }
        if (inp_portd) {
            GPIO_setup_input_af((struct mcu_pin){GPIOD, inp_portd}, ITYPE_PULLUP, AF_FSMC);
        }
    }
    if (data_porte || addr_porte | ctrl_porte) {
        rcc_periph_clock_enable(RCC_GPIOE);
        GPIO_setup_output_af((struct mcu_pin){GPIOE, data_porte | addr_porte | ctrl_porte},
                             OTYPE_PUSHPULL, AF_FSMC);
    }
    if (addr_portf) {
        rcc_periph_clock_enable(RCC_GPIOF);
        GPIO_setup_output_af((struct mcu_pin){GPIOF, addr_portf},
                             OTYPE_PUSHPULL, AF_FSMC);
    }
    if (addr_portg | ctrl_portg) {
        rcc_periph_clock_enable(RCC_GPIOG);
        GPIO_setup_output_af((struct mcu_pin){GPIOG, addr_portg | ctrl_portg},
                             OTYPE_PUSHPULL, AF_FSMC);
    }
    if (ctrl_portb) {
        rcc_periph_clock_enable(RCC_GPIOB);
        GPIO_setup_output_af((struct mcu_pin){GPIOB, ctrl_portb},
                             OTYPE_PUSHPULL, AF_FSMC);
    }
    /* Extended mode, write enable, 16 bit access, bank enabled */
    FSMC_BCR(bank) = (1 << 7) | bcr;  // Bit 7 is reserved and must be set
    FSMC_BTR(bank) = btr;
    if (bwtr)
        FSMC_BWTR(bank) = bwtr;
}
#endif  // _DTX_STM32_FSMC_H_
