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

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#define INPUT 0xFF
#define SELECT_BIT_BUTTON_BOARD 0x70

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#include "audio.c"
//#include "lcd_functions.h"
#include "LCDDriver.h"

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

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];
uint8_t alarm_change = 1;
//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];
uint8_t brightness_level;
uint8_t update_LCD = 0;
//int delay_time[10] = {50, 60, 70, 80, 90, 100, 110, 120, 130, 150};
uint16_t time = 0;
char *str;
uint16_t alarm_time = 0;
uint16_t show_time = 0;
uint8_t am_pm = 0;
uint8_t show_ampm;
uint16_t ampm_time;
uint8_t snooze_offset = 0;
static uint8_t mode = 0;
static uint8_t sw_table[] = {0, 1, 2, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0, 2, 1, 0};
//static uint32_t counter = 0;
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
uint8_t music_status = 0;
static uint8_t ticker = 0;
uint8_t volume = 0;
uint8_t blink = 0;
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
    uint8_t button = 0;
    static int previous_mode;   
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
		case 0:  
		    mode = 0;
		    break;
		case 1:
		    if(mode == 1){
			mode = 0;	
		    }
		    else{
			mode = 1;
		    }
		    break;
		case 2:
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
		    break;
		case 5:
		    show_ampm = !show_ampm;
		    break;
		case 6:
		    if(music_status){
			snooze_flag = 1;
			music_status = 0;
			music_off();
		    }
		    break;
		case 7:
		    if(alarm_on && music_status){
			music_off();
			music_status = 0;
		    }
		    alarm_on = !alarm_on;
		    update_LCD = 1;
		    //alarm_change = 1; 
		    break;
		default:
		    break;
	    }
	    bar_graph();
	    /*
	       if(previous_mode == button && button != 7){
	       mode = 0;
	       previous_mode = 0;
	       bar_graph();
	       return;
	       }
	       else if(button == 6 && music_status){
	       snooze_flag = 1;
	       music_status = 0;
	       music_off();
	       }
	       else if(button == 7){
	       if(alarm_on && music_status){
	       music_off();
	       music_status = 0;
	       }
	       alarm_on = !alarm_on;
	       alarm_change = 1;
	       }

	       else{
	       mode = button;
	       } 
	       bar_graph();
	       previous_mode = button;
	     */
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
    if(count%8 == 0){
	beat++;
	blink = !blink;
    }
    if((count%128)==0){
	ticker++;     
	second++;    
	if(alarm_on){
	    if ((alarm_time == time) && !snooze_flag && !music_status){
		//play music
		music_on();
		music_status = 1;
	    }
	    else if(snooze_flag){
		snooze_second++;
		if(snooze_second >= 10){
		    snooze_flag = 0;
		    music_on();
		    snooze_second = 0;
		}	
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
    if(count ==  255){
    }
    switch(count%8){
	case 0:
	    check_knobs();
	    break;
	case 1:
	    //display_update();
	    /* if(alarm_change){
	       if(alarm_on){
	       string2lcd("Alarm On");
	       }
	       else{
	       string2lcd("Alarm Off");
	       }
	       alarm_change = 0;
	       }*/
	    break;
	default:
	    break;
    }    
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
    static int minute_change = 0;
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
    minute_change = 1;

    alarm_time = (alarm_hour * 100) + alarm_minute;
    if(minute_change){
	time = (hour * 100) + minute;
	minute_change = 0;

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

    }
}

void SPI_init(){
    /* Set MOSI and SCK output, all others input */
    //DDRB = (1<<PB3)|(1<<PB1);

    /* Enable SPI, Master, set clock rate fck/16 */
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
    //SPSR = (1<<SPI2X);
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
    PORTE &= 0;       //Write 0 to PE6 to trigger SPI on radio board
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
	write = 0xF0;
    }
    //write &= 0xF7;
    //Write the bargraph to SPI
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
    uint8_t display_segment = 0;
    //static uint8_t rotate_7seg = 0;
    switch(mode){
	case 2:
	    segsum(alarm_time);
	    segment_data[2] = 0x00;
	    break;
	case 3:
	    segsum(volume);
	    segment_data[2] = 0xFF; //decimal
	    break;
	case 4:
	    break;
	default:
	    segsum(show_time);
	    break;
    }

    for(display_segment = 0 ; display_segment < 5 ; display_segment++){
	PORTB = display_segment << 4;
	PORTA = segment_data[display_segment];
	_delay_us(40);
	PORTA = OFF;
    }

    /*
       if(rotate_7seg > 4){
       rotate_7seg = 0;
       }
       PORTB &= 0x8F;
       PORTB |= rotate_7seg << 4;
       PORTA = segment_data[rotate_7seg];	
       rotate_7seg++;
     */
    //_delay_us(40);
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
	case 3:
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
	case 3:
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
	default:
	    break;
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
    DDRE |= (1<<PE1);
    TCCR3A  = (1<<WGM30) | (1<<COM3A1);
    TCCR3B = (1<<WGM32) | (1<<CS30);
    OCR3A = volume;
}


int main()
{
    //set port bits 4-7 B as outputs
    //uint8_t c = 0;
    DDRE = 0xc0;
    PORTE &= 0x7F;
    DDRB = 0xF7;
    DDRD |= (1 << PB2);

    volume = 100;
    timer_init();
    ADC_init();
    music_init();   
    SPI_init();
    LCD_SPIInit();
    LCD_Init();
    volume_control_init();
    // lcd_init();
    // clear_display();
    // string2lcd("hello");
    sei();
    while(1){
	display_update();
	update_time();
	if(update_LCD){
		LCD_Clr();
		if(alarm_on){
               		LCD_PutStr("ALARM ON!!");
		}
		else{
               		LCD_PutStr("ALARM OFF!!");
		}
		update_LCD = 0;
	}
	//minute++;
	//Alarm mode is on
    }
    return 0;
}
