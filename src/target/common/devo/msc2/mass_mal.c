/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : mass_mal.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : Medium Access Layer interface
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include <stm32f10x.h>
#include "mass_mal.h"
#include "usb_bot.h"
#include "common.h"
//#include "target.h"

#include <string.h>

#if defined USE_DEVOFS && USE_DEVOFS
   #define EMULATE_FAT 1
   #define FAT_OFFSET 3    //boot_sector + fat_sector + root_sector
  static const u8 boot[] = {
      0xeb, 0x3c, 0x90,
      'M', 'S', 'D', 'O', 'S', '5', '.', '0',
      0x00, 0x10,               // Bytes per Sector
      0x10,                     // Sectors per Cluster
      0x01, 0x00,               // Reserved Sectors
      0x01,                     // Number of FATs
      0x80, 0x00,               // Root Entries
      0x13, 0x00,               // Small Sectors
      0xF8,                     // Media Type
      0x01, 0x00,               // Sectors per FAT
      0x20, 0x00,               // Sectors per Track
      0x40, 0x00,               // Number of Heads
      0x00, 0x00, 0x00, 0x00,   // Hidden Sectors
      0x00, 0x00, 0x00, 0x00,   // Large Sectors
      0x80,                     // Physical Disk Number
      0x00,                     // Current Head
      0x29,                     // Signature
      0xC2, 0x72, 0x31, 0x29,   // Volume Serial Number
      'D', 'E', 'V', 'O', 'F', 'S', 0, 0, 0, 0, 0,
      'F', 'A', 'T', '1', '2', ' ', ' ', ' ',
  };
  
  static const u8 fat[] = {
   0xF8, 0x0F | 0xF0, 0xFF,
   0xFF, 0x0F
  };
//   0x03, 0x00 | 0x40, 0x00,
//   0x05, 0x00 | 0x60, 0x00,
//   0x07, 0x00 | 0x80, 0x00,
//   0x09, 0x00 | 0xA0, 0x00,
//   0x0B, 0x00 | 0xC0, 0x00,
//   0x0D, 0x00 | 0xE0, 0x00,
//   0x0F, 0x00 | 0x00, 0x01,
//   0x11, 0x00 | 0xF0, 0xFF,
  
  static const u8 root[] = {
   'D',  'E',  'V',  'O',  ' ',  ' ',  ' ',  ' ',  'F',  'S',  ' ',  0x20, 0x18, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
  };
#else
   #define EMULATE_FAT 0
   #define FAT_OFFSET 0
#endif
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t Mass_Memory_Size[2] = {0x1000 * (FAT_OFFSET + SPIFLASH_SECTORS - SPIFLASH_SECTOR_OFFSET), 0};
uint32_t Mass_Block_Size[2] = {4096, 0};
uint32_t Mass_Block_Count[2] = {FAT_OFFSET + SPIFLASH_SECTORS - SPIFLASH_SECTOR_OFFSET, 0};
volatile uint32_t Status = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
  uint16_t status = MAL_OK;

  switch (lun)
  {
    case 0:
      //NAND_Init();
      break;
    default:
      return MAL_FAIL;
  }
  return status;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
  switch (lun)
  {
    case 0:
      //DBG("Writing: 0x%08x %d\n", (unsigned int)Memory_Offset, (int)Transfer_Length);
#if EMULATE_FAT
      if (Memory_Offset + Transfer_Length < (0x1000 * FAT_OFFSET)) {
          return MAL_OK;
      }
      if (Memory_Offset < (0x1000 * FAT_OFFSET)) {
          Transfer_Length -= ((0x1000 * FAT_OFFSET) - Memory_Offset);
          Writebuff += ((0x1000 * FAT_OFFSET) - Memory_Offset);
          Memory_Offset = 0x1000 * FAT_OFFSET;
      }
#endif
      SPIFlash_WriteBytes(Memory_Offset  + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000), Transfer_Length, (u8 *)Writebuff);
      //NAND_Write(Memory_Offset, Writebuff, Transfer_Length);
      break;
    default:
      return MAL_FAIL;
  }
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
  switch (lun)
  {
    case 0:
      //NAND_Read(Memory_Offset, Readbuff, Transfer_Length);
      //DBG("Reading: 0x%08x %d\n", (unsigned int)Memory_Offset, (int)Transfer_Length);
#if EMULATE_FAT      
      if (Memory_Offset < (FAT_OFFSET * 0x1000)) {
          static const struct {
              u16 addr;
              u16 size;
              const u8* mem;
          } map[3] = {
              {0, sizeof(boot), boot},
              {4096, sizeof(fat), fat},
              {8192, sizeof(root), root}
          };
          memset(Readbuff, 0, Transfer_Length);
          for(int i = 0; i < 3; i++) {
              u32 end = map[i].addr + map[i].size;
              if(Memory_Offset >= map[i].addr && Memory_Offset < end) {
                  int size = end - Memory_Offset;
                  if (Transfer_Length < size) {
                      size = Transfer_Length;
                  }
                  memcpy(Readbuff, map[i].mem + (Memory_Offset - map[i].addr), size);
              }
          }
          if(0x1FE >= Memory_Offset && 0x1FE < (Memory_Offset + Transfer_Length)) {
              //printf("Setting %06x to %02x\n", 0x1FE - Memory_Offset, 0x55);
              ((u8 *)Readbuff)[0x1FE - Memory_Offset] = 0x55;
          }
          if(0x1FF >= Memory_Offset && 0x1FF < (Memory_Offset + Transfer_Length)) {
              //printf("Setting %06x to %02x\n", 0x1FF - Memory_Offset, 0xAA);
              ((u8 *)Readbuff)[0x1FF - Memory_Offset] = 0xAA;
          }
          break;
      }
#endif
      SPIFlash_ReadBytes(Memory_Offset  + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000), Transfer_Length, (u8*)Readbuff);
      break;
    default:
      return MAL_FAIL;
  }
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_Clear
* Description    : Clear sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Clear(uint8_t lun, uint32_t Memory_Offset)
{
  (void)lun;
  //printf("Erase: (%d)%06x\n", lun, Memory_Offset);
#if EMULATE_FAT
  if (Memory_Offset < (FAT_OFFSET * 0x1000)) {
      return MAL_OK;
  }
#endif
  SPIFlash_EraseSector(Memory_Offset + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000));
  return MAL_OK;
}
/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
  (void)lun;
  return MAL_OK;
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
