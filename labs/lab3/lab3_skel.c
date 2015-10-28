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

#define NOT_SPIN 0x03
#define CW_INIT 0x02
#define CW_MID  0x00
#define CW_END  0x01

#define CCW_INIT 0x01 
#define CCW_MID  0x00
#define CCW_END  0x02

#define CW  1
#define CCW 2

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];
uint8_t dif = 0;;
uint8_t modeA = 0;
uint8_t modeB = 0;
uint16_t value = 0;
uint8_t checkButtonNow = 0;
uint8_t left_encoder_state = 0;
uint8_t right_encoder_state = 0;
uint8_t left_previous_state = NOT_SPIN;
uint8_t right_previous_state = 0xff;
uint8_t read_dir = 99;
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
    PORTE &= 0;
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    // Wait until 8 clock cycles are done 
    SPDR = 0x00;
    PORTE |= (1 << PE6);  
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
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
void decode_spi_left_knob(void){

}

void decode_spi_right_knob(){
    static uint8_t sw_table[] = {0, 1, 2, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0, 2, 1, 0};
    uint8_t sw_index = 0;
    static uint8_t acount1 = 0;
    static uint8_t previous_encoder1 = 0;
    static uint8_t acount2 = 0;
    static uint8_t previous_encoder2 = 0;
    uint8_t direction = 0;
    uint8_t encoder = SPI_Receive();
    uint8_t encoder1, encoder2; 
    encoder1 = ENCODE_LEFT_KNOB(encoder);
    encoder2 = ENCODE_RIGHT_KNOB(encoder);

    //Encode left knob
    sw_index = (previous_encoder1 << 2) | encoder1;
    direction = sw_table[sw_index];
    //value = acount2;
    if(direction == CW){
	value = value + 1;
	acount1++;
    }	
    if(direction == CCW){
	acount1--;
    }
    if(encoder1 == 3){
	if((acount1 > 1) && (acount1 < 3)){
	    value = value + dif;
	}
	if ((acount1 <= 0xFF) && (acount1 > 0x90)){
	    value = value - 1;
	}
	acount1 = 0;
    }
    previous_encoder1 = encoder1;

    //Encoder 2 (right knob)
    sw_index = (previous_encoder2 << 2) | encoder2;
    direction = sw_table[sw_index];
    //value = acount2;
    if(direction == CW){
	value = value + 1;
	acount2++;
    }	
    if(direction == CCW){
	acount2--;
    }
    if(encoder2 == 3){
	if((acount2 > 1) && (acount2 < 3)){
	    value = value + 1;
	}
	if ((acount2 <= 0xFF) && (acount2 > 0x90)){
	    value = value - 1;
	}
	acount2 = 0;
    }
    previous_encoder2 = encoder2;
}




int decode_spi_knob_2(){
    uint8_t spi_read;
    static int left_encoder_dir;
    static int right_encoder_dir;
    static uint16_t counter = 0;
    uint8_t left_data, right_data;
    spi_read = SPI_Receive();
    left_data = ENCODE_LEFT_KNOB(spi_read); 
    //value = left_data;
    /*if (left_data != NOT_SPIN){
      left_encoder_state = 1;	
      left_previous_state = left_data;
      /value = value + 1;
      } */
    //value = left_previous_state;
    /*    
	  if ((left_data == CW_INIT) && left_previous_state == NOT_SPIN){
    //value = value + 1;
    left_previous_state = CW_INIT;
    }

    else if ((left_data == CW_MID) &&  (left_previous_state == CW_INIT)){
    left_previous_state = CW_MID;
    }
    else if ((left_data == CW_END) &&  (left_previous_state == CW_MID)){
    // value = value + 10;
    left_previous_state = CW_END;
    }
    else if ((left_data == NOT_SPIN) &&  (left_previous_state == CW_MID)){
    left_previous_state = NOT_SPIN;
    // value = value + 10;
    }
    else{
    if(counter >= 20000){
    left_previous_state = NOT_SPIN;
    counter = 0;
    }
    else{
    counter++;
    }
    }
     */
    // Left CCW

    //value = read_dir;
    if (read_dir == 99){
	if ((left_data == CCW_INIT) && left_previous_state == NOT_SPIN){
	    //value = value + 1;
	    left_previous_state = CCW_INIT;
	    read_dir = 0; //CCW
	}
	else if((left_data == CW_INIT) && (left_previous_state == NOT_SPIN)){
	    left_previous_state = CW_INIT;
	    read_dir = 1; //CW
	}
    }
    else if (read_dir == 0){
	if ((left_data == CCW_MID) &&  (left_previous_state == CCW_INIT)){
	    left_previous_state = CCW_MID;
	}

	else if ((left_data == CCW_END) &&  (left_previous_state == CCW_MID)){
	    // value = value + 10;
	    left_previous_state = CCW_END;
	}

	else if ((left_data == NOT_SPIN) &&  (left_previous_state == CCW_END)){
	    left_previous_state = NOT_SPIN;
	    value = value - 10;
	}
	else {
	    if(counter >= 200){
		left_previous_state = NOT_SPIN;
		counter = 0;
		read_dir = 99;
	    }
	    else{
		counter++;
	    }
	}
    }
    else if (read_dir == 1){
	if ((left_data == CW_MID) &&  (left_previous_state == CW_INIT)){
	    left_previous_state = CW_MID;
	}

	else if ((left_data == CW_END) &&  (left_previous_state == CW_MID)){
	    value = value + 10;
	    left_previous_state = CW_END;
	}

	//else if ((left_data == NOT_SPIN) &&  (left_previous_state == CW_END)){
	//    left_previous_state = NOT_SPIN;
	//    value = value + 10;
	//}
	else {
	    if(counter >= 10){
		left_previous_state = NOT_SPIN;
		counter = 0;
		read_dir = 99;
	    }
	    else{
		counter++;
	    }
	}
    } 
}
/*       
	 value = left_data;
	 if(left_previous_state = 0xff){	
	 if(ENCODE_LEFT_KNOB(spi_read) != 0b11){ //left knob was turned
	 left_encoder_state = 1;
	 }
	 if(ENCODE_RIGHT_KNOB(spi_read) != 0b11){ //right knob was turned
	 right_encoder_state = 1;
	 }

	 }
//value =  ENCODE_LEFT_KNOB(spi_read); 
switch (left_encoder_state){
case 1: //When detecting a spin
if( ENCODE_LEFT_KNOB(spi_read) == left_previous_state){
return 0;
}	
if(ENCODE_LEFT_KNOB(spi_read) == 0x02){ //CW spin
left_encoder_dir = 1;     //Direction = positive

value = left_data;
if(left_previous_state = 0xff){	
if(ENCODE_LEFT_KNOB(spi_read) != 0b11){ //left knob was turned
left_encoder_state = 1;
}
if(ENCODE_RIGHT_KNOB(spi_read) != 0b11){ //right knob was turned
right_encoder_state = 1;
}

}
//value =  ENCODE_LEFT_KNOB(spi_read); 
switch (left_encoder_state){
case 1: //When detecting a spin
if( ENCODE_LEFT_KNOB(spi_read) == left_previous_state){
return 0;
}	
if(ENCODE_LEFT_KNOB(spi_read) == 0x02){ //CW spin
left_encoder_dir = 1;     //Direction = positive
left_encoder_state += 1;  //Go to next state
left_previous_state = 0x02;  //Save value
return 0;
}
else if(ENCODE_LEFT_KNOB(spi_read) == 0x01){ //CCW spin
left_encoder_dir = -1;   //Direction = negative
left_encoder_state +=1;  //Go to next state
left_previous_state = 0x01;//Save for next state
return 0;
}
else{
//left_encoder_dir = 0;   //Reset direction to 0
//left_encoder_state = 0; //Reset state to 0   
//previous_state = 0xFF; //Reset previous to default
return 0;
}    
case 2: //Next stage of spin
//Last stage is coming towart CW
if( ENCODE_LEFT_KNOB(spi_read) == left_previous_state){
return 0;
}	
if((ENCODE_LEFT_KNOB(spi_read) == 0x00) && (left_previous_state == 0x02)){
left_encoder_dir = 1;  //Ensure the direction is CW
left_encoder_state += 1;  //Next state
left_previous_state =  0x00;
return 0;
}
//Last stage is coming towart CCW
else if((ENCODE_LEFT_KNOB(spi_read) == 0x00) && left_previous_state == 0x01){
left_encoder_dir = -1;      //Ensure CCW
left_encoder_state +=1;     //Next state
left_previous_state =  0x00;
return 0;
}
else{
    //left_encoder_dir = 0;       //Reset Left encoder direction
    //left_encoder_state = 0;     //Reset Left state
    //previous_state = 0xFF; //Reset previous to default
    return 0;
}
case 3: //Almost last stae 
//Last stage is coming towart CW
if( ENCODE_LEFT_KNOB(spi_read) == left_previous_state){
    return 0;
}	
if((ENCODE_LEFT_KNOB(spi_read) == 0x01) && (left_previous_state == 0x00)){
    left_encoder_dir = 1;  //Ensure the direction is CW
    left_encoder_state += 1;  //Next state
    left_previous_state =  0x01;
    return 0;
}
//Last stage is coming towart CCW
else if((ENCODE_LEFT_KNOB(spi_read) == 0x02) && left_previous_state == 0x00){
    left_encoder_dir = -1;      //Ensure CCW
    left_encoder_state +=1;     //Next state
    left_previous_state =  0x02;
    return 0;
}
else{
    //left_encoder_dir = 0;       //Reset Left encoder direction
    //left_encoder_state = 0;     //Reset Left state
    //previous_state = 0xFF; //Reset previous to default
    return 0;
}              
return 0;

case 4:   //Almost last stae 
//Last stage is coming towart CW
if( ENCODE_LEFT_KNOB(spi_read) == left_previous_state){
    return 0;
}	
if((ENCODE_LEFT_KNOB(spi_read) == 0x03) && (left_previous_state == 0x01)){
    left_encoder_dir = 0;  //Ensure the direction is CW
    left_encoder_state = 0;  //Next state = back to 0
    left_previous_state =  0xFF;
    return 1;
}
//Last stage is coming towart CCW
else if((ENCODE_LEFT_KNOB(spi_read) == 0x03) && left_previous_state == 0x02){
    left_encoder_dir = 0;      //Ensure CCW
    left_encoder_state = 0;     //Next state = back to 0
    left_previous_state =  0xFF;
    return -1;
}
else{
    //left_encoder_dir = 0;       //Reset Left encoder direction
    //left_encoder_state = 0;     //Reset Left state
    //previous_state = 0xFF; //Reset previous to default
    return 0;
}     
return 0;
}
*/


int16_t update_number(int sign){
    return sign*dif;

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
	//value = SPI_Receive();
	//spi_read = SPI_Receive();
	decode_spi_knob();
	//value = value+ (dif * (decode_spi_knob()));
	//value = decode_spi_knob();
	//update_number();

	display_update();

    }//while
    return 0;
}//main
