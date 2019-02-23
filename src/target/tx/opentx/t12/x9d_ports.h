#ifndef _FRSKY9XD_PORTS_H_
#define _FRSKY9XD_PORTS_H_

//Sound port
//#define _SOUND_PORT                GPIOB
//#define _SOUND_PIN                 GPIO0
//#define _SOUND_RCC_APB1ENR_TIMEN   RCC_APB1ENR_TIM3EN
//#define _SOUND_TIM_OC              TIM_OC3
//#define _SOUND_TIM                 TIM3
//End Sound port

#define _USART USART3

//disabled io_ctl and cp/wp
#define CARD_SUPPLY_SWITCHABLE   	0
#define SOCKET_WP_CONNECTED      	0
#define SOCKET_CP_CONNECTED      	0
/* set to 1 to provide a disk_ioctrl function even if not needed by the FatFs */
#define STM32_SD_DISK_IOCTRL_FORCE      0
#define STM32_SD_USE_DMA //Enable the DMA for SD 


#endif //_FRSKY9XD_PORTS_H_
