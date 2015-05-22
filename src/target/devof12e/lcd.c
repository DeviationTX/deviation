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
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/f1/nvic.h>
#include "common.h"
#include "lcd.h"


#define  I2C_EVENT_MASTER_MODE_SELECT                      ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */
#define  I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED        ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define  I2C_EVENT_MASTER_BYTE_TRANSMITTED                 ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */
#define  I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED           ((uint32_t)0x00030002)  /* BUSY, MSL and ADDR flags */

#define I2C_FLAG_BUSY                   ((uint32_t)0x00020000)
#define I2C_FLAG_BTF                    ((uint32_t)0x10000004)
#define I2C_FLAG_RXNE                   ((uint32_t)0x10000040)
#define I2C_FLAG_ADDR                   ((uint32_t)0x10000002)

#define LONG_TIMEOUT 0x10000
#define FLAG_TIMEOUT 0x1000

volatile u8 dmaComplete;
int I2C1_check_event(u32 event1, u32 event2)
{
    u32 event = event2 << 16 | event1;
    u32 sr = (I2C1_SR2 << 16) | I2C1_SR1;
    return (event & sr) == event;
}
unsigned I2C1_CheckEvent(u32 event)
{
  u32 lastevent = ((I2C1_SR2 << 16) | I2C1_SR1) & 0x00FFFFFF;

  return (lastevent & event) == event;
}

unsigned I2C1_GetFlagStatus(u32 flag)
{
  /* Read flag register index */
  u32 i2creg = (I2C1_SR2 << 16) | I2C1_SR1;

  /* Get bit[23:0] of the flag */
  flag &= 0xFFFFFF;

  return flag & i2creg;
}

u32 i2c1_timeout()
{
    I2C1_SR1 = ~ (0x1000DFDF & 0xFFFFFF);
    i2c_send_stop(I2C1);
    return 1;
}

u32 I2C1_WriteBufferDMA(u16 deviceId, u8 *dmaAddr, s16 periphMemAddr, int len)
{ 
  /*!< While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while(I2C1_GetFlagStatus(I2C_FLAG_BUSY))
  {
    if((timeout--) == 0) {printf("Error1\n"); return i2c1_timeout(); }
  }
  
  /*!< Send START condition */
  i2c_send_start(I2C1);
  
  /*!< Test on EV5 and clear it */
  timeout = FLAG_TIMEOUT;
  while(!I2C1_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((timeout--) == 0) {printf("Error2\n"); return i2c1_timeout(); }
  }
  
  /*!< Send EEPROM address for write */
  i2c_send_7bit_address(I2C1, deviceId, I2C_WRITE);

  /*!< Test on EV6 and clear it */
  timeout = FLAG_TIMEOUT;
  //while(!I2C1_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  //{
  //  if((timeout--) == 0) {printf("Error3\n"); return i2c1_timeout(); }
  //}
  while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR)) {
    if((timeout--) == 0) {printf("Error3\n"); return i2c1_timeout(); }
  }
  (void)I2C_SR2(I2C1);

  /*!< Send the EEPROM's internal address to write to : only one byte Address */
  i2c_send_data(I2C1, periphMemAddr);
  
  /*!< Test on EV8 and clear it */
  timeout = FLAG_TIMEOUT; 
  //while(!I2C1_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  //while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF | I2C_SR1_TxE)))
  while (!(I2C_SR1(I2C1) & (I2C_SR1_BTF)))
  {
    if((timeout--) == 0) {printf("Error4\n"); return i2c1_timeout(); }
  }
  while(len--) {
      i2c_send_data(I2C1, *dmaAddr++);
      timeout = FLAG_TIMEOUT; 
      u32 flag = len ? I2C_SR1_BTF : (I2C_SR1_BTF | I2C_SR1_TxE);
      while (!(I2C_SR1(I2C1) & flag))
      {
        if((timeout--) == 0) {printf("Error%d\n", 5 + len); return i2c1_timeout(); }
      }
  }

  /* Send STOP condition. */
  i2c_send_stop(I2C1);

  
  //dma_set_number_of_data(DMA1, DMA_CHANNEL6, len);
  //dma_set_memory_address(DMA1, DMA_CHANNEL6, (long)dmaAddr);
  //dma_set_read_from_memory(DMA1, DMA_CHANNEL6);
  //dma_enable_channel(DMA1, DMA_CHANNEL6);
  
  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

//void dma1_channel6_isr()
//{
//    printf("DMA Done\n");
//    dmaComplete = 1;
//    dma_disable_channel(DMA1, DMA_CHANNEL6);
//
//}

u32 I2C1_ReadBufferDMA(u16 deviceId, u8 *dmaAddr, s16 periphMemAddr, int len)
{  
  /*!< While the bus is busy */
  unsigned timeout = LONG_TIMEOUT;
  while(I2C1_GetFlagStatus(I2C_FLAG_BUSY))
  {
    if((timeout--) == 0) {printf("ReadErr1\n"); return i2c1_timeout();}
  }
  
  /*!< Send START condition */
  i2c_send_start(I2C1);
  
  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  timeout = FLAG_TIMEOUT;
  while(!I2C1_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((timeout--) == 0) {printf("ReadErr2\n"); return i2c1_timeout();}
  }
  
  /*!< Send EEPROM address for write */
  i2c_send_7bit_address(I2C1, deviceId, I2C_WRITE);

  /*!< Test on EV6 and clear it */
  timeout = FLAG_TIMEOUT;
  while(!I2C1_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((timeout--) == 0) {printf("ReadErr3\n"); return i2c1_timeout();}
  } 

  (void)I2C_SR2(I2C1);

  /*!< Send the EEPROM's internal address to read from: Only one byte address */
  i2c_send_data(I2C1, periphMemAddr);

  /*!< Test on EV8 and clear it */
  timeout = FLAG_TIMEOUT;
  //while(! I2C1_GetFlagStatus(I2C_FLAG_BTF))
  while (!(I2C1_SR1 & (I2C_SR1_BTF | I2C_SR1_TxE)));
  {
    if((timeout--) == 0) {printf("ReadErr4\n"); return i2c1_timeout();}
  }
  
  /*!< Send STRAT condition a second time */  
  i2c_send_start(I2C1);
  
  /*!< Test on EV5 and clear it (cleared by reading SR1 then writing to DR) */
  timeout = FLAG_TIMEOUT;
  while(!I2C1_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((timeout--) == 0) {printf("ReadErr5\n"); return i2c1_timeout();}
  } 
  
  /*!< Send EEPROM address for read */
  i2c_send_7bit_address(I2C1, deviceId, I2C_READ);
  
  /* If number of data to be read is 1, then DMA couldn't be used */
  /* One Byte Master Reception procedure (POLLING) ---------------------------*/
  if (len < 2)
  {
    /* Wait on ADDR flag to be set (ADDR is still not cleared at this level */
    timeout = FLAG_TIMEOUT;
    while(! I2C1_GetFlagStatus(I2C_FLAG_ADDR))
    {
      if((timeout--) == 0) {printf("ReadErr6a\n"); return i2c1_timeout();}
    }     
    
    /*!< Disable Acknowledgement */
    i2c_disable_ack(I2C1);

    /* Call User callback for critical section start (should typically disable interrupts) */
    asm volatile ("cpsid i");
    
    /* Clear ADDR register by reading SR1 then SR2 register (SR1 has already been read) */
    (void)I2C1_SR2;
    
    /*!< Send STOP Condition */
    i2c_send_stop(I2C1);
   
    /* Call User callback for critical section end (should typically re-enable interrupts) */
    asm volatile ("cpsie i");
    
    /* Wait for the byte to be received */
    timeout = FLAG_TIMEOUT;
    while(! I2C1_GetFlagStatus(I2C_FLAG_RXNE))
    {
      if((timeout--) == 0) {printf("ReadErr6b\n"); return i2c1_timeout(); }
    }
    
    /*!< Read the byte received from the EEPROM */
    *dmaAddr = i2c_get_data(I2C1);
    
    /*!< Decrement the read bytes counter */
    len--;        
    
    /* Wait to make sure that STOP control bit has been cleared */
    timeout = FLAG_TIMEOUT;
    while(I2C1_CR1 & I2C_CR1_STOP)
    {
      if((timeout--) == 0) {printf("ReadErr6c\n"); return i2c1_timeout();}
    }  
    
    /*!< Re-Enable Acknowledgement to be ready for another reception */
    i2c_enable_ack(I2C1);
  }
  else/* More than one Byte Master Reception procedure (DMA) -----------------*/
  {
    /*!< Test on EV6 and clear it */
    timeout = FLAG_TIMEOUT;
    while(!I2C1_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
      if((timeout--) == 0) {printf("ReadErr7a\n"); return i2c1_timeout(); }
    }  
    
    /* Configure the DMA Rx Channel with the buffer address and the buffer size */
    dma_set_number_of_data(DMA1, DMA_CHANNEL7, len);
    dma_set_memory_address(DMA1, DMA_CHANNEL7, (long)dmaAddr);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL7);
    
    /* Inform the DMA that the next End Of Transfer Signal will be the last one */
    i2c_set_dma_last_transfer(I2C1);
    
    /* Enable the DMA Rx Channel */
    dma_enable_channel(DMA1, DMA_CHANNEL7);
  }
  
  /* If all operations OK, return sEE_OK (0) */
  return 0;
}

u32 LCD_ReadReg(unsigned reg)
{
    u8 val;
    int i;
    for (i = 0; i < 2; i++) {
        if (! I2C1_ReadBufferDMA(0x45, &val, reg, 1))
            break;
    }
    if (i == 3)
        return 0;
    return val;
}

void LCD_WriteReg(unsigned reg, u8 val)
{
    int i;
    //dmaComplete = 0;
    for (i = 0; i < 2; i++) {
        //printf("Try %d\n", i);
        if (! I2C1_WriteBufferDMA(0x45, &val, reg, 1))
            break;
    }
    //printf("Wait\n");
    if (i == 3)
        return;
    //while(! dmaComplete)
    //    ;
    return;
}

void LCD_WriteBuffer(u16 periphAddr, u8 *buffer, unsigned len)
{
    dmaComplete = 0;
    if(! I2C1_WriteBufferDMA(0x45, buffer, periphAddr, len)) {
        //while(! dmaComplete)
        //    ;
    }
}

void LCD_Init()
{
    //rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_DMA1EN);
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_I2C1EN);
    
    //for(int channel = 6; channel < 8; channel++) {
    //    dma_channel_reset(DMA1, channel);
    //    dma_set_peripheral_address(DMA1, channel, I2C1_DR);
    //    dma_set_read_from_memory(DMA1, channel);
    //    dma_set_number_of_data(DMA1, channel, 0xFFFF);
    //    dma_disable_peripheral_increment_mode(DMA1, channel);
    //    dma_enable_memory_increment_mode(DMA1, channel);
    //    dma_set_peripheral_size(DMA1, channel, DMA_CCR_PSIZE_8BIT);
    //    dma_set_memory_size(DMA1, channel, DMA_CCR_MSIZE_8BIT);
    //    dma_set_priority(DMA1, channel, DMA_CCR_PL_VERY_HIGH);
    //    dma_enable_transfer_complete_interrupt(DMA1, channel);
    //}
    //nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);

    i2c_peripheral_disable(I2C1);
    i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
    i2c_set_fast_mode(I2C1);
    i2c_set_ccr(I2C1, 0x1e);
    i2c_set_trise(I2C1, 0x0b);
    i2c_set_dutycycle(I2C1, I2C_CCR_DUTY_DIV2);
    i2c_peripheral_enable(I2C1);
    //i2c_enable_dma(I2C1);
    
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
    //Video channel bit 3
    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
    gpio_clear(GPIOD, GPIO10);
    TW8816_Reset();
    TW8816_Init();
    //TW8816_Test();
}

void LCD_Clear(unsigned int color) {
    (void)color;
    printf("Clearing display\n");
    TW8816_ClearDisplay();
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    c = TW8816_map_char(c);
    if (x >= LCD_WIDTH)
        x = LCD_WIDTH-1;
    u32 pos = ((y*LCD_WIDTH)>>2) + (x>>1);
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
    (void)enable;
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
                TW8816_DisplayCharacter(i, 0x100 + 200 + i, 7);
            }
        } else {
            TW8816_LoadFont(font_map, window-4, 1);
        }
        TW8816_SetWindow(1);
    }
    window = val;
}

void VIDEO_SetChannel(int ch)
{
    if(ch & 0x01)
        gpio_set(GPIOE, GPIO8);
    else
        gpio_clear(GPIOE, GPIO8);

    if(ch & 0x02)
        gpio_set(GPIOE, GPIO9);
    else
        gpio_clear(GPIOE, GPIO9);

    if(ch & 0x04)
        gpio_set(GPIOE, GPIO10);
    else
        gpio_clear(GPIOE, GPIO10);

    if(ch & 0x08)
        gpio_set(GPIOD, GPIO10);
    else
        gpio_clear(GPIOD, GPIO10);
}

void VIDEO_Enable(int on)
{
    if(on) {
        gpio_set(GPIOE, GPIO11);
    } else {
        gpio_clear(GPIOE, GPIO11);
    }
}

