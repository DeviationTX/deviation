#ifndef _FRSKY9XD_PORTS_H_
#define _FRSKY9XD_PORTS_H_

//ADC overrides
#define _ADC
#define ADC_OVERSAMPLE_WINDOW_COUNT  1
//End ADC

//Sound port
//#define _SOUND_PORT                GPIOB
//#define _SOUND_PIN                 GPIO0
//#define _SOUND_RCC_APB1ENR_TIMEN   RCC_APB1ENR_TIM3EN
//#define _SOUND_TIM_OC              TIM_OC3
//#define _SOUND_TIM                 TIM3
//End Sound port

//PWM Port
#define _PWM_PIN                   GPIO8
#define _PWM_EXTI                  EXTI8
#define _PWM_TIM_OC                TIM_OC1
//End PWM Port

#define _USART USART3

//SD Card
#define RCC_GPIO_CP                     RCC_AHB1ENR_IOPDEN
#define GPIO_PORT_CP                    GPIOD
#define GPIOCP                          GPIO9  //PD.09

#define RCC_GPIO_PORT_CS		RCC_AHB1ENR_IOPBEN
#define GPIO_PORT_CS                    GPIOB
#define GPIOCS                          GPIO12 //PB.12

#define GPIO_PORT_SPI_SD		GPIOB
#define GPIOSPI_SD_SCK                  GPIO13 //PB.13
#define GPIOSPI_SD_MISO                 GPIO14 //PB.14
#define GPIOSPI_SD_MOSI                 GPIO15 //PB.15
#define RCC_SPI                         RCC_APB1ENR
#define RCC_SPI_SD                      RCC_APB1ENR_SPI2EN

#define SPI_SD                          SPI2
#define GPIO_AF_SD                      GPIO_AF5
#define RCC_GPIO                        RCC_AHB1ENR
#define SPI_BaudRatePrescaler_SPI_SD	SPI_CR1_BAUDRATE_FPCLK_DIV_4 //10.5<20MHZ,make sure < 20MHZ
#define SPI_BaudRatePrescaler_slow	SPI_CR1_BR_FPCLK_DIV_128
#define SPI_BaudRatePrescaler_fast	SPI_CR1_BAUDRATE_FPCLK_DIV_4

//disabled io_ctl and cp/wp
#define CARD_SUPPLY_SWITCHABLE   	0
#define SOCKET_WP_CONNECTED      	0
#define SOCKET_CP_CONNECTED      	0
/* set to 1 to provide a disk_ioctrl function even if not needed by the FatFs */
#define STM32_SD_DISK_IOCTRL_FORCE      0

//DMA
#define STM32_SD_USE_DMA //Enable the DMA for SD 

#ifdef STM32_SD_USE_DMA
#define DMA_STREAM_SPI_SD_RX     	DMA_STREAM3
#define DMA_STREAM_SPI_SD_TX    	DMA_STREAM4

#define DMA_Channel_SPI_RX		DMA_SxCR_CHSEL_0
#define DMA_Channel_SPI_TX		DMA_SxCR_CHSEL_0
#endif

#endif //_FRSKY9XD_PORTS_H_
