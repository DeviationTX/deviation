#define F_CPU 8000000UL

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

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
register uint8_t global_NOTA asm ("17");
register uint8_t global_tmp asm ("14");
//register uint8_t global_B asm ("14");
//register uint8_t global_NOTB asm ("15");


void setup_switch() {
    uint8_t i = 0;
    uint8_t data[2];
    TCNT0 = 0;
    TCCR0B = (1 << CS01) | (1 << CS00); // div-by-64 prescalar = ~2msec overflow

    USICR = (1 << USIWM0) | (1 << USICS1);  // 3-sire (SPI), slave, rising clock
    USISR = 1 << USIOIF;                    // Reset USI interrupt flag
    while(i < 2) {
        if (TCNT0 >= 125) {  //Abort after 1 msec
            USICR = 0x00;   // disable SPI
            return;
        }
        if (USISR & (1 << USIOIF)) {
            data[i] = USIDR;
            USISR = 1 << USIOIF;
            i++;
        }
    }
    USICR = 0x00; // disable SPI
    TCCR0B = 0; // Disable Timer0
    uint8_t setA = 0, toggleA = 0;
    //uint8_t setB = 0, toggleB = 0;
    for(i = 0; i < NUM_PINS; i++) {
        //Set each pin to the proper value
        uint8_t is_toggle = data[1] & (1 << i);
        if (is_toggle) {
            //if (PIN_MASK[i] & 0x80) {
            //    toggleB |= 1 << (PIN_MASK[i] & 0x7f);
            //} else {
                toggleA |= 1 << (PIN_MASK[i] & 0x7f);
            //}
        } else if (data[0] & (1 << i)) {
            //if (PIN_MASK[i] & 0x80) {
            //    setB |= 1 << (PIN_MASK[i] & 0x7f);
            //} else {
                setA |= 1 << (PIN_MASK[i] & 0x7f);
            //}
        }
    }
    global_A = toggleA | setA;
    global_NOTA = ~toggleA & setA;
    //global_B = toggleB | setB;
    //global_NOTB = ~toggleB & setB;
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
    asm("mov r14, r17");
    asm("out 0x1b, r14");
    #endif
    reti();
}

enum {
    CLEAR = 0,
    CLOCK_RISE  ,
};

int main (void)
{
    uint8_t state= CLEAR;
    global_A = 0xff;
    global_NOTA = ~0;
    //global_B = 0;
    //global_NOTB = ~0;
    DDRA = (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA7);
    DDRB = (1 << PB1) | (1 << PB2);
    PCMSK0 = SCK_PIN;      //Watch for clock toggle
    PCMSK1 = CSN_PIN;      //Watch for CSN toggle
    GIMSK  = 1 << PCIE1;   //Enable PCINT1 interrupt
    GIFR = (1 << PCIF0) | (1 << PCIF1);
    SREG   |= 0x80;        //Enable interrupts
    while(1)
    {
        //PORTB ^= 1 << PB0;
        //_delay_ms(500);
        if (GIFR & (1 << PCIF0)) {  //check for clock toggle
            if (SCK && CSN) {
                //clock rise with CSN=1 ==> start of trigger
                state = CLOCK_RISE;
                } else if (state == CLOCK_RISE && ! SCK && CSN) {
                //clock fall with state == CLOCK_RISE and CSN=1 ==> completion of trigger
                state = CLEAR;
                GIMSK  = 0;            //Disable PCINT1 interrupt
                setup_switch();
                GIMSK  = 1 << PCIE1;   //Enable PCINT1 interrupt
            }
            GIFR = 1 << PCIF0;  //clear clock toggle
            } else if(! CSN) {
            state = CLEAR;
        }
    }
}
