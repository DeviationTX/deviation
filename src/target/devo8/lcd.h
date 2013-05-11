#ifndef _DEVO8_LCD_H_
#define _DEVO8_LCD_H_

#include <libopencm3/stm32/fsmc.h>

#define LCD_REG_ADDR  ((uint32_t)FSMC_BANK1_BASE)    /* Register Address */
#define LCD_DATA_ADDR  ((uint32_t)FSMC_BANK1_BASE + 0x20000)  /* Data Address */

#define LCD_REG  *(volatile uint16_t *)(LCD_REG_ADDR)
#define LCD_DATA *(volatile uint16_t *)(LCD_DATA_ADDR)

struct lcdtype {
    void (*set_pos)(unsigned int x0, unsigned int y0);
    void (*draw_start)(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);
    void (*sleep)();
};
extern const struct lcdtype *disp_type;
extern u8 screen_flip;

extern void lcd_cmd(uint8_t addr, uint8_t data);

extern void ili9341_init();
extern void hx8347_init();
#endif
