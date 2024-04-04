#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include "usart.h"
#include "bit.h"

#define FOSC 16000000 // Frequency Oscillator 16Mhz for Uno R3
#define BAUD 9600 // 9600 Bits per second
#define MYUBRR FOSC / 16 / BAUD - 1 // My USART Baud Rate Register
#define PRESCALER 3 //250 KHz
#define DELAY 200 // 200 ms

#define PORT &PORTB
#define DDR &DDRB
#define PIN PINB0

int setPrescaler_tc0(char option);
int setMax_count_us(float x,
    unsigned char top,
    char prescaler_option);
void my_delay_us(float x);
void set_tc0_mode(char mode);
void my_delay_1e6us();
void my_delay_2s_ctc();
void my_delay_us_ctc(float x,
    unsigned long top,
    char prescaler_option);

volatile unsigned long num0V = 0;

void set_tc0_mode(char mode){
    TCCR0A = 0; //Clear registers
    TCCR0B = 0;
    TCCR0A |= ((mode & 0x03) << WGM00); // Set WGM01 and WGM00
    TCCR0B |= ((mode & 0x04) << WGM02); // Set WGM02
}

void my_delay_2s_ctc()
{
  set_tc0_mode(2);
  OCR0A = 124; // TOP
  unsigned long num0V_max = 125 * 2;

  num0V = 0;
  TCNT0 = 0;
  setPrescaler_tc0(5);
  while (num0V < num0V_max);
  setPrescaler_tc0(0);
}


void my_delay_us_ctc(float x,
    unsigned long top,
    char prescaler_option)
  {
    set_tc0_mode(2);
    OCR0A = top; // TOP
    unsigned long num0V_max = 125 * x;

    num0V = 0;
    TCNT0 = 0;
    setPrescaler_tc0(prescaler_option);
    while (num0V < num0V_max);
    setPrescaler_tc0(0);

  }


// delay of 1 second with default prescaler
void my_delay_1e6us()
{
  unsigned long num0V_max = 62500;
  num0V = 0; // timer0 overflow counter, sets count to 0
  TCNT0 = 0; // timer0 counter register, sets count to 0
  //bitSet(TCCR0B, CS00); // starts timer0
  setPrescaler_tc0(2);
  // repeatedly counts to 255 and overflows at 256 triggering the ISR to increment.
  while (num0V < num0V_max - 1); 
  //bitClear(TCCR0B, CS00); // stops timer0
  setPrescaler_tc0(0);
}

int setMax_count_us(float x,
    unsigned char top,
    char prescaler_option)
{
  unsigned long prescalers[6] = {0, 1, 8, 64, 256, 1024};
  float overflow_count = ((1 / ((float)FOSC / prescalers[PRESCALER])) * top);
  float time_sec = x / 1000000;
  float max = (time_sec / overflow_count);
  unsigned long max_count = (unsigned long)round(max);
  return max_count;
}

// custom delay with prescalers
void my_delay_ms(float x)
{
  int prescale = 1;
  unsigned long top = 256;
  unsigned long num0V_max = setMax_count_us(x, top, prescale);
  num0V = 0; // timer0 overflow counter, sets count to 0
  TCNT0 = 0; // timer0 counter register, sets count to 0
  setPrescaler_tc0(PRESCALER);
  while (num0V < num0V_max - 1); 
  
  setPrescaler_tc0(0);
}

ISR(TIMER0_OVF_vect)
{
  num0V++;
}

ISR(TIMER0_COMPA_vect) {
    num0V++;
}

int setPrescaler_tc0(char option)
{
  switch (option)
  {
  case 0:
    // no clock source 
    bitClear(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 1:
    // full clock: 16 Mhz = 62.5 ns
    bitSet(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 2:
    // 1/8 clock: 2 Mhz = 500 ns
    bitClear(TCCR0B, CS00);
    bitSet(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 3:
    // 1/64 clock: 250 kHz = 4 us
    bitSet(TCCR0B, CS00);
    bitSet(TCCR0B, CS01);
    bitClear(TCCR0B, CS02);
    break;
  case 4:
    // 1/256 clock: 62.5 kHz = 16 us
    bitClear(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitSet(TCCR0B, CS02);
    break;
  case 5:
    // 1/1024 clock: 15.625 kHz = 64 us
    bitSet(TCCR0B, CS00);
    bitClear(TCCR0B, CS01);
    bitSet(TCCR0B, CS02);
    break;
  default:
    return -1;
  }
  return option;
}

int main()
{
  set_tc0_mode(2);
  usart_init(MYUBRR); // 103-9600 bps; 8-115200
  // bitSet(TIMSK0, TOIE0); // enable timer0 overflow interrupt
  bitSet(TIMSK0, OCIE0A); // enable timer0 compare interrupt
  sei(); // enable global interrupts
  bitSet(*DDR, PIN); // set pin 8 as output

  while(1)
  {
    my_delay_us_ctc(2, 124, 5);
    //my_delay_2s_ctc();
    bitSet(*PORT, PIN);
    //my_delay_2s_ctc();
    my_delay_us_ctc(2, 124, 5);
    bitClear(*PORT, PIN);
  }
}

