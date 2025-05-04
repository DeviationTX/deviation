#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "../common/devo/devo.h"

void PWR_Init(void)
{
    SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT; //sleep immediate on WFI
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    /* Enable GPIOA and GPIOE so we can blink LEDs! */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);

    /* Disable SWD - SWDIO PA13 is used for Green LED */
    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;

    /* Green LED - pin PA13 */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

    /* Pin to ground - LED lit */
    gpio_clear(GPIOA, GPIO13);

    /* Red LED - pin PE1 */
    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO1);

    /* Pin to Vcc - LED down */
    gpio_set(GPIOE, GPIO1);
}

/* AT9 uses a hard power switch */
int PWR_CheckPowerSwitch()
{
    return 0;
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    BACKLIGHT_Brightness(0);
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);
    rcc_wait_for_osc_ready(HSI);
    while(1) ;
}

void PWR_Sleep()
{
    //asm("wfi");
}

/* Return milivolts */
unsigned PWR_ReadVoltage(void)
{
    u32 v = adc_array_raw[NUM_ADC_CHANNELS-1];
    /* Multily the above by 1000 to get milivolts */
    v = v * VOLTAGE_NUMERATOR / 100 + VOLTAGE_OFFSET;
    return v;
}
