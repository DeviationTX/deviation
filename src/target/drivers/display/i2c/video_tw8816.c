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

#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "common.h"
#include "lcd.h"
#include "tw8816_map.h"
#include "target/drivers/mcu/stm32/i2c.h"

//Set this to the number of trials
#define DEBUG_SCREEN_ALIGNMENT 0
#include "tw8816_init_1.6.h"
//#include "tw8816_init_1.5.h"
//#include "tw8816_init_1.3.h"
//#include "tw8816_init_bl1.3a.h"
//#include "tw8816_init_bl1.6a.h"

//Video Standard
enum
{
    NOINPUT = 0,
    NTSC,
    PAL,
    SECAM,
    NTSC4,
    PALM,
    PALN,
    PAL60,
    UNKNOWN = 0xFE,
};

extern u8 window;

#define LONG_TIMEOUT 0x10000
#define FLAG_TIMEOUT 0x1000

static u32 lcd_i2c_timeout()
{
    I2C_SR1(LCD_I2C_CFG.i2c) = ~(0x1000DFDF & 0xFFFFFF);
    i2c_send_stop(LCD_I2C_CFG.i2c);
    return 1;
}

static u32 LCD_I2C_WriteBuffer(u16 deviceId, u8 *buffer, s16 periphMemAddr, int len)
{
  /* While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while (I2C_SR2(LCD_I2C_CFG.i2c) & I2C_SR2_BUSY)
  {
    if ((timeout--) == 0) {printf("Error1\n"); return lcd_i2c_timeout(); }
  }

  /* Send START condition */
  i2c_send_start(LCD_I2C_CFG.i2c);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while (!((I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_SB) & (I2C_SR2(LCD_I2C_CFG.i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if ((timeout--) == 0) {printf("Error2\n"); return lcd_i2c_timeout(); }
  }

  /* Send EEPROM address for write */
  i2c_send_7bit_address(LCD_I2C_CFG.i2c, deviceId, I2C_WRITE);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_ADDR))
  {
    if ((timeout--) == 0) {printf("Error3\n"); return lcd_i2c_timeout(); }
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(LCD_I2C_CFG.i2c);

  /* Send the EEPROM's internal address to write to : only one byte Address */
  i2c_send_data(LCD_I2C_CFG.i2c, periphMemAddr);

  /* Waiting for byte transfer finished */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(LCD_I2C_CFG.i2c) & (I2C_SR1_BTF)))
  {
    if ((timeout--) == 0) {printf("Error4\n"); return lcd_i2c_timeout(); }
  }

  while(len--) {
      /* Send the byte to be written */
      i2c_send_data(LCD_I2C_CFG.i2c, *buffer++);
      /* Waiting for byte transfer finished */
      timeout = FLAG_TIMEOUT;
      u32 flag = len ? I2C_SR1_BTF : (I2C_SR1_BTF | I2C_SR1_TxE);
      while (!(I2C_SR1(LCD_I2C_CFG.i2c) & flag))
      {
        if ((timeout--) == 0) {printf("Error%d\n", 5 + len); return lcd_i2c_timeout(); }
      }
  }

  /* Send STOP condition. */
  i2c_send_stop(LCD_I2C_CFG.i2c);

  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

static u32 LCD_I2C_ReadBuffer(u16 deviceId, u8 *buffer, s16 periphMemAddr, int len)
{
  /* While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while (I2C_SR2(LCD_I2C_CFG.i2c) & I2C_SR2_BUSY)
  {
    if ((timeout--) == 0) {printf("ReadErr1\n"); return lcd_i2c_timeout();}
  }

  /* Send START condition */
  i2c_send_start(LCD_I2C_CFG.i2c);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while (!((I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_SB) & (I2C_SR2(LCD_I2C_CFG.i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if ((timeout--) == 0) {printf("ReadErr2\n"); return lcd_i2c_timeout();}
  }

  /* Send EEPROM address for write */
  i2c_send_7bit_address(LCD_I2C_CFG.i2c, deviceId, I2C_WRITE);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_ADDR))
  {
    if ((timeout--) == 0) {printf("ReadErr3\n"); return lcd_i2c_timeout();}
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(LCD_I2C_CFG.i2c);

  /* Send the EEPROM's internal address to read from: Only one byte address */
  i2c_send_data(LCD_I2C_CFG.i2c, periphMemAddr);

  /* Waiting for byte transfer finished */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(LCD_I2C_CFG.i2c) & (I2C_SR1_BTF | I2C_SR1_TxE)))
  {
    if ((timeout--) == 0) {printf("ReadErr4\n"); return lcd_i2c_timeout();}
  }

  /* Send START condition a second time */
  i2c_send_start(LCD_I2C_CFG.i2c);

  /* Wait for master mode selected */
  timeout = FLAG_TIMEOUT;
  while (!((I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_SB) & (I2C_SR2(LCD_I2C_CFG.i2c) & (I2C_SR2_MSL | I2C_SR2_BUSY))))
  {
    if ((timeout--) == 0) {printf("ReadErr5\n"); return lcd_i2c_timeout();}
  }

  /* Send EEPROM address for read */
  i2c_send_7bit_address(LCD_I2C_CFG.i2c, deviceId, I2C_READ);

  /* Waiting for address is transferred */
  timeout = FLAG_TIMEOUT;
  while (!(I2C_SR1(LCD_I2C_CFG.i2c) & I2C_SR1_ADDR))
  {
    if ((timeout--) == 0) {printf("ReadErr6a\n"); return lcd_i2c_timeout();}
  }

  /* Cleaning ADDR condition sequence */
  (void)I2C_SR2(LCD_I2C_CFG.i2c);

  /* Enable Acknowledgement to be ready for another reception */
  i2c_enable_ack(LCD_I2C_CFG.i2c);

  /* While there is data to be read */
  while(len) {
      if(len == 1) {
          /* Disable Acknowledgement */
          i2c_disable_ack(LCD_I2C_CFG.i2c);

          /* Send STOP Condition */
          i2c_send_stop(LCD_I2C_CFG.i2c);
      }

      /* Wait for the byte to be received */
      timeout = FLAG_TIMEOUT;
      while (!(I2C_SR1(LCD_I2C_CFG.i2c) & (I2C_SR1_RxNE)))
      {
        if ((timeout--) == 0) {printf("ReadErr6b\n"); return lcd_i2c_timeout(); }
      }

      /* Read a next byte */
      *buffer = i2c_get_data(LCD_I2C_CFG.i2c);

      /* Point to the next location where the byte read will be saved */
      buffer++;

      /* Decrement the read bytes counter */
      len--;
  } //while(len)

  /* Wait to make sure that STOP control bit has been cleared */
  timeout = FLAG_TIMEOUT;
  while (I2C_CR1(LCD_I2C_CFG.i2c) & I2C_CR1_STOP)
  {
    if ((timeout--) == 0) {printf("ReadErr6c\n"); return lcd_i2c_timeout();}
  }

  /* Re-Enable Acknowledgement to be ready for another reception */
  i2c_enable_ack(LCD_I2C_CFG.i2c);

  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

static u32 LCD_ReadReg(unsigned reg)
{
    u8 val = 0;
    for (int i = 0; i < 2; i++) {
        if (!LCD_I2C_ReadBuffer(0x45, &val, reg, 1))
            break;
    }
    return val;
}

static void LCD_WriteReg(unsigned reg, u8 val)
{
    u8 value = val;
    for (int i = 0; i < 2; i++) {
        if (!LCD_I2C_WriteBuffer(0x45, &value, reg, 1))
            break;
    }
}

static void LCD_WriteBuffer(u16 periphAddr, u8 *buffer, unsigned len)
{
    for (int i = 0; i < 2; i++) {
        if (!LCD_I2C_WriteBuffer(0x45, buffer, periphAddr, len))
            break;
    }
}

#if DEBUG_SCREEN_ALIGNMENT
static void wait_button() {
    u32 buttons = ScanButtons();
    while(! CHAN_ButtonIsPressed(buttons, BUT_EXIT))
        buttons = ScanButtons();
    _msleep(100);
    while( CHAN_ButtonIsPressed(buttons, BUT_EXIT))
        buttons = ScanButtons();
    _msleep(100);
}
#endif

static void TW8816_Init_Ports()
{
    _i2c_init(LCD_I2C_CFG);

    // LCD Reset
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_RESET_PIN));
    GPIO_setup_output(LCD_RESET_PIN, OTYPE_PUSHPULL);
    GPIO_pin_set(LCD_RESET_PIN);

    // Video channel bits 2:0 and av on/off
    // NOTE: do no wrap this in a loop.  Doing so will break macro expansion
    // CS0
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_CS0));
    GPIO_setup_output(LCD_VIDEO_CS0, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_CS0);
    // CS1
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_CS1));
    GPIO_setup_output(LCD_VIDEO_CS1, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_CS1);
    // CS2
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_CS2));
    GPIO_setup_output(LCD_VIDEO_CS2, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_CS2);
    // CS3
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_CS3));
    GPIO_setup_output(LCD_VIDEO_CS3, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_CS3);
    // CS4
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_CS4));
    GPIO_setup_output(LCD_VIDEO_CS4, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_CS4);
    // PWR
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_VIDEO_PWR));
    GPIO_setup_output(LCD_VIDEO_PWR, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_VIDEO_PWR);
}

static void TW8816_Reset()
{
    GPIO_pin_clear(LCD_RESET_PIN);
    _msleep(250);
    GPIO_pin_set(LCD_RESET_PIN);
    _msleep(100);
}

static void TW8816_ResetLoop()
{
    u8 count = 0;

    TW8816_Reset();
    while(1) {
        LCD_WriteReg(0xFF, 0x00);
        if (LCD_ReadReg(0x00) == 0x22)
            break;
        count++;
        if (count < 250) {
            Delay(0x800);
        } else {
            count = 0;
            TW8816_Reset();
        }
    }
}

static void TW8816_ReinitPixelClock()
{
    u8 reg1 = LCD_ReadReg(0xB6);
    u8 reg2 = LCD_ReadReg(0xB2);
    LCD_WriteReg(0xB6, reg1 | 0x80);
    LCD_WriteReg(0xB2, reg2);
    Delay(0x60000);
    LCD_WriteReg(0xB6, reg1 & 0x7F);
    LCD_WriteReg(0xB2, reg2);
}

void TW8816_Init()
{
    TW8816_Init_Ports();

    TW8816_ResetLoop();

#if DEBUG_SCREEN_ALIGNMENT
    u8 reg[sizeof(reg_init)];
    struct _map {
        u8 grp;
        u8 idx;
        u8 reg;
        u8 val;
    } map[] = {
      {99, 102, 0xB4, 0x10},
      {1, 102, 0xB4, 0x18},
      {2, 102, 0xB4, 0x20},
      {3, 102, 0xB4, 0x28},
      {4, 102, 0xB4, 0x2E},
      {5, 102, 0xB4, 0x2E},
      {5, 107, 0xB9, 0x18},
      {6, 102, 0xB4, 0x2E},
      {6, 107, 0xB9, 0x1A},
      {7, 102, 0xB4, 0x2E},
      {7, 107, 0xB9, 0x1C},
      {8, 102, 0xB4, 0x2E},
      {8, 107, 0xB9, 0x1E},
      {9, 102, 0xB4, 0x2E},
      {9, 107, 0xB9, 0x20},
    };
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(1);
    for (int ii = 0; ii < DEBUG_SCREEN_ALIGNMENT ; ii++) {
        memcpy(reg, reg_init, sizeof(reg_init));
        for (unsigned i = 0; i < sizeof(map) / sizeof(struct _map); i++) {
            if(map[i].grp == ii) {
                reg[map[i].idx*2+1] = map[i].val;
            }
        }
#else
    const u8 *reg = reg_init;
    {
#endif
        TW8816_Reset();
        printf("1\n");
        for (unsigned i = 0; i < sizeof(reg_init); i+=2) {
            LCD_WriteReg(reg[i], reg[i+1]);
        }
        LCD_WriteReg(0xFF, 0x00);
        TW8816_SetVideoMode(0);
        TW8816_ReinitPixelClock();
        LCD_WriteReg(0xFF, 0x00);
        if (LCD_ReadReg(0x00) != 0x22) {
            printf("Could not identify display\n");
        }
        printf("3\n");
        //Clear screen
        LCD_WriteReg(0xFF, 0);
#if DEBUG_SCREEN_ALIGNMENT
        LCD_WriteReg(0x94, 0);
        //Setup normal display
        LCD_WriteReg(0x9e, 0x00);
        LCD_WriteReg(0x9f, 0x01);
        LCD_WriteReg(0xa0, 0x00);
        LCD_WriteReg(0xa1, 0x00);
        LCD_WriteReg(0xa2, 0x00);
        LCD_WriteReg(0xa6, 0x00);
        LCD_WriteReg(0xa7, 0x00);
        LCD_WriteReg(0xa3, 33);
        LCD_WriteReg(0xa4, 13);
        LCD_WriteReg(0xAC, 0x08);
        LCD_WriteReg(0xA9,  0x50);
        LCD_WriteReg(0x94, 2);
        TW8816_DisplayCharacter(0, 'A' + ii, 7);
        TW8816_DisplayCharacter(1, 'A' + ii, 7);
        TW8816_DisplayCharacter(31, 'A' + ii, 7);
        TW8816_DisplayCharacter(32, 'A' + ii, 7);
        TW8816_DisplayCharacter(12*33, 'A' + ii, 7);
        TW8816_DisplayCharacter(12*33+32, 'A' + ii, 7);
        wait_button();
#endif
    }
    //Setup XY Graph
    LCD_WriteReg(0x9e, 0x00);
    LCD_WriteReg(0x9f, 0x01);
    LCD_WriteReg(0xa0, (564 >> 8));
    LCD_WriteReg(0xa1, 0xff & 564); //564
    LCD_WriteReg(0xa2, 126); //126
    LCD_WriteReg(0xa3, 6);
    LCD_WriteReg(0xa4, 4);
    LCD_WriteReg(0xa5, 6);
    LCD_WriteReg(0xa6, 0x00);
    LCD_WriteReg(0xa7, 0x00);
    LCD_WriteReg(0xa8, 0x00);
    LCD_WriteReg(0xa9, 0xA1);
    LCD_WriteReg(0xaa, 0xAE); //0x1AE = 430
    LCD_WriteReg(0xAB,  0x12);
    LCD_WriteReg(0xAC,   0x08);
    LCD_WriteReg(0xAD,     0);
    LCD_WriteReg(0xAE,     0);

    //Setup normal display
    LCD_WriteReg(0x9e, 0x01);
    LCD_WriteReg(0x9f, 0x01);
    LCD_WriteReg(0xa0, 0x00);
    LCD_WriteReg(0xa1, 0x00);
    LCD_WriteReg(0xa2, 0x00);
    LCD_WriteReg(0xa6, 0x00);
    LCD_WriteReg(0xa7, 0x00);
    LCD_WriteReg(0xa3, 33);
    LCD_WriteReg(0xa4, 13);
    LCD_WriteReg(0xAC, 0x08);
    LCD_WriteReg(0xA9,  0x50);

    window = 0;

    //Hide XY Graph placeholder
    TW8816_UnmapWindow(0);
}

void TW8816_LoadFont(u8 *data, unsigned offset, unsigned count)
{
    LCD_WriteReg(0x94, 1);
    LCD_WriteReg(0x9B, 0xE2);
    LCD_WriteReg(0xE0, 0x10);
    for (unsigned i = 0; i < count; i++) {
        LCD_WriteReg(0x99, offset + i);
        LCD_WriteBuffer(0x9A, data + i * 27, 27);
    }
    LCD_WriteReg(0x94, 0);
    LCD_WriteReg(0xE0, 0);
}

void TW8816_SetVideoMode(unsigned enable)
{
    unsigned reg = enable ? 0x40 : 0x44;
    if(LCD_ReadReg(0x2) != reg) {
        LCD_WriteReg(0x02, reg);
        LCD_WriteReg(0x2F, enable ? 0xE0 : 0xE6);
    }
}

void TW8816_DisplayCharacter(u16 pos, unsigned chr, unsigned attr)
{
    if (window == 1)
      pos += 430;
    LCD_WriteReg(0x94, ((chr & 0xFF00) >= 0x300) ? 0x80 : 0x00); // isRamFont
    LCD_WriteReg(0x95, pos >> 8);
    LCD_WriteReg(0x96, pos & 0xff);
    LCD_WriteReg(0x97, chr & 0xff);
    LCD_WriteReg(0x98, attr);
}

void TW8816_ClearDisplay()
{
    LCD_WriteReg(0x94, 2);
}

void TW8816_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    (void)w;
    (void)h;
    TW8816_SetWindow(val);
    LCD_WriteReg(0x9f, 0x01);
    x *= CHAR_WIDTH;
    y *= CHAR_HEIGHT;
    LCD_WriteReg(0xa0, (0x30 & (y >> 4)) | (0x07 & (x >> 8)));
    LCD_WriteReg(0xa1, 0xff & x);
    LCD_WriteReg(0xa2, 0xff & y);
    TW8816_SetWindow(window);
}

void TW8816_SetWindow(unsigned i) {
    LCD_WriteReg(0x9e, i);
}

void TW8816_UnmapWindow(unsigned i)
{
    TW8816_SetWindow(i);
    LCD_WriteReg(0x9f, 0x00);
    TW8816_SetWindow(window);
}

void TW8816_Contrast(unsigned contrast)
{
    LCD_WriteReg(0x11, contrast);
}

void TW8816_Brightness(int brightness)
{
    LCD_WriteReg(0x10, brightness);
}

void TW8816_Sharpness(unsigned sharpness)
{
    int s = LCD_ReadReg(0x12);
    s = (s & ~0x0f) | sharpness;
    LCD_WriteReg(0x12, s);
}

void TW8816_Chroma(unsigned chromau, unsigned chromav)
{
    LCD_WriteReg(0x13, chromau);
    LCD_WriteReg(0x14, chromav);
}

u8 TW8816_GetVideoStandard()
{
    u8 val;
    u8 std;

    // Check no input
    val = LCD_ReadReg(0x01);   // Decoder status register
    if((val & 0xc0) != 0x40) { // No decoder input
        std = NOINPUT;
        return std;
    }
    //Check color system by decoder
    val = LCD_ReadReg(0x1C);   // SDT
    if(val & 0x80) {           // Detection in proress
        std = UNKNOWN;
    } else {
        val >>= 4;
        if(val == 0x07)        // Not valid
            std = UNKNOWN;
        else
            std = val + 1;     // 0=NTSC(M), 1=PAL(B,D,G,H,I), 2=SECAM, 3=NTSC4.43, 4=PAL(M), 5=PAL (CN), 6=PAL 60
    }
    return std;
}

void TW8816_SetVideoStandard(u8 standard)
{
    u8  val;
    u8  page;
    u16 Vactive;
    u32 Yscale;
    u16 Hperiod;

    //printf("VideoStd: %d\n", standard);

    switch (standard) {
        case NTSC:
        case NTSC4:
        case PALM:
        case PAL60:
            //IVF = 60 Hz
            Vactive = 240;  //240, number of lines in a half-frame
            Yscale = 32358; //65536*TV_ScaleHeight/Panel_Vactive 65536*237/480
            Hperiod = 1056;  //1056*525*60 = 33264000 Hz
            break;

        case PAL:
        case SECAM:
        case PALN:
        default:
            //IVF = 50 Hz
            Vactive = 288;  //288, number of lines in a half-frame
            Yscale = 38639; //65536*TV_ScaleHeight/Panel_Vactive 65536*283/480
            Hperiod = 1268; //1268*525*50 = 33285000 Hz
            break;
    }

    //Save index page
    page = LCD_ReadReg(0xFF);
    LCD_WriteReg(0xFF, 0x00); // index page 0

    //Set Decode Window
    LCD_WriteReg(0x1C, 0x0F); // Disable the shadow registers.
    val = LCD_ReadReg(0x07);
    val &= 0xCF; //clear bit4-bit5
    val |= (u8)((Vactive & 0x300)>>4);
    LCD_WriteReg(0x07, val);
    LCD_WriteReg(0x09, (u8)(Vactive & 0xFF));

    //Scaler config
    LCD_WriteReg(0x6A, (u8)(Yscale));
    Yscale >>= 8;
    LCD_WriteReg(0x62, (u8)(Yscale));
    Yscale >>= 8;
    Yscale <<= 2;
    val = LCD_ReadReg(0x63) & 0xF3;
    val |= (u8)Yscale;
    LCD_WriteReg(0x63, val);

    //Set panel FPHS Period
    val = LCD_ReadReg(0xB6);
    val &= 0x70; // clear bit0-bit3
    val |= (u8)((Hperiod & 0xF00)>>8);
    LCD_WriteReg(0xB6, val);
    LCD_WriteReg(0xB2, (u8)(Hperiod & 0xFF));

    //Restore index page
    LCD_WriteReg(0xFF, page);
}

void TW8816_SetVideoChannel(int ch)
{
    if(ch & 0x01)
        GPIO_pin_clear(LCD_VIDEO_CS0);
    else
        GPIO_pin_set(LCD_VIDEO_CS0);

    if(ch & 0x02)
        GPIO_pin_clear(LCD_VIDEO_CS1);
    else
        GPIO_pin_set(LCD_VIDEO_CS1);

    if(ch & 0x04)
        GPIO_pin_clear(LCD_VIDEO_CS2);
    else
        GPIO_pin_set(LCD_VIDEO_CS2);

    if(ch & 0x08)
        GPIO_pin_clear(LCD_VIDEO_CS3);
    else
        GPIO_pin_set(LCD_VIDEO_CS3);

    if(ch & 0x10)
        GPIO_pin_set(LCD_VIDEO_CS4);
    else
        GPIO_pin_clear(LCD_VIDEO_CS4);
}

void TW8816_EnableVideo(int on)
{
    if(on) {
        GPIO_pin_set(LCD_VIDEO_PWR);
        LCD_ShowVideo(1);
    } else {
        GPIO_pin_clear(LCD_VIDEO_PWR);
        LCD_ShowVideo(0);
    }
}
