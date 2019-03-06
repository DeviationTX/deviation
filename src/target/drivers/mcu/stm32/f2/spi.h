#ifndef _DTX_STM32F2_SPI_H_
#define _DTX_STM32F2_SPI_H_

#include "rcc.h"

static void _spi_init(struct spi_config spi_cfg)
{
    rcc_periph_clock_enable(get_rcc_from_port(spi_cfg.spi));
    rcc_periph_clock_enable(get_rcc_from_pin(spi_cfg.sck));  // Assume sck, mosi, miso all on same port
    GPIO_setup_output_af(spi_cfg.sck, OTYPE_PUSHPULL, spi_cfg.spi);
    if (HAS_PIN(spi_cfg.mosi)) {
        GPIO_setup_output_af(spi_cfg.mosi, OTYPE_PUSHPULL, spi_cfg.spi);
    }
    if (HAS_PIN(spi_cfg.miso)) {
        GPIO_setup_input_af(spi_cfg.miso, ITYPE_FLOAT, spi_cfg.spi);  // This is different from the F1
    }
    // spi_reset(spi_cfg.spi);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
    ctassert(spi_cfg.rate < 8, use_SPI_CR1_BR_FPCLK_DIV_xx_not_SPI_CR1_BAUDRATE_FPCLK_DIV_2);
#pragma GCC diagnostic pop
    spi_init_master(spi_cfg.spi,
                    spi_cfg.rate << 3,  // spi_init_master takes a different value than spi_set_baudrate_prescalar
                    spi_cfg.cpol,
                    spi_cfg.cpha,
                    spi_cfg.dff,
                    spi_cfg.endian);
    spi_enable_software_slave_management(spi_cfg.spi);
    spi_set_nss_high(spi_cfg.spi);
    spi_enable(spi_cfg.spi);
}

#endif  // _DTX_STM32F2_SPI_H_
