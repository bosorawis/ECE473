// Sorawis Nilparuk
// 10/29/2015

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base and OE_N of BAR_GRAPH
//  PORTB bit 3 goes to OE_n of BAR_GRAPH
//  PORTB bit 2 goes to SDIN of BAR_GRAPH
//  PORTB bit 1 goes to SRCLK of BAR_GRAPH and SCK of Encoder
//  PORTD bit 2 goes to REGCLK of BAR_GRAPH
//  PORTE bit 6 goes to SHIFT_LD_N on Encoder
//  PORTE bit 7 goes to CLK_INT on Encoder  (always driven low)

#define F_CPU 16000000UL // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#define INPUT 0xFF
#define SELECT_BIT_BUTTON_BOARD 0x70

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "audio.c"
//#include "LCDDriver.h"

#include "lcd_functions.h"
#include "uart_functions.h"
#include "twi_master.h"
#include "lm73_functions.h"
#include "si4734.h"
#include "radio.h"

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
#define BUTTON_COUNT 8
#define MAX_SUM 1023
#define ENCODE_LEFT_KNOB(read)   (read & 0x0C) >> 2
#define ENCODE_RIGHT_KNOB(read)  (read & 0x03) 
#define CHECK_LEFT_KNOB   0x03
#define CHECK_RIGHT_KNOB  0x0c
#define FARENHEIT 1
#define CELCIUS 2

#define SECOND_MULTIPLIER 50000

#define CW  1
#define CCW 2                 

void left_dec();
void left_inc();
void right_inc();
void right_dec();
void decode_spi_left_knob(uint8_t encoder1);
void decode_spi_right_knob(uint8_t encoder2);
void bar_graph();
void check_knobs();

void display_update();



//holds data to be sent to the segments. logic zero turns segment on
static char *loc_temp_str; //Hold location string
static char *rem_temp_str; //Hold remote string
static char *alrm_str;	   //Hold Alarm string
static char *alrm_snooze;  //Hold Alarm snooze
uint8_t clear_LCD;		
uint8_t segment_data[5];
uint8_t encode_flag = 0;
uint8_t alarm_change = 1;
uint8_t get_temp = 0;
//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];
uint8_t brightness_level;
uint8_t update_LCD = 0;
uint8_t reset_temp = 0;
uint8_t bar_graph_flag = 0;
uint8_t temp_is_up = 0;
uint8_t radio = 0;
uint16_t radio_preset[5] = {9910, 8870, 10630, 10790, 10470};
uint8_t preset_rotate = 0;
//int delay_time[10] = {50, 60, 70, 80, 90, 100, 110, 120, 130, 150};
uint16_t time = 0;
char *str;
uint16_t alarm_time = 0;
uint16_t show_time = 0;
uint8_t show_temp = 0;
uint8_t am_pm = 0;
uint8_t show_ampm;
uint16_t ampm_time;
static uint8_t mode = 0;
static uint8_t sw_table[] = {0, 1, 2, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0, 2, 1, 0};
static uint8_t second = 0;
static uint8_t minute = 0;
static uint8_t hour = 0;
int volume_change = 0;
int8_t pressed_button = -1;
uint8_t snooze_init_second = 0;
uint8_t snooze_second = 0;
uint8_t snooze_flag = 0;
static uint8_t alarm_minute = 0;
static uint8_t alarm_hour = 0;
uint8_t alarm_on = 0;
uint8_t playing = 0;
static uint8_t ticker = 0;
uint8_t volume = 100;
uint8_t blink = 0;
uint8_t temp_mode = 0;
uint8_t LCD_mode=0;
uint8_t alarm_mode_change = 0 ;
uint8_t alarm_go = 0;
uint8_t alarm_stop = 0;
uint8_t turn_radio_on = 0;
uint8_t radio_is_on = 0;
uint8_t radio_tuning = 0;
uint8_t radio_change_time;
uint8_t display_radio=0;
enum radio_band{FM,AM,SW}; 
uint8_t snooze_offset = 1;
uint8_t snooze_offset_is_ten = 0;
volatile enum radio_band current_radio_band = FM;
volatile uint8_t STC_interrupt;  //flag bit to indicate tune or seek is done
uint8_t alarm_is_radio;
uint16_t eeprom_fm_freq;
uint8_t  eeprom_volume;

uint16_t current_fm_freq;
uint16_t new_fm_freq;
uint8_t  current_volume;

extern uint8_t lm73_rd_buf[2];
extern uint8_t lm73_wr_buf[2];

//*****************************************************************************
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
//***********************************************************************************
// int2seg
// return the 7-segment code for each digit
//***********************************************************************************
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
//***********************************************************************************
//*******************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|
//***********************************************************************************

void segsum(uint16_t sum) {
	//determine how many digits there are 
	//int digit;
	// Break down the digits

	if(ticker%2 == 1){
		segment_data[2] = 0xFC;
	}
	else{
		segment_data[2] = 0xFF;
	} 
	if(display_radio){
		sum = sum/10;
	}
	//When setting alarm is on)
	//break up decimal sum into 4 digit-segments
	segment_data[0] = int2seg(sum % 10); //ones
	segment_data[1] = int2seg((sum % 100)/10); //tens
	//segment_data[2] = 1; //decimal
	segment_data[3] = int2seg((sum % 1000)/100); //hundreds
	segment_data[4] = int2seg(sum/1000); //thousands
	//blank out leading zero digits 
	//now move data to right place for misplaced colon position
	if(am_pm && show_ampm){
		segment_data[0] &= 0x7F;
	}
	else{
		segment_data[0] |= 0b10000000;
	}
	if(display_radio){
		segment_data[1] &= 0x7F;	
	}
	else{
		if(mode != 2){
			segment_data[1] |= (1<<7);
		}
	}
	if(mode == 1 && blink){
		segment_data[4] = 0xFF;
		segment_data[3] = 0xFF;
		segment_data[1] = 0xFF;
		segment_data[0] = 0xFF;
	}
}//segment_sum
//***********************************************************************************
void button_routine(){
	// L -> R
	// 3 2 1 0 7 6 5 4
	static uint8_t rotate_LCD_mode = 1;
	uint8_t button = 0;
	//static int previous_mode;   
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
			//Check the state of buttons
			switch(button){
				case 0:  //Play radio 
					if(mode == 5){
						mode = 0;
						radio = 0;
					}
					else{
						mode = 5;
						radio = 1;
					}
					break;
				case 1: //Edit time
					if(mode == 1){
						mode = 0;	
					}
					else{
						mode = 1;
					}
					break;
				case 2: //Edit alarm
					if(mode == 2){
						mode = 0;	
					}
					else{
						mode = 2;
					}
					break;
				case 3:
					if(mode == 3){
						mode = 0;			
					}
					else{
						mode = 3;
					}
					break;
				case 4:
					//temp_mode = !temp_mode;
					rotate_LCD_mode++;
					clear_LCD = 1;
					LCD_mode = rotate_LCD_mode%3;
					if(!LCD_mode){
						mode = 4;
					}
					else{
						mode = 0;
					}
					break;
				case 5:
					if(snooze_offset == 10){
						snooze_offset = 1;
					}
					else{
						snooze_offset = 10;
					}
					break;
				case 6:
					if(playing){
						alarm_stop = 1;
						alarm_minute += snooze_offset;
					}
					break;
				case 7:
					if(playing){
						alarm_stop = 1;
					}
					alarm_on = !alarm_on;
					//alarm_change = 1; 
					break;
				default:
					break;
			}
			bar_graph_flag = 1;	
		}
	}
	DDRA = 0xFF;  //switch PORTA to output
	__asm__ __volatile__ ("nop"); //Buffer
	__asm__ __volatile__ ("nop"); //Buffer 

}
/***************************************************************************
  Interrupt routine: set flag for checking button in main
  (Might cause an issue if button check takes too long to run, it will become
  polling instead of interrupt. Tried putting button routine in the ISR,
  LED dims
 ****************************************************************************/
ISR(TIMER0_OVF_vect){
	static uint8_t count = 0;
	count++;
	//update_time();
	if(count%2 == 0){
		update_LCD = 1;

	}
	if(count%8 == 0){
		//	update_LCD = 1;
		beat++;
		blink = !blink;
	}
	if((count%128)==0){
		ticker++;     
		second++; 
		reset_temp = 1;   
		if(alarm_on){
			if (alarm_time == time){
				alarm_go = 1;
			}
			//else if(snooze_flag){
			//alarm_go = 0;
			//snooze_second++;
			//if(snooze_second >= 10){
			//	snooze_flag = 0;
			//music_on();
			//	alarm_go = 1;
			//	snooze_second = 0;
			//}	
			//}
			else if(alarm_time != time){
				alarm_stop = 1;
			}
		}
	}
}

ISR(TIMER1_COMPA_vect){
	PORTD ^= ALARM_PIN;      //flips the bit, creating a tone
	PORTB |= (1<<PB0);
	if(beat >= max_beat) {   //if we've played the note long enough
		notes++;               //move on to the next note
		play_song(song, notes);//and play it
	}
}

ISR(TIMER2_OVF_vect){
	static uint8_t count = 0;
	count++;
	//display_update();

	if(count%64 == 0){
		button_routine();
	}

	switch(count%8){
		case 0:
			encode_flag = 1;
			//check_knobs();
			break;
		case 1:
			display_update();
			break;
		default:
			break;
	}    
} 

ISR(INT7_vect){
	STC_interrupt = TRUE;
	//minute++;
	//time = 1919;
}

ISR(ADC_vect){

	if(ADCH < 100){
		OCR2 = 100-ADCH;
	}  
	else{
		OCR2 = 1;// brightness_level;
	}
}
/***************************************************************************
  Initialize SPI 
 ****************************************************************************/
void update_time(void){
	// static int minute_change = 0;
	if (second >= 60){
		minute++;
		second = 0;
	}             
	if(minute >=60){
		hour++;
		minute = 0;
	}
	if(hour >= 24){
		hour = 0;
	} 

	alarm_time = (alarm_hour * 100) + alarm_minute;
	// if(minute_change){
	time = (hour * 100) + minute;
	// minute_change = 0;

	if(show_ampm){
		if(hour>=12){
			if(hour == 12){
				show_time = 1200 + minute;
			}
			else{
				show_time = (hour-12)*100 + minute;
				am_pm = 1;
			}
		}
		else{           
			if(hour == 0){
				show_time = 1200 + minute;
			}
			else{
				show_time = (hour)*100 + minute;
			}
			am_pm = 0;
		}
	}
	else{
		show_time = (hour * 100) + minute;
	}

	//}
}

void SPI_init(){
	/* Set MOSI and SCK output, all others input */
	//DDRB = (1<<PB3)|(1<<PB1);

	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR);
	//SPSR = 0x01;
}

/***************************************************************************
  Transmit data to SPI
 ****************************************************************************/
void SPI_Transmit(uint8_t data){
	SPDR = data;    //Write data to SPDR
	while(!(SPSR & (1<<SPIF))){} //SPIN write
}

/***************************************************************************
  Read data from SPI input (SPDR)
 ****************************************************************************/
uint8_t SPI_Receive(void){
	PORTE &= 0xBF;       //Write 0 to PE6 to trigger SPI on radio board
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	// Wait until 8 clock cycles are done 
	SPDR = 0x00;     //Write 1 to set the SPI slave input to one (wait for read)
	PORTE |= (1 << PE6);  
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	while (bit_is_clear(SPSR,SPIF)){} //SPIN read 
	// Return incoming data from SPDR
	return(SPDR);  
}
void check_knobs(void){
	static uint8_t cnt = 0;
	static uint8_t encoder;
	cnt++;
	encoder = SPI_Receive();
	//TIFR |= (1<<TOV2);
	if(cnt%2==0){
		decode_spi_left_knob(encoder);
	}
	else{
		decode_spi_right_knob(encoder);
	}
}
/***************************************************************************
 *void bar_graph()
 *show selected modes on the bar graph
 **************************************************************************/
void bar_graph(){
	uint8_t write = 0;
	if(mode == 0){
		write = 0x00;
	}
	else{
		write = 1<<(mode-1);
	}

	if(alarm_on != 0){
		write = 0xFF;
	}

	SPI_Transmit(write);
	PORTD = (1 << PD2);  //Push data out of SPI
	__asm__ __volatile__ ("nop"); //Buffer
	__asm__ __volatile__ ("nop");  //Buffer


	PORTD = (2 << PD2);  // Push data out of SPI
	__asm__ __volatile__ ("nop");  //Buffer
	__asm__ __volatile__ ("nop");  //Buffer
}
/************************************************************************
 *Display the number (code from lab1)
 **************************************************************************/
void display_update(){
	//uint8_t display_segment = 0;
	static uint8_t rotate_7seg = 0;
	switch(mode){
		case 2:
			display_radio = 0;
			segsum(alarm_time);
			segment_data[2] = 0x00;

			break;
		case 3:
			display_radio = 0;
			segsum(OCR3A);
			segment_data[2] = 0xFF; //decimal

			break;
		case 5: 
			if(abs(radio_change_time - second) <= 3){
				segsum(current_fm_freq);
				segment_data[2] = 0xFF;
				display_radio = 1;
			}
			else{
				segsum(show_time);
				display_radio = 0;

			}
			break;
		default:
			segsum(show_time);
			break;
	}
	/*
	   for(display_segment = 0 ; display_segment < 5 ; display_segment++){
	   PORTB = display_segment << 4;
	   PORTA = segment_data[display_segment];
	   _delay_us(80);
	   PORTA = OFF;
	   }
	 */
	switch(rotate_7seg%5){
		case 0:
			PORTB = 0 << 4;
			PORTA = segment_data[0];
			break;

		case 1:
			PORTB = 1 << 4;
			PORTA = segment_data[1];
			break;
		case 2:
			PORTB = 2 << 4;
			PORTA = segment_data[2];
			break;
		case 3:
			PORTB = 3 << 4;
			PORTA = segment_data[3];
			break;
		case 4:
			PORTB = 4 << 4;
			PORTA = segment_data[4];
			break;
		default:
			break;

	}
	rotate_7seg++;

}
/**************************************************************************
 *Decode the knobs encoder using table method
 *Track the last phase and current phase
 **************************************************************************/
void decode_spi_left_knob(uint8_t encoder1){
	//Set up the table
	//Set up index
	uint8_t sw_index = 0;
	//Counter for preventing unneccessary reset    
	static uint8_t acount1 = 0;
	static uint8_t previous_encoder1 = 0; //Initialize previous    
	uint8_t direction = 0;                    //Direction variable
	encoder1 = ENCODE_LEFT_KNOB(encoder1);  //Mask the bit for decoding left know
	sw_index = (previous_encoder1 << 2) | encoder1; 
	/*shift previous to the left use it as an index Since
	  we know the pattern of the knob when it is turning
	  Use that data to compare with the table to determine
	  Which way it is turning*/
	direction = sw_table[sw_index];
	//Read out the direction from table
	if(direction == CW){  //If CW, add counter
		acount1++;
	}	
	if(direction == CCW){ //If CCW, decrement counter
		acount1--;
	}
	if(encoder1 == 3){    //encoder1 = 3 (stop spinning)
		if((acount1 > 1) && (acount1 < 10)){   //Check if the counter for CW
			left_inc();
		}
		if ((acount1 <= 0xFF) && (acount1 > 0xF0)){    //Check counter for CCW
			left_dec();
		}
		acount1 = 0;                     //Reset counter
	}
	previous_encoder1 = encoder1;
}
/*************************************************************************
  Exactly the same with decode_spi_left_knob(), only mask different bits 
 **************************************************************************/
void decode_spi_right_knob(uint8_t encoder2){
	uint8_t sw_index = 0;
	static uint8_t acount2 = 0;
	static uint8_t previous_encoder2 = 0;
	uint8_t direction = 0;
	encoder2 = ENCODE_RIGHT_KNOB(encoder2);
	sw_index = (previous_encoder2 << 2) | encoder2;
	direction = sw_table[sw_index];
	//value = modeA;
	if(direction == CW){
		acount2++;
	}	
	if(direction == CCW){
		acount2--;
	}
	if(encoder2 == 3){
		if((acount2 > 1) && (acount2 < 10)){
			right_inc();
		}
		if ((acount2 <= 0xFF) && (acount2 > 0xF0)){
			right_dec();
		}
		//update_number();
		acount2 = 0;
	}
	previous_encoder2 = encoder2;
}
//**************************************************************************

/***************************************************************************
 * Knob handle
 * increment/decrement timers depending on the selected mode
 ****************************************************************************/
void right_inc(){
	switch(mode){
		case 0: 
			break;
		case 1:
			minute++;
			if(minute >= 60){
				minute = 0;	    
			}     
			second++;
			break;
		case 2: 
			alarm_minute++;
			if(alarm_minute >= 60){
				alarm_minute = 0;	
			}
			break;
		case 4:
			snooze_offset_is_ten++;	
			break;
		case 5:
			radio_change_time = second;
			preset_rotate++;
			new_fm_freq = radio_preset[preset_rotate%5];
			break;
		default:
			break;             
	}
}
void right_dec(){
	switch(mode){
		case 0: 
			break;
		case 1:
			minute--;
			if(minute >= 240){
				minute = 59;	    
			}   
			break;
		case 2: 
			alarm_minute--;
			if(alarm_minute >= 240){
				alarm_minute = 59;	
			} 
			break;
		case 4:
			snooze_offset_is_ten--;	
			break;
		case 5:
			radio_change_time = second;
			preset_rotate--;
			new_fm_freq = radio_preset[preset_rotate%5];
			break;
		default:
			break;
	}

}
void left_inc(){
	switch(mode){
		case 0: 
			break;
		case 1:
			hour++;
			if(hour >= 24){
				hour = 0;	    
			}   
			break;
		case 2: 
			alarm_hour++;
			if(alarm_hour >= 24){
				alarm_hour = 0;
			}
			break;
		case 3:
			volume++;
			OCR3A = volume;
			break;
		case 4: 
			alarm_is_radio++;
			break;
		case 5:
			radio_change_time = second;
			new_fm_freq += 20;
			break;
		default:
			break;
	}
}
void left_dec(){
	switch(mode){
		case 0: 
			break;
		case 1:
			hour--;    
			if(hour >= 240){
				hour = 23;	    
			}   
			break;
		case 2:
			alarm_hour--;
			if(alarm_hour >= 240){
				alarm_hour = 23;
			}
			break;
		case 3:
			volume--;
			OCR3A = volume;
			break;
		case 4: 
			alarm_is_radio++;
			break;
		case 5:
			radio_change_time = second;
			new_fm_freq -= 20;
			break;
		default:
			break;
	}
}


//TOD
// Pull temp functions from temp directory
// Display temp

uint16_t get_remote_temp(uint8_t f_or_c){
	uint8_t lo, hi; //Low and Hi byte of temperature
	uint16_t tmp; //Full temperature result
	//If temp_mode != 0 ---- want celcius
	if(f_or_c == 1){
		uart_putc(CELCIUS); //Ask atmega48 for celcius
	}
	else{
		uart_putc(FARENHEIT); //Ask for F
	}

	lo = uart_getc(); //Get low byte
	hi = uart_getc(); //Get high byte

	tmp = (hi<<8) | lo; //Concatinate
	return tmp;	   //return tempearture
}

void generate_temp_str(){
	uint16_t remote_temp, local_temp;
	char local_buf[3];
	char remote_buf[3];
	if(reset_temp == 0){
		return;
	}
	//TODO

	if(temp_mode){
		remote_temp = get_remote_temp(1);
		local_temp = get_local_temp(1);
		loc_temp_str[15] = 'C';
		rem_temp_str[15] = 'C';

	}
	else {
		local_temp = get_local_temp(2);
		remote_temp = get_remote_temp(2);
		loc_temp_str[15] = 'F';
		rem_temp_str[15] = 'F';
	}
	itoa(local_temp,local_buf, 10);
	itoa(remote_temp,remote_buf, 10);

	if(local_buf[2] == '1'){
		loc_temp_str[12] = local_buf[0];
		loc_temp_str[13] = local_buf[1];
		loc_temp_str[14] = local_buf[2];
	}
	else{
		loc_temp_str[13] = local_buf[0];
		loc_temp_str[14] = local_buf[1];
	}
	if(remote_buf[2] == '1'){
		//rem_temp_str[11] = remote_buf[2];
		rem_temp_str[12] = remote_buf[0];
		rem_temp_str[13] = remote_buf[1];
		rem_temp_str[14] = remote_buf[2];
	}
	else{
		rem_temp_str[13] = remote_buf[0];
		rem_temp_str[14] = remote_buf[1];
	}
	//l_temp_str[13] = '3';
	//loc_temp_str[12] = '2';
	reset_temp = 0;
}

void generate_alarm_string(){
	if(alarm_is_radio%2){
		//strcpy(alrm_string, "ALARM RADIO");		
		alrm_str[7]  = 'R';
		alrm_str[8]  = 'A';
		alrm_str[9]  = 'D';
		alrm_str[10] = 'I';
		alrm_str[11] = 'O';
	}
	else{
		//strcpy(alrm_string, "ALARM MUSIC");		
		alrm_str[7]  = 'M';
		alrm_str[8]  = 'U';
		alrm_str[9]  = 'S';
		alrm_str[10] = 'I';
		alrm_str[11] = 'C';
	}

	if(snooze_offset == 1){
		alrm_snooze[7] = '0';
		alrm_snooze[8] = '1';
	}
	else {
		alrm_snooze[7] = '1';
		alrm_snooze[8] = '0';
	}
}

void show_temperature(){
	static uint8_t counter = 0;
	generate_temp_str();
	if(counter <= 15){
		char2lcd(loc_temp_str[counter]);
		if(counter == 15){
			home_line2();
			//_delay_ms(1);
		}
		counter++;
		//return;
	}
	else if (counter >=16 && counter <= 31){
		//minute++;
		char2lcd(rem_temp_str[counter-16]);
		counter++;
		//if(counter == 31){
		//	cursor_home();
		//}
		//return;
	}
	else if(counter >= 75){
		temp_is_up = 1;
		counter = 0;
		cursor_home();
	}
	else{
		counter++;
	}
}

void show_alarm(){
	static uint8_t counter = 0;
	generate_alarm_string();
	if(counter <= 15){
		char2lcd(alrm_str[counter]);
		if(counter == 15){
			home_line2();
		}
		counter++;
	}
	else if (counter >=16 && counter <= 31){
		char2lcd(alrm_snooze[counter-16]);
		counter++;
	}
	else if (counter >= 75){
		counter = 0;
		cursor_home();
	}
	else{
		counter++;
	}
}


//******************************************************************
/*******************************************************************
 * Alarm operation:
 * Normal mode
 *Show time
 * Press button 1-Alarm edit mode
 * Display alarm time
 *Left knob  = change hour
 *Right knob = change minutes
 * Press button 2 - Brightness control
 *Display number (1-100)  (time needs to keep running)
 *Left knob  = PWM for 7-seg
 *Right knob = PWM for bar graph
 *****************************************************************/

void timer_init(void){
	TCCR0 |= (1<<CS00) ;  //normal mode, prescale by 32
	ASSR  |= (1<<AS0);
	TCCR2 |= (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<COM20)|(0<<CS20)| (1<<CS21); //normal mode, prescale by 32
	TIMSK |= (1<<TOIE0)| (1<<TOIE2);// | (1<<OCIE2);             //enable interrupts
}

void ADC_init(void){
	DDRF |= !(1<<PF0);
	PORTF = 0x00;
	ADMUX  |= (1<<ADLAR) | (1<<REFS0);
	ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADFR) | (1<<ADIE)\
		  |(1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	OCR2 = 0xFF;
}

void volume_control_init(void){
	//DDRE |= (1<<PE3);
	TCCR3A  = (1<<WGM30) | (1<<COM3A1);
	TCCR3B = (1<<WGM32) | (1<<CS30);
	OCR3A = volume;
}

void initialize_string(){
	loc_temp_str = "Local  temp:   C";
	rem_temp_str = "Remote temp:   C";
	alrm_str     = "ALARM      *****";
	alrm_snooze  = "Snooze:   ******";
}

void encoder_init(){
	DDRF &= 0xFC;
	PORTF |= (1<<PF7);
}

void LCD_handler(){
	if(LCD_mode==0){
		//alarm_mode_change = 1;
		show_alarm();	
		//show words;
	}
	else if(LCD_mode==1){
		//Mode 2 => show temp in F
		temp_mode = 0;	
		show_temperature();
	}
	else if(LCD_mode==2){
		//Mode 3 => show temp in C
		temp_mode = 1;	
		show_temperature();
	}
}


int main()
{
	//set port bits 4-7 B as outputs
	//uint8_t c = 0;
	//DDRE = 0xFF;
	//PORTE &= 0x7F;
	DDRB = 0xF7;
	DDRD |= (1 << PB2);

	volume = 100;
	timer_init();
	ADC_init();
	//encoder_init();
	music_init();   
	SPI_init();
	lcd_init();
	init_twi();
	uart_init();
	volume_control_init();
	lm73_init();
	cursor_off();
	initialize_string();
	//Test radio
	//_delay_ms(1000);
	radio_interrupt_init();
	radio_init();
	current_fm_freq = 10630;
	new_fm_freq = current_fm_freq;
	//EIMSK |= (1<<INT7);
	//EICRB |= (1<<ISC71);
	sei();

	while(1){

		update_time();
		if(snooze_offset_is_ten%2){
			snooze_offset = 10;
		}	
		else{
			snooze_offset = 1;
		}
		if(encode_flag){
			check_knobs();
			encode_flag = 0;
		}
		if(bar_graph_flag){
			bar_graph();
			bar_graph_flag = 0;
		}
		if(update_LCD){
			//show_temperature();
			LCD_handler();
			update_LCD = 0;
		}

		if(clear_LCD){
			clear_display();
			clear_LCD = 0;
		}
		if(radio){
			if(mode != 5){
				radio_pwr_dwn();
				radio = 0;
				radio_is_on = 0;
			}
			if(!radio_is_on){	
				radio_reset();
				radio_powerUp();
				_delay_ms(1000);
				radio_tune_freq();
				//new_fm_freq = current_fm_freq;
				radio_is_on = 1;
			}

			else{
				if(current_fm_freq != new_fm_freq){
					current_fm_freq = new_fm_freq;
					radio_tune_freq();
				}
			}
		}
		else{
			if(radio_is_on){
				radio_pwr_dwn();
				radio = 0;
				radio_is_on = 0;
			}
		}

		if(alarm_go){
			alarm_go = 0;
			if(!playing){
				playing = 1;
				if(alarm_is_radio){
					radio_reset();
					radio_powerUp();
					_delay_ms(1000);
					radio_tune_freq();
					//fm_tune_status();
					turn_radio_on = 0;
				}
				else{
					music_on();
				}
			}
			else{

			}
		}
		if(alarm_stop){
			alarm_stop = 0;
			if(playing){
				playing = 0;
				if(alarm_is_radio){
					radio_pwr_dwn();
				}
				else{
					music_off();
				}
			}

		}
	}
	return 0;
}
