#ifndef _DEVOF7_HARDWARE_H_
#define _DEVOF7_HARDWARE_H_

#define LCD_VIDEO_CS0 ((struct mcu_pin){GPIOA, GPIO0})
#define LCD_VIDEO_CS1 ((struct mcu_pin){GPIOA, GPIO8})
#define LCD_VIDEO_CS2 ((struct mcu_pin){GPIOA, GPIO15})
#define LCD_VIDEO_PWR ((struct mcu_pin){GPIOB, GPIO9})

#endif  // _DEVOF7_HARDWARE_H_
