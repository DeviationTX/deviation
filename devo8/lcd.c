#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/stm32/timer.h>
#include "../tx.h"

#define LCD_REG_ADDR  ((uint32_t)FSMC_BANK1_BASE)    /* Register Address */
#define LCD_DATA_ADDR  ((uint32_t)FSMC_BANK1_BASE + 0x20000)  /* Data Address */

#define LCD_REG  *(volatile uint16_t *)(LCD_REG_ADDR)
#define LCD_DATA *(volatile uint16_t *)(LCD_DATA_ADDR)

void Delay(uint32_t);

void lcd_cmd(uint8_t addr, uint8_t data)
{
    LCD_REG = addr;
    LCD_DATA = data;
}
void lcd_draw(unsigned int color)
{
    LCD_DATA = color;
}
void lcd_drawstart(void)
{
    LCD_REG = 0x22;
}
void lcd_drawstop(void)
{
  return;
}

void lcd_area(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
  lcd_cmd(0x03, (x0>>0)); //set x0
  lcd_cmd(0x02, (x0>>8)); //set x0
  lcd_cmd(0x05, (x1>>0)); //set x1
  lcd_cmd(0x04, (x1>>8)); //set x1
  lcd_cmd(0x07, (y0>>0)); //set y0
  lcd_cmd(0x06, (y0>>8)); //set y0
  lcd_cmd(0x09, (y1>>0)); //set y1
  lcd_cmd(0x08, (y1>>8)); //set y1

  return;
}
unsigned char _0[8] = {0x1C,0x22,0x26,0x2A,0x32,0x22,0x1C,0x00};
unsigned char _1[8] = {0x08,0x18,0x08,0x08,0x08,0x08,0x1C,0x00};
unsigned char _2[8] = {0x1C,0x22,0x02,0x04,0x08,0x10,0x3E,0x00};
unsigned char _3[8] = {0x3E,0x04,0x08,0x04,0x02,0x22,0x1C,0x00};
unsigned char _4[8] = {0x04,0x0C,0x14,0x24,0x3E,0x04,0x04,0x00};
unsigned char _5[8] = {0x3E,0x20,0x3C,0x02,0x02,0x22,0x1C,0x00};
unsigned char _6[8] = {0x0C,0x10,0x20,0x3C,0x22,0x22,0x1C,0x00};
unsigned char _7[8] = {0x3E,0x22,0x02,0x04,0x08,0x08,0x08,0x00};
unsigned char _8[8] = {0x1C,0x22,0x22,0x1C,0x22,0x22,0x1C,0x00};
unsigned char _9[8] = {0x1C,0x22,0x22,0x1E,0x02,0x04,0x18,0x00};
unsigned char _A[8] = {0x1C,0x22,0x22,0x22,0x3E,0x22,0x22,0x00};
unsigned char _B[8] = {0x3C,0x22,0x22,0x3C,0x22,0x22,0x3C,0x00};
unsigned char _C[8] = {0x1C,0x22,0x20,0x20,0x20,0x22,0x1C,0x00};
unsigned char _D[8] = {0x38,0x24,0x22,0x22,0x22,0x24,0x38,0x00};
unsigned char _E[8] = {0x3E,0x20,0x20,0x3C,0x20,0x20,0x3E,0x00};
unsigned char _F[8] = {0x3E,0x20,0x20,0x3C,0x20,0x20,0x20,0x00};
unsigned char _G[8] = {0x1C,0x22,0x20,0x2E,0x22,0x22,0x1E,0x00};
unsigned char _H[8] = {0x22,0x22,0x22,0x3E,0x22,0x22,0x22,0x00};
unsigned char _I[8] = {0x1C,0x08,0x08,0x08,0x08,0x08,0x1C,0x00};
unsigned char _J[8] = {0x0E,0x04,0x04,0x04,0x04,0x24,0x18,0x00};
unsigned char _K[8] = {0x22,0x24,0x28,0x30,0x28,0x24,0x22,0x00};
unsigned char _L[8] = {0x20,0x20,0x20,0x20,0x20,0x20,0x3E,0x00};
unsigned char _M[8] = {0x22,0x36,0x2A,0x2A,0x22,0x22,0x22,0x00};
unsigned char _N[8] = {0x22,0x22,0x32,0x2A,0x26,0x22,0x22,0x00};
unsigned char _O[8] = {0x1C,0x22,0x22,0x22,0x22,0x22,0x1C,0x00};
unsigned char _P[8] = {0x3C,0x22,0x22,0x3C,0x20,0x20,0x20,0x00};
unsigned char _Q[8] = {0x1C,0x22,0x22,0x22,0x2A,0x24,0x1A,0x00};
unsigned char _R[8] = {0x3C,0x22,0x22,0x3C,0x28,0x24,0x22,0x00};
unsigned char _S[8] = {0x1E,0x20,0x20,0x1C,0x02,0x02,0x3C,0x00};
unsigned char _T[8] = {0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0x00};
unsigned char _U[8] = {0x22,0x22,0x22,0x22,0x22,0x22,0x1C,0x00};
unsigned char _V[8] = {0x22,0x22,0x22,0x22,0x22,0x14,0x08,0x00};
unsigned char _W[8] = {0x22,0x22,0x22,0x2A,0x2A,0x2A,0x14,0x00};
unsigned char _X[8] = {0x22,0x22,0x14,0x08,0x14,0x22,0x22,0x00};
unsigned char _Y[8] = {0x22,0x22,0x22,0x14,0x08,0x08,0x08,0x00};
unsigned char _Z[8] = {0x3E,0x02,0x04,0x08,0x10,0x20,0x3E,0x00};
unsigned char _none[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
void lcd_writeCharacter(unsigned char character, unsigned int x, unsigned int y){
	uint8_t i,j;
	unsigned char *ptr;

	switch(character){
		case 'A': ptr = _A;
			break;
		case 'B': ptr = _B;
			break;
		case 'C': ptr = _C;
			break;
		case 'D': ptr = _D;
			break;
		case 'E': ptr = _E;
			break;
		case 'F': ptr = _F;
			break;
		case 'G': ptr = _G;
			break;
		case 'H': ptr = _H;
			break;
		case 'I': ptr = _I;
			break;
		case 'J': ptr = _J;
			break;
		case 'K': ptr = _K;
			break;
		case 'L': ptr = _L;
			break;
		case 'M': ptr = _M;
			break;
		case 'N': ptr = _N;
			break;
		case 'O': ptr = _O;
			break;
		case 'P': ptr = _P;
			break;
		case 'Q': ptr = _Q;
			break;
		case 'R': ptr = _R;
			break;
		case 'S': ptr = _S;
			break;
		case 'T': ptr = _T;
			break;
		case 'U': ptr = _U;
			break;
		case 'V': ptr = _V;
			break;
		case 'W': ptr = _W;
			break;
		case 'X': ptr = _X;
			break;
		case 'Y': ptr = _Y;
			break;
		case 'Z': ptr = _Z;
			break;
		case '0': ptr = _0;
			break;
		case '1': ptr = _1;
			break;
		case '2': ptr = _2;
			break;
		case '3': ptr = _3;
			break;
		case '4': ptr = _4;
			break;
		case '5': ptr = _5;
			break;
		case '6': ptr = _6;
			break;
		case '7': ptr = _7;
			break;
		case '8': ptr = _8;
			break;
		case '9': ptr = _9;
			break;
		default: ptr = _none;
			break;
	}

	lcd_area(x, y, (x+6), (y+7));
	lcd_drawstart();
	for(i=0; i<8; i++){
		for(j=7;j>0; j--){
			if(((ptr[i]>>(j-1))&0x01) == 1){
				lcd_draw(0xFFFF);
			}else{
				lcd_draw(0x0000);
			}
		}
	}
	/*for(i=0;i<8;i++){
		lcd_draw(0xFFFF);
	}*/
	lcd_drawstop();
}
void Initialize_LCD()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_FSMCEN);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);

    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7 | GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO11);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO4 | GPIO5);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7);

    /* Extended mode, write enable, 16 bit access, bank enabled */
    FSMC_BCR1 = FSMC_BCR_MWID | FSMC_BCR_WREN | FSMC_BCR_MBKEN;

    /* Read & write timings */
    FSMC_BTR1  = FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);
    FSMC_BWTR1 = FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);

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
}

void lcd_clear(unsigned int color){
        uint16_t zeile, spalte;
        lcd_area(0, 0, (320-1), (240-1));
        lcd_drawstart();
        for(zeile = 0; zeile < 240; zeile++){
                for(spalte = 0; spalte < 320; spalte++){
                        lcd_draw(color);
                }
        }
        lcd_drawstop();

        return;
}

void Initialize_Backlight()
{

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO1);

    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM3EN);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT,
                    TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, 0);
    timer_set_repetition_counter(TIM3, 0);
    timer_set_period(TIM3, 0x2CF);


    timer_set_oc_mode(TIM3, TIM_OC4, TIM_OCM_PWM1);
    timer_set_oc_value(TIM3, TIM_OC4, 0x168);
    timer_enable_oc_output(TIM3, TIM_OC4);
}

