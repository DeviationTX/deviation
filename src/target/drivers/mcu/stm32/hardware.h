#ifndef _DTX_STM32_HARDWARE_DEFAULT_H_
#define _DTX_STM32_HARDWARE_DEFAULT_H_

// Add additional callbacks
#ifndef TARGET_PRIORITY
    #define TARGET_PRIORITY TIMER_SOUND
#endif


#ifndef HAS_JTAG
    // HAS_JTAG enables the JTAG pins.  Otherwise, allow use as GPIO
    #define HAS_JTAG 0
#endif

#ifndef PWR_ENABLE_PIN
    #define PWR_ENABLE_PIN NULL_PIN
#endif

#ifndef PWR_SWITCH_PIN
    #define PWR_SWITCH_PIN NULL_PIN
#endif

#if !defined(HAS_POWER_SWITCH)
    #if defined PWR_SWITCH_PIN
        #define HAS_POWER_SWITCH 1
    #else
        #define HAS_POWER_SWITCH 0
        #define PWR_SWITCH_PIN   NULL_PIN
    #endif
#endif


#ifndef FLASH_SPI
    #define FLASH_SPI ((struct spi_csn){ .spi = 0 })
#endif

#ifndef LCD_SPI
    #define LCD_SPI ((struct spi_csn){ .spi = 0 })
#endif


#ifndef SPI1_CFG
    #define SPI1_CFG ((struct spi_config){ .spi = 0 })
#endif

#ifndef SPI2_CFG
    #define SPI2_CFG ((struct spi_config){ .spi = 0 })
#endif

#ifndef SPI3_CFG
    #define SPI3_CFG ((struct spi_config){ .spi = 0 })
#endif

#ifndef UART_CFG
    #define UART_CFG ((struct uart_config){ .uart = 0})
#endif

#endif  // _DTX_STM32_HARDWARE_DEFAULT_H_
