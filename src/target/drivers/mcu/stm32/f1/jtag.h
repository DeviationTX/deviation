#ifndef _DTX_STM32F1_JTAG_H_
#define _DTX_STM32F1_JTAG_H_
#include "common.h"
#include "../rcc.h"

static inline void DisableJTAG()
{
    rcc_periph_clock_enable(RCC_AFIO);
    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
    gpio_set_mode(GPIO_BANK_JTMS_SWDIO, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTMS_SWDIO);
    gpio_set(GPIO_BANK_JTMS_SWDIO, GPIO_JTMS_SWDIO);
    gpio_set_mode(GPIO_BANK_JTCK_SWCLK, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTCK_SWCLK);
    gpio_set(GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK);
}

#endif  // _DTX_STM32F1_JTAG_H_
