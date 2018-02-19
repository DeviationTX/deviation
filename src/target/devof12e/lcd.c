/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.

    This file is partially based upon the CooCox ILI9341S driver
    http://www.coocox.org/driver_repo/305488dd-734b-4cce-a8a4-39dcfef8cc66/html/group___i_l_i9341_s.html
*/

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include "common.h"
#include "lcd.h"


#define LONG_TIMEOUT 0x10000
#define FLAG_TIMEOUT 0x1000

u32 i2c1_timeout()
{
    I2C1_SR1 = ~ (0x1000DFDF & 0xFFFFFF);
    i2c_send_stop(I2C1);
    return 1;
}

u32 I2C1_WriteBuffer(u16 deviceId, u8 *buffer, s16 periphMemAddr, int len)
{
  /* While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while(I2C_SR2(I2C1) & I2C_SR2_BUSY)
  {
    if((timeout--) == 0) {printf("Error1\n"); return i2c1_timeout(); }
  }

  /* Send START condition */
  i2c_send_start(I2C1);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while(!((I2C_SR1(I2C1) & I2C_SR1_SB) & (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if((timeout--) == 0) {printf("Error2\n"); return i2c1_timeout(); }
  }

  /* Send EEPROM address for write */
  i2c_send_7bit_address(I2C1, deviceId, I2C_WRITE);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR))
  {
    if((timeout--) == 0) {printf("Error3\n"); return i2c1_timeout(); }
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(I2C1);

  /* Send the EEPROM's internal address to write to : only one byte Address */
  i2c_send_data(I2C1, periphMemAddr);

  /* Waiting for byte transfer finished */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF)))
  {
    if((timeout--) == 0) {printf("Error4\n"); return i2c1_timeout(); }
  }

  while(len--) {
      /* Send the byte to be written */
      i2c_send_data(I2C1, *buffer++);
      /* Waiting for byte transfer finished */
      timeout = FLAG_TIMEOUT;
      u32 flag = len ? I2C_SR1_BTF : (I2C_SR1_BTF | I2C_SR1_TxE);
      while (!(I2C_SR1(I2C1) & flag))
      {
        if((timeout--) == 0) {printf("Error%d\n", 5 + len); return i2c1_timeout(); }
      }
  }

  /* Send STOP condition. */
  i2c_send_stop(I2C1);

  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

u32 I2C1_ReadBuffer(u16 deviceId, u8 *buffer, s16 periphMemAddr, int len)
{
  /* While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while(I2C_SR2(I2C1) & I2C_SR2_BUSY)
  {
    if((timeout--) == 0) {printf("ReadErr1\n"); return i2c1_timeout();}
  }

  /* Send START condition */
  i2c_send_start(I2C1);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while(!((I2C_SR1(I2C1) & I2C_SR1_SB) & (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if((timeout--) == 0) {printf("ReadErr2\n"); return i2c1_timeout();}
  }

  /* Send EEPROM address for write */
  i2c_send_7bit_address(I2C1, deviceId, I2C_WRITE);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while(!(I2C_SR1(I2C1) & I2C_SR1_ADDR))
  {
    if((timeout--) == 0) {printf("ReadErr3\n"); return i2c1_timeout();}
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(I2C1);

  /* Send the EEPROM's internal address to read from: Only one byte address */
  i2c_send_data(I2C1, periphMemAddr);

  /* Waiting for byte transfer finished */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF | I2C_SR1_TxE)))
  {
    if((timeout--) == 0) {printf("ReadErr4\n"); return i2c1_timeout();}
  }

  /* Send START condition a second time */
  i2c_send_start(I2C1);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while(!((I2C_SR1(I2C1) & I2C_SR1_SB) & (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if((timeout--) == 0) {printf("ReadErr5\n"); return i2c1_timeout();}
  }

  /* Send EEPROM address for read */
  i2c_send_7bit_address(I2C1, deviceId, I2C_READ);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while(!(I2C_SR1(I2C1) & I2C_SR1_ADDR))
  {
    if((timeout--) == 0) {printf("ReadErr6a\n"); return i2c1_timeout();}
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(I2C1);

  /* Enable Acknowledgement to be ready for another reception */
  i2c_enable_ack(I2C1);

  /* While there is data to be read */
  while(len) {
      if(len == 1) {
          /* Disable Acknowledgement */
          i2c_disable_ack(I2C1);

          /* Send STOP Condition */
          i2c_send_stop(I2C1);
      }

      /* Wait for the byte to be received */
      timeout = FLAG_TIMEOUT;
      while(!(I2C_SR1(I2C1) & (I2C_SR1_RxNE)))
      {
        if((timeout--) == 0) {printf("ReadErr6b\n"); return i2c1_timeout(); }
      }

      /* Read a next byte */
      *buffer = i2c_get_data(I2C1);

      /* Point to the next location where the byte read will be saved */
      buffer++;

      /* Decrement the read bytes counter */
      len--;
  } //while(len)

  /* Wait to make sure that STOP control bit has been cleared */
  timeout = FLAG_TIMEOUT;
  while(I2C_CR1(I2C1) & I2C_CR1_STOP)
  {
    if((timeout--) == 0) {printf("ReadErr6c\n"); return i2c1_timeout();}
  }

  /* Re-Enable Acknowledgement to be ready for another reception */
  i2c_enable_ack(I2C1);

  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

u32 LCD_ReadReg(unsigned reg)
{
    u8 val;
    for (int i = 0; i < 2; i++) {
        if (! I2C1_ReadBuffer(0x45, &val, reg, 1))
            break;
    }
    return val;
}

void LCD_WriteReg(unsigned reg, u8 val)
{
    u8 value = val;
    for (int i = 0; i < 2; i++) {
        if (! I2C1_WriteBuffer(0x45, &value, reg, 1))
            break;
    }
}

void LCD_WriteBuffer(u16 periphAddr, u8 *buffer, unsigned len)
{
    for (int i = 0; i < 2; i++) {
        if (! I2C1_WriteBuffer(0x45, buffer, periphAddr, len))
            break;
    }
}

void LCD_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_I2C1EN);
    i2c_peripheral_disable(I2C1);
    i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
    i2c_set_fast_mode(I2C1);
    i2c_set_ccr(I2C1, 0x1e);
    i2c_set_trise(I2C1, 0x0b);
    i2c_set_dutycycle(I2C1, I2C_CCR_DUTY_DIV2);
    i2c_peripheral_enable(I2C1);

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);
    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);
    gpio_set(GPIOE, GPIO7);

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO6 | GPIO7);


    //Video channel bits 2:0 and av on/off
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);
    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL, GPIO8 | GPIO9 | GPIO10 | GPIO11);
    gpio_clear(GPIOE, GPIO8 | GPIO9 | GPIO10 | GPIO11);
    //Video channel bit 3, 4
    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL, GPIO8 | GPIO10);
    gpio_clear(GPIOD, GPIO8 | GPIO10);
    TW8816_ResetLoop();
    TW8816_Init();
}

void LCD_Clear(unsigned int color) {
    (void)color;
    //printf("Clearing display\n");
    TW8816_ClearDisplay();
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    c = TW8816_map_char(c);
    if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;
    u32 pos = ((y >> 1)*(LCD_WIDTH>>1)) + (x>>1);
    //printf("%02x(%c): %d, %d, %d\n", c, c, x, y, pos);
    TW8816_DisplayCharacter(pos, c, 7);
}

static const struct font_def default_font = {2, 2};
struct font_str cur_str;

u8 LCD_SetFont(unsigned int idx)
{
    (void)idx;
    cur_str.font = default_font;
    return 1;
}

u8 FONT_GetFromString(const char *value)
{
    (void)value;
    return 1;
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir _dir)
{
    (void) x0; (void) y0; (void) x1; (void) y1; (void) _dir;
}

void LCD_DrawStop(void)
{

}

void LCD_ShowVideo(u8 enable)
{
    TW8816_SetVideoMode(enable);
}

extern u8 font_map[27 * 6* 4];
extern u8 window;

void LCD_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    val = val == 1 ? 0 : 1;
    TW8816_CreateMappedWindow(val, x, y, w, h);
}

void LCD_UnmapWindow(unsigned i)
{
    i = i == 1 ? 0 : 1;
    TW8816_UnmapWindow(i);
}

void LCD_SetMappedWindow(unsigned val)
{
    if (val != 0) {
        TW8816_SetWindow(0);
        memset(font_map, 0, sizeof(font_map));
    } else {
        if (window < 4) {
            TW8816_LoadFont(font_map, 200, 6 * 4);
            for (int i = 0; i < 24; i++) {
                TW8816_DisplayCharacter(i, 0x300 + 200 + i, 7);
            }
        } else {
            TW8816_LoadFont(font_map, window - 4, 1);
        }
        TW8816_SetWindow(1);
    }
    window = val;
}

void LCD_Contrast(unsigned contrast)
{
    (void)contrast;
}

void VIDEO_Contrast(int contrast)
{
    int c = (int)contrast * 128 / 10 + 128;
    if (c < 0)
        c = 0;
    if (c > 255)
        c = 255;
    TW8816_Contrast(c);
}

void VIDEO_Brightness(int brightness)
{
    int b = (int)brightness * 128 / 10;
    if (b < -128)
        b = -128;
    if (b > 127)
        b = 127;
    TW8816_Brightness(b);
}

void VIDEO_Chroma(unsigned chromau, unsigned chromav)
{
    chromau *= 20;
    chromav *= 20;
    TW8816_Chroma(chromau, chromav);
}

void VIDEO_SetChannel(int ch)
{
    if(ch & 0x01)
        gpio_clear(GPIOE, GPIO8);
    else
        gpio_set(GPIOE, GPIO8);

    if(ch & 0x02)
        gpio_clear(GPIOE, GPIO9);
    else
        gpio_set(GPIOE, GPIO9);

    if(ch & 0x04)
        gpio_clear(GPIOE, GPIO10);
    else
        gpio_set(GPIOE, GPIO10);

    if(ch & 0x08)
        gpio_clear(GPIOD, GPIO10);
    else
        gpio_set(GPIOD, GPIO10);
    if(ch & 0x10)
        gpio_set(GPIOD, GPIO8);
    else
        gpio_clear(GPIOD, GPIO8);
}

void VIDEO_Enable(int on)
{
    if(on) {
        gpio_set(GPIOE, GPIO11);
        LCD_ShowVideo(1);
    } else {
        gpio_clear(GPIOE, GPIO11);
        LCD_ShowVideo(0);
    }
}

u8 VIDEO_GetStandard()
{
    return TW8816_GetVideoStandard();
}

void VIDEO_SetStandard(u8 standard)
{
    TW8816_SetVideoStandard(standard);
}
