// siren.c  - R. Traylor  - 10.31.2013
// 
// Setup TCNT1 to be a varaible frequency tone generator. Its made variable by dynamically 
// changing the compare register.
//
// Setup TCNT3 to interrupt the processor at a rate of 1000 times a second.  When the 
// interrupt occurs, the ISR for TCNTR3 changes the frequency of timer TCNT1 to affect 
// a continually changing audio frequency at PORTB bit 5.
//
// set OC1A (PB5) as the audio output signal PORTB (PB7) is the PWM signal for creating 
// the volume control.
//
// Timer TCNT3 is set to interrupt the processor at a rate of 1000 times 
// a second.  When the interrupt occurs, the ISR for TCNTR3 changes the
// frequency of timer TCNT1 to affect a continually changing audio
// frequency at PORTB bit 5.
//
// to download: 
// wget http://www.ece.orst.edu/~traylor/ece473/inclass_exercises/timers_and_counters/siren_skeleton.c

#include <avr/io.h>
#include <avr/interrupt.h>

ISR(                                ) {
//OCR1A values that work well are from 10000 to 65000
//the values should increment and decrement by about 64
//
  static uint16_t count=0;
  static uint8_t up=1;
    if(up==1){ 
        if(count < 62000){ count = count + 64;}
        else {count = count - 64; up=0;} 
    }  
    if(up==0){
        if(count > 10096){ count = count - 64 ;}
        else{count = count + 64; up= 1;}
    }
  OCR1A = count;
}
                                   

int main(){

  DDRB   |=                          //set port B bit five and seven as outputs

//setup TCNT1

  TCCR1A |=                          //CTC mode with output pin on 

  TCCR1B |=                          //use OCR1A as source for TOP, use clk/1

  TCCR1C =                           //no forced compare 

//setup TCNT3
// siren update frequency = (16,000,000)/(OCR3A) ~ set to about 1000 cycles/sec

  TCCR3A =                           //CTC mode, output pin does not toggle 

  TCCR3B =                           //no prescaling      

  TCCR3C =                           //no forced compare 

  OCR3A =                            //pick a speed from 0x1000 to 0xF000

  ETIMSK =                           //enable timer 3 interrupt on OCIE3A
 
 //TCNT2 setup for providing the volume control
 //fast PWM mode, TOP=0xFF, clear on match, clk/128
 //output is on PORTB bit 7 
 TCCR2 =  (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<COM20) | (1<<CS20) | (1<<CS21);
 OCR2  = 0x90;  //set to adjust volume control 

  sei();     //set GIE to enable interrupts
  while(1){} //do forever
 
}  // main
