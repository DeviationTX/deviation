#define F_CPU 8000000UL

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define VERSION 0x02

#define CSN_PIN (1 << PB0)
#define SCK_PIN (1 << PA4)

#define CSN (PINB & CSN_PIN)
#define SCK (PINA & SCK_PIN)

#define NUM_PINS 7
uint8_t PIN_MASK[NUM_PINS] = {
    0,
    1,
    2,
    3,
    7,
    0x80 | 1,
    0x80 | 2,
};
register uint8_t global_A asm ("16");
register uint8_t global_NOTA asm ("15");
register uint8_t global_tmp asm ("14");
register uint8_t state asm ("17");
//register uint8_t global_B asm ("14");
//register uint8_t global_NOTB asm ("15");

enum {
    CLEAR = 0,
    CLOCK_RISE,
};

void setup_switch() {
    uint8_t i = 0;
    uint8_t data[2];
    TCNT0 = 0;
    TCCR0B = (1 << CS01) | (1 << CS00); // div-by-64 prescalar = ~2msec overflow
    DDRA = (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA7) | (1 << PA5);  //Enable MISO output

    USICR = (1 << USIWM0) | (1 << USICS1);  // 3-sire (SPI), slave, rising clock
    USISR = 1 << USIOIF;                    // Reset USI interrupt flag
    USIDR = 0xA5;
    while(i < 2) {
        if (TCNT0 >= 125) {  //Abort after 1 msec
            USICR = 0x00;   // disable SPI
            return;
        }
        if (USISR & (1 << USIOIF)) {
            data[i] = USIDR;
            USISR = 1 << USIOIF;
            USIDR = VERSION;
            i++;
        }
    }
    USICR = 0x00; // disable SPI
    DDRA = (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA7);
    PORTA = data[0];      //Set PA.0, PA.1, PA,2, PA.3, PA.7
    PORTB = (data[0] >> 3) & 0x06; //Set PB.1 = BIT5, PB.2 = BIT6
    global_A = data[0];
    global_NOTA = data[1];
    PORTA = global_A;   //CSN must be high
    //PORTB = global_B;
}

ISR(PCINT1_vect, ISR_NAKED) {
    //CSN change
    #if 0
    if (CSN) {
        PORTA = global_A;
        //PORTB = global_B;
    } else {
        PORTA = global_NOTA;
        //PORTB = global_NOTB;
    }
    #else
    asm("mov r14, r16");
    asm("sbis 0x16, 0");
    asm("mov r14, r15");
    asm("out 0x1b, r14");
    #endif
    state = CLEAR;
    //asm("ldi r17, 0x01"); //state = CSN_CHANGED;
    reti();
}



int main (void)
{
    state = CLEAR;
    global_A = 0x0f;
    global_NOTA = 0x0f;
    //global_B = 0;
    //global_NOTB = ~0;
    DDRA = (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA7);
    DDRB = (1 << PB1) | (1 << PB2);
    PORTA = global_A;
    PCMSK0 = SCK_PIN;      //Watch for clock toggle (PCINT4)
    PCMSK1 = CSN_PIN;      //Watch for CSN toggle   (PCINT8)
    GIMSK  = 1 << PCIE1;   //Enable pin-change interrupt #1 (PCINT8-11)
    GIFR = (1 << PCIF0) | (1 << PCIF1);  //Reset pin-change interrupt flags
    SREG   |= 0x80;        //Enable interrupts
    while(1)
    {
        if (CSN) {
            if (GIFR & (1 << PCIF0)) {  //check for clock toggle
                if (SCK) { 
                    if (CSN) {
                        //clock rise with CSN=1 ==> start of trigger
                        state = CLOCK_RISE;
                    }
                } else {
                    if (state != CLEAR) {
                        //clock fall with state == CLOCK_RISE and CSN=1 ==> completion of trigger
                        GIMSK  = 0;            //Disable PCINT1 interrupt
                        setup_switch();
                        GIMSK  = 1 << PCIE1;   //Enable PCINT1 interrupt
                    } 
                    state = CLEAR;
                }
                GIFR = 1 << PCIF0;  //clear clock toggle
           }
        } else {
           GIFR = 1 << PCIF0;  //clear clock toggle
        }
    }
}

