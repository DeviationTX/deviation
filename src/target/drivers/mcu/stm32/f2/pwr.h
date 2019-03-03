#ifndef _DTX_STM32F1_PWR_H_
#define _DTX_STM32F1_PWR_H_

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/scb.h>

static const struct rcc_clock_scale hse_12mhz_3v3 =
{
    /* sysclk frequency = XTAL * plln / pllm / pllp = 60 MHz */
    /* USB clock        = XTAL * plln / pllm / pllq = 48 MHz */
    .pllm = 12,
    .plln = 240,
    .pllp = 4,
    .pllq = 5,                       // USB @ 48MHz
    .hpre = RCC_CFGR_HPRE_DIV_NONE,  // AHB @ 60MHz
    .ppre1 = RCC_CFGR_PPRE_DIV_2,
    .ppre2 = RCC_CFGR_PPRE_DIV_NONE,
    .flash_config = FLASH_ACR_DCEN | FLASH_ACR_ICEN |
                            FLASH_ACR_LATENCY_1WS,  // 1WS @HCLK=60MHz, 2WS <=90, 3WS <=120
    .apb1_frequency = 30000000,
    .apb2_frequency = 60000000,
};

static inline void _pwr_init()
{
    // SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT;  // sleep immediate on WFI
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT;  // sleep immediate on WFI
    rcc_clock_setup_hse_3v3(&hse_12mhz_3v3);
    rcc_ahb_frequency = 60000000;
}

static inline void _pwr_shutdown()
{
}

#endif  // _DTX_STM32F1_PWR_H_
