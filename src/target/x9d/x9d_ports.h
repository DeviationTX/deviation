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
#define _PWM_PIN GPIO8
#define _PWM_EXTI EXTI8
//End PWM Port

#define _USART USART3
#endif //_FRSKY9XD_PORTS_H_
