//heartint.c
//setup TCNT1 in pwm mode, TCNT3 in normal mode 
//set OC1A (PB5) as pwm output 
//pwm frequency:  (16,000,000)/(1 * (61440 + 1)) = 260hz
//
//Timer TCNT3 is set to interrupt the processor at a rate of 30 times a second.
//When the interrupt occurs, the ISR for TCNTR3 changes the duty cycle of timer 
//TCNT1 to affect the brightness of the LED connected to pin PORTB bit 5.
//
//to download: 
//wget http://www.ece.orst.edu/~traylor/ece473/inclass_exercises/timers_and_counters/heartint_skeleton.c
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#define TRUE  1
#define FALSE 0

//Traverse the array up then down with control statements or double the size of
//the array and make the control easier.  Values from 0x0100 thru 0xEF00 work 
//well for setting the brightness level.

uint16_t brightness[20] = {300, 1000, 5000, 9000, 15000, 30000, 40000, 45000, 47000, 50000, 50000, 47000,
				45000, 40000, 30000, 15000, 9000, 5000, 1000, 300} ;
ISR(TIMER3_OVF_vect) {
	//int i;
        /*for(i = 0; i<10 ; i++){
        	OCR1A = brightness[8];
	}
	for(i = 9; i<=0 ; i--){
        	OCR1A = brightness[8];
	}
	  */
        static uint8_t temp;
	OCR1A = brightness[temp%20];
	temp++;                           
}

int main() {
  DDRB    = (1 << PB5);                          //set port B bit five to output

//setup timer counter 1 as the pwm source

  TCCR1A |= 0 | (1 << WGM11) | (1 << COM1A1) | (1 << COM1A0); //fast pwm, set on match, clear@bottom, 
                                        		  //(inverting mode) ICR1 holds TOP

  TCCR1B |= 0 | (1 << WGM13) | (1 << WGM12)  | (1 << CS10);//use ICR1 as source for TOP, use clk/1

  TCCR1C  = 0;                            //no forced compare 

  ICR1    = 0xF000;                            //clear at 0xF000                               

  
//setup timer counter 3 as the interrupt source, 30 interrupts/sec
// (16,000,000)/(8 * 2^16) = 30 cycles/sec

  TCCR3A = 0;                             //normal mode

  TCCR3B = (1 << CS31);                   //use clk/8  (15hz)  

  TCCR3C = 0;                             //no forced compare 

  ETIMSK = (1 << TOIE3);                  //enable timer 3 interrupt on TOV

  sei();                                //set GIE to enable interrupts
  while(1) { } //do forever
 
}  // main
