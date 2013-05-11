#include "common.h"
#include "lcd.h"


void hx8347_set_pos(unsigned int x0, unsigned int y0)
{
    lcd_cmd(0x03, (x0>>0)); //set x0
    lcd_cmd(0x02, (x0>>8)); //set x0
    lcd_cmd(0x07, (y0>>0)); //set y0
    lcd_cmd(0x06, (y0>>8)); //set y0
    LCD_REG = 0x22;
}

void hx8347_draw_start(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
    if (screen_flip) {
        lcd_cmd(0x16, 0x28); //MY=0 MX=0 MV=1 ML=0 BGR=1
    } else {
        lcd_cmd(0x16, 0x68); //MY=0 MX=1 MV=1 ML=0 BGR=1
    }
    lcd_cmd(0x03, (x0>>0)); //set x0
    lcd_cmd(0x02, (x0>>8)); //set x0
    lcd_cmd(0x05, (x1>>0)); //set x1
    lcd_cmd(0x04, (x1>>8)); //set x1
    lcd_cmd(0x07, (y0>>0)); //set y0
    lcd_cmd(0x06, (y0>>8)); //set y0
    lcd_cmd(0x09, (y1>>0)); //set y1
    lcd_cmd(0x08, (y1>>8)); //set y1
    LCD_REG = 0x22;
}

void hx8347_sleep()
{
    lcd_cmd(0x28, 0x38);
    Delay(40);
    lcd_cmd(0x1f, 0x89);
    Delay(40);
    lcd_cmd(0x28, 0x04);
    Delay(40);
    lcd_cmd(0x19, 0x00);
    Delay(5);
}

static const struct lcdtype hx8347_type = {
    hx8347_set_pos,
    hx8347_draw_start,
    hx8347_sleep,
};

void hx8347_init()
{
  //driving ability
  lcd_cmd(0xEA, 0x00);
  lcd_cmd(0xEB, 0x20);
  lcd_cmd(0xEC, 0x0C);
  lcd_cmd(0xED, 0xC4);
  lcd_cmd(0xE8, 0x40);
  lcd_cmd(0xE9, 0x38);
  lcd_cmd(0xF1, 0x01);
  lcd_cmd(0xF2, 0x10);
  lcd_cmd(0x27, 0xA3);

  //power voltage
  lcd_cmd(0x1B, 0x1B);
  lcd_cmd(0x1A, 0x01);
  lcd_cmd(0x24, 0x2F);
  lcd_cmd(0x25, 0x57);

  //VCOM offset
  lcd_cmd(0x23, 0x8D); //for flicker adjust

//ownTry
//lcd_cmd(0x29,0x06);
//lcd_cmd(0x2A,0x00);
//lcd_cmd(0x2C,0x06);
//lcd_cmd(0x2D,0x06);

  //power on
  lcd_cmd(0x18, 0x36);
  lcd_cmd(0x19, 0x01); //start osc
  lcd_cmd(0x01, 0x00); //wakeup
  lcd_cmd(0x1F, 0x88);
  Delay(5);
  lcd_cmd(0x1F, 0x80);
  Delay(5);
  lcd_cmd(0x1F, 0x90);
  Delay(5);
  lcd_cmd(0x1F, 0xD0);
  Delay(5);

  lcd_cmd(0x16, 0x68); //MY=0 MX=1 MV=1 ML=0 BGR=1

  //color selection
  lcd_cmd(0x17, 0x05); //0x0005=65k, 0x0006=262k

  //panel characteristic
  lcd_cmd(0x36, 0x00);

  //display on
  lcd_cmd(0x28, 0x38);
  Delay(40);
  lcd_cmd(0x28, 0x3C);
  disp_type = &hx8347_type;
}
