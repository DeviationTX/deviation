#ifndef _DTX_STM32_SPI_H_
#define _DTX_STM32_SPI_H_

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
        GPIO_setup_input(spi_cfg.miso, ITYPE_FLOAT);
    }
    // spi_reset(spi_cfg.spi);
    spi_init_master(spi_cfg.spi,
                    spi_cfg.rate,
                    spi_cfg.cpol,
                    spi_cfg.cpha,
                    spi_cfg.dff,
                    spi_cfg.endian);
    spi_enable_software_slave_management(spi_cfg.spi);
    spi_set_nss_high(spi_cfg.spi);
    spi_enable(spi_cfg.spi);
}

#endif  // _DTX_STM32_SPI_H_
