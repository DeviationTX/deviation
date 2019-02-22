#ifndef _DTX_STM32F1_PWR_H_
#define _DTX_STM32F1_PWR_H_

static inline void _pwr_init()
{
    SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT;  // sleep immediate on WFI
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}

static inline void _pwr_shutdown()
{
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);
    rcc_wait_for_osc_ready(RCC_HSI);
}

#endif  // _DTX_STM32F1_PWR_H_
