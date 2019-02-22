#ifndef _DEVOF7_HARDWARE_H_
#define _DEVOF7_HARDWARE_H_

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }

#define LCD_VIDEO_CS0 ((struct mcu_pin){GPIOA, GPIO0})
#define LCD_VIDEO_CS1 ((struct mcu_pin){GPIOA, GPIO8})
#define LCD_VIDEO_CS2 ((struct mcu_pin){GPIOA, GPIO15})
#define LCD_VIDEO_PWR ((struct mcu_pin){GPIOB, GPIO9})

#endif  // _DEVOF7_HARDWARE_H_
