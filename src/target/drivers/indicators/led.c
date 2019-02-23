#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/pwr.h"
#include "target/drivers/mcu/stm32/jtag.h"

void LED_Init()
{
    if (HAS_PIN(LED_STATUS_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(LED_STATUS_PIN));
        GPIO_setup_output(LED_STATUS_PIN, OTYPE_PUSHPULL);
        GPIO_pin_clear(LED_STATUS_PIN);
    }

    if (HAS_PIN(LED_RF_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(LED_RF_PIN));
        GPIO_setup_output(LED_RF_PIN, OTYPE_PUSHPULL);
        GPIO_pin_clear(LED_RF_PIN);
    }

    if (HAS_PIN(LED_STORAGE_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(LED_STORAGE_PIN));
        GPIO_setup_output(LED_STORAGE_PIN, OTYPE_PUSHPULL);
        GPIO_pin_clear(LED_STORAGE_PIN);
    }
}

void LED_Status(u8 on)
{
    if (HAS_PIN(LED_STATUS_PIN)) {
        if (on)
            GPIO_pin_clear(LED_STATUS_PIN);
        else
            GPIO_pin_set(LED_STATUS_PIN);
    }
}

void LED_RF(u8 on)
{
    if (HAS_PIN(LED_RF_PIN)) {
        if (on)
            GPIO_pin_clear(LED_RF_PIN);
        else
            GPIO_pin_set(LED_RF_PIN);
    }
}

void LED_Storage(u8 on)
{
    if (HAS_PIN(LED_STORAGE_PIN)) {
        if (on)
            GPIO_pin_clear(LED_STORAGE_PIN);
        else
            GPIO_pin_set(LED_STORAGE_PIN);
    }
}
