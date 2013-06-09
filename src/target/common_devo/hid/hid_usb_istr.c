/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_istr.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : ISTR events interrupt service routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "hid_usb_prop.h"
#include "hid_usb_istr.h"
#include "usb_pwr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t wIstr;  /* ISTR register last read value */

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* function pointers to non-control endpoints service routines */
void (*HID_pEpInt_IN[7])(void) =
  {
    HID_EP1_IN_Callback,
    HID_EP2_IN_Callback,
    HID_EP3_IN_Callback,
    HID_EP4_IN_Callback,
    HID_EP5_IN_Callback,
    HID_EP6_IN_Callback,
    HID_EP7_IN_Callback,
  };

void (*HID_pEpInt_OUT[7])(void) =
  {
    HID_EP1_OUT_Callback,
    HID_EP2_OUT_Callback,
    HID_EP3_OUT_Callback,
    HID_EP4_OUT_Callback,
    HID_EP5_OUT_Callback,
    HID_EP6_OUT_Callback,
    HID_EP7_OUT_Callback,
  };

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

