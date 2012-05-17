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

void SPIFlash_EraseSector(u32 sectorAddress);
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, u8 * buffer);
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#define SECTOR_OFFSET 36
uint32_t Mass_Memory_Size[2] = {0x1000 * (1024 - SECTOR_OFFSET), 0};
uint32_t Mass_Block_Size[2] = {4096, 0};
uint32_t Mass_Block_Count[2] = {1024 - SECTOR_OFFSET, 0};
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
      SPIFlash_WriteBytes(Memory_Offset  + (SECTOR_OFFSET * 0x1000), Transfer_Length, (u8 *)Writebuff);
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
      SPIFlash_ReadBytes(Memory_Offset  + (SECTOR_OFFSET * 0x1000), Transfer_Length, (u8*)Readbuff);
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
  SPIFlash_EraseSector(Memory_Offset + (SECTOR_OFFSET * 0x1000));
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
  return MAL_OK;
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
