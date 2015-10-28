// lab2_skel.c 
// R. Traylor
// 9.12.08

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#define INPUT 0xFF
#define SELECT_BIT_BUTTON_BOARD 0x70
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#define ONE	 0xf9
#define TWO	 0xa4
#define THREE	 0xb0
#define FOUR     0x99
#define FIVE     0x92
#define SIX 	 0x82
#define SEVEN 	 0xf8
#define EIGHT 	 0x80
#define NINE 	 0x90
#define ZERO  	 0xc0
#define OFF	 0xff


#define MAX_SEGMENT 5
#define BUTTON_COUNT 3
#define MAX_SUM 1023
#define ENCODE_LEFT_KNOB(read)   (read & 0x0C) >> 2
#define ENCODE_RIGHT_KNOB(read)  (read & 0x03) 
#define CHECK_LEFT_KNOB   0x03
#define CHECK_RIGHT_KNOB  0x0c
//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];
uint8_t dif;
uint8_t modeA = 0;
uint8_t modeB = 0;
uint16_t value = 0;
uint8_t checkButtonNow = 0;
uint8_t previous_spi_value;
uint8_t current_spi_value;
//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 



int8_t chk_buttons(uint8_t button){
    static uint16_t state = 0;
    state = (state << 1) | (bit_is_clear(PINA, button) | 0xE000);
    if (state == 0xF000){
	return 1;
    }
    return 0;

}
//***********************************************************************************
// int2seg
// return the 7-segment code for each digit
uint8_t int2seg(uint8_t number){
    if(number == 0 ){
	return ZERO;
    }
    else if(number == 1 ){
	return ONE;
    }
    else if(number == 2 ){
	return TWO;
    }
    else if(number == 3 ){
	return THREE;
    }
    else if(number == 4 ){
	return FOUR;
    }
    else if(number == 5 ){
	return FIVE;
    }
    else if(number == 6 ){
	return  SIX;
    }
    else if(number == 7 ){
	return SEVEN;
    }
    else if(number == 8 ){
	return EIGHT;
    }
    else if(number == 9 ){
	return NINE;
    }
    else{ 
	return 0;
    }
}
//*******************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|

void segsum(uint16_t sum) {
    //determine how many digits there are 
    int digit;
    // Break down the digits
    if(sum >= 1000){
	digit = 4;
    }
    else if (sum >= 100 && sum < 1000){
	digit = 3;
    }
    else if (sum >= 10 && sum < 100){
	digit = 2;
    }
    else if (sum <10){
	digit = 1;
    }
    //break up decimal sum into 4 digit-segments
    segment_data[0] = int2seg(sum % 10); //ones
    segment_data[1] = int2seg((sum % 100)/10); //tens
    //segment_data[2] = 1; //decimal
    segment_data[3] = int2seg((sum % 1000)/100); //hundreds
    segment_data[4] = int2seg(sum/1000); //thousands
    //blank out leading zero digits 
    switch (digit){
	case 3:
	    segment_data[4] = OFF;
	    break;
	case 2:
	    segment_data[4] = OFF;  	
	    segment_data[3] = OFF;  	
	    break;
	case 1:
	    segment_data[4] = OFF;  	
	    segment_data[3] = OFF;  	
	    segment_data[1] = OFF;  	
	    break;
	default:
	    break;
    }
    //now move data to right place for misplaced colon position
}//segment_sum
//***********************************************************************************
void button_routine(){
    uint8_t button;
    DDRA  = 0x00; // PORTA input mode
    PORTA = 0xFF; //Pull ups
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    //enable tristate buffer for pushbutton switches
    PORTB |= 0x70; //Set S2,S1,S0 to 111
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    //now check each button and increment the count as needed
    for (button = 0 ; button < BUTTON_COUNT ; button++){
	if (chk_buttons(button)){
	    if(button == 0){
		modeA = !modeA;
		//value = 1;
	    }
	    else if( button == 1){
		modeB = !modeB;
		//value = 2;
	    } 
	    else if (button == 2){
		//value = value + 100;
		if (modeA && modeB){
		    //value = 4;
		    dif = 0;
		}
		else if(modeA && !modeB){
		    //value = modeA;
		    // value = 4;
		    dif =  1;
		}
		else if (modeB && !modeA){
		    //value = modeB;
		    dif =  2;
		}

	    }
	}
    }
    //bound the count to 0 - 1023
    if (value > MAX_SUM){
	value = value - MAX_SUM;
    }
    //break up the disp_value to 4, BCD digits in the array: call (segsum)
    //value = 20;
    //bound a counter (0-4) to keep track of digit to display 
    //make PORTA an output
    //value = value+1;
    
    checkButtonNow = 0;
}

ISR(TIMER0_OVF_vect){
    checkButtonNow = 1;
}
void SPI_init(){
    /* Set MOSI and SCK output, all others input */
    //DDRB = (1<<PB3)|(1<<PB1);
	
    /* Enable SPI, Master, set clock rate fck/16 */
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void SPI_Transmit(uint8_t data){
   
   SPDR = data;
   while(!(SPSR & (1<<SPIF))){}

}

uint8_t SPI_Receive(void){
    // Wait until 8 clock cycles are done 
    while (bit_is_clear(SPSR,SPIF)){} 
    // Return incoming data from SPDR
    return(SPDR);  
}

void bar_graph(){
    
    uint8_t write = 0;

    if(modeA){
      write |= 0x01;  
    }

    else if(!modeA){
      write &= 0xFE;
    }

    if(modeB){
      write |= 0x02;
    }
    else if(!modeB){
      write &= 0xFD;
    }
    SPI_Transmit(write);
    PORTD = (1 << PD2);
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
  

    PORTD = (2 << PD2);
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
}

void display_update(){
    int display_segment;

    segsum(value);
    DDRA = 0xFF;
    __asm__ __volatile__ ("nop"); //Buffer
    __asm__ __volatile__ ("nop"); //Buffer 
    for(display_segment = 0 ; display_segment < MAX_SEGMENT ; display_segment++){
	//send PORTB the digit to display
	//value = 1;
	//segsum(value);
	PORTB &= 0x8F;
	PORTB |= display_segment << 4;
	//send 7 segment code to LED segments
	//update digit to display
	PORTA = segment_data[display_segment];	
	_delay_ms(1);
    }
}

int decode_spi_knob(){
    uint8_t spi_read, spi_value_1, spi_value_2;
    spi_read = SPI_Receive();
    value = spi_read;

    if(ENCODE_LEFT_KNOB(spi_read) != 0b11){ //left knob was turned
	spi_value_1 = ENCODE_LEFT_KNOB(spi_read); //Save the value for comparision
        spi_read  = SPI_Receive();
	if(!ENCODE_LEFT_KNOB(spi_read)){ //0 if the cycle is completed
		if(spi_value_1 == 0b01){  
			value = value+1;
                       // return -1; //CCW
		}
		else if(spi_value_1 == 0b10){ //CW
                	//return 1;
		}
		else{
                 	//return 0; //Unknown
		}
	} 
	
    }

    else if(ENCODE_RIGHT_KNOB(spi_read) != 0b11){
	previous_spi_value = ENCODE_RIGHT_KNOB(spi_read); 

    }
}


uint16_t update_number(int sign){
	return value*sign*dif;

}

//***********************************************************************************
int main()
{
    //set port bits 4-7 B as outputs
    DDRE = 0xc0;
    PORTE &= 0x7F;
    DDRB = 0xF7;
    DDRD |= (1 << PB2);

    segment_data[2] = OFF;

    TIMSK |= (1<<TOIE0);             //enable interrupts
    TCCR0 |= (1<<CS02) | (1<<CS00);  //normal mode, prescale by 128
    SPI_init();
    sei();
    while(1){
	//insert loop delay for debounce 
	//PORTA = OFF;
	_delay_ms(2);
	//make PORTA an input port with pullups 
	//bar_graph();
	if(checkButtonNow){
	    button_routine();
	}
	bar_graph();
	value = SPI_Receive();
	//spi_read = SPI_Receive();
	//decode_spi_knob();
	//value = value+update_number(decode_spi_knob());
	//update_number();

	display_update();

    }//while
    return 0;
}//main
