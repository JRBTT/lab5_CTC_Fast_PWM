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
#define PRESCALER 3 //16Mhz
#define DELAY 2000 // 200ms

#define PORT &PORTB
#define DDR &DDRB
#define PIN PINB0

int setPrescaler_tc0(int option);
unsigned long setMax_count_ms(float x,
    unsigned long top,
    int prescaler_option);
void my_delay_us(float x,
  int prescaler_option);
void set_tc0_mode(char mode);
void my_delay_2s_ctc();
void my_delay_ms_ctc(float x,
    unsigned long top,
    int prescaler_option);

volatile unsigned long num0V = 0;

void set_tc0_mode(int mode){
    TCCR0A = 0; //Clear registers
    TCCR0B = 0;
    TCCR0A |= ((mode & 0x03) << WGM00); // Set WGM01 and WGM00
    TCCR0B |= ((mode & 0x04) << WGM02); // Set WGM02
    //bitSet(TCCR0A, WGM01); // CTC mode
}

void my_delay_2s_ctc()
{
  set_tc0_mode(2);
  OCR0A = 124; // TOP
  bitSet(TIMSK0, OCIE0A); 
  unsigned long num0V_max = 125 * 2;

  num0V = 0;
  TCNT0 = 0;
  setPrescaler_tc0(5);
  while (num0V < num0V_max);
  setPrescaler_tc0(0);
  bitClear(TIMSK0, OCIE0A);
}

unsigned long setMax_count_ms(float x,
    unsigned long top,
    int prescaler_option)
{

  int prescalers[6] = {0, 1, 8, 64, 256, 1024};
  //float overflow_count = ((1 / ((float)FOSC / prescalers[prescaler_option])) * top);
  float overflow_count = (top+1)*(prescalers[prescaler_option]/(float)FOSC);
  // usart_transmit('1');
  // usart_transmit('\n');
  float time_sec = x / 1000;
  float max = (time_sec / overflow_count);
  unsigned long max_count = (unsigned long)max;
  return max_count;
}

void my_delay_ms_ctc(float x,
    unsigned long top,
    int prescaler_option)
    {
      //bitSet(TIMSK0, OCIE0A); 
      //bitSet(TIFR0, OCF0A);
      OCR0A = top;
      num0V = 0;
      TCNT0 = 0;
      unsigned long num0V_max = 200;//setMax_count_ms(x, top, prescaler_option);
      setPrescaler_tc0(prescaler_option);
      while (num0V < num0V_max);
      setPrescaler_tc0(0);
      //bitClear(TIMSK0, OCIE0A);
    }

// custom delay with prescalers
void my_delay_us(float x,
  int prescaler_option)
{
  set_tc0_mode(0);
  bitSet(TIMSK0, TOIE0); // enable timer0 overflow interrupt
  unsigned long num0V_max = 76875;
  num0V = 0; // timer0 overflow counter, sets count to 0
  TCNT0 = 0; // timer0 counter register, sets count to 0
  setPrescaler_tc0(prescaler_option);
  while (num0V < num0V_max - 1); 
  setPrescaler_tc0(0);
  bitClear(TIMSK0, TOIE0);
}

// ISR(TIMER0_OVF_vect)
// {
//   num0V++;
// }

ISR(TIMER0_COMPA_vect) 
{
  num0V++;
}

int setPrescaler_tc0(int option)
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
  usart_init(MYUBRR); // 103-9600 bps; 8-115200
  cli();
  bitSet(TIMSK0, OCIE0A); // enable timer0 output compare interrupt
  bitSet(*DDR, PIN); // set pin 8 as output
  set_tc0_mode(2); // CTC mode
  //bitSet(TCCR0A, WGM01); // CTC mode
  sei(); // enable global interrupts
  
  
  while (1)
  {
    bitSet(*PORT, PIN);
    // usart_tx_string("a:>");
    // usart_transmit('1');
    // usart_transmit('\n');
    my_delay_ms_ctc(DELAY, 124, 3);
    //my_delay_2s_ctc();
    //my_delay_us(DELAY, 1);
    bitClear(*PORT, PIN);
    // usart_tx_string("a:>");
    // usart_transmit('0');
    // usart_transmit('\n');
    //my_delay_2s_ctc();
    my_delay_ms_ctc(DELAY, 124, 3);
    //my_delay_us(DELAY, 1);
  }
}

