// thermo3_skel.c
// R. Traylor
// 11.15.2011 (revised 11.18.2013)

//Demonstrates basic functionality of the LM73 temperature sensor
//Uses the mega128 board and interrupt driven TWI.
//Display is the raw binary output from the LM73.
//PD0 is SCL, PD1 is SDA. 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "lm73_functions_skel.h"
#include "twi_master.h"
#include "uart_functions.h"
#include "lcd_functions.h"
uint8_t i;                     //general purpose index

//delclare the 2 byte TWI read and write buffers (lm73_functions_skel.c)
extern uint8_t lm73_rd_buf[2]; 
extern uint8_t lm73_wr_buf[2];

//********************************************************************
//                            spi_init                               
//Initalizes the SPI port on the mega128. Does not do any further    
// external device specific initalizations.                          
//********************************************************************
void spi_init(void){
    DDRB |=  0x07;  //Turn on SS, MOSI, SCLK
    //mstr mode, sck=clk/2, cycle 1/2 phase, low polarity, MSB 1st, 
    //no interrupts, enable SPI, clk low initially, rising edge sample
    SPCR=(1<<SPE) | (1<<MSTR); 
    SPSR=(1<<SPI2X); //SPI at 2x speed (8 MHz)  
}//spi_init

/***********************************************************************/
/*                                main                                 */
/***********************************************************************/
int main ()
{     
    uint16_t lm73_temp;  //a place to assemble the temperature from the lm73
    char str[2];
    uint8_t lo, hi;
	uint16_t tmp;
    uint8_t temp_mode;
    //uint8_t send_buff_low, send_buff_high;  //Buffer for sending
    spi_init();//initalize SPI 
    init_twi();//initalize TWI (twi_master.h)  
    uart_init();
    lcd_init();
    //set LM73 mode for reading temperature by loading pointer register

    //this is done outside of the normal interrupt mode of operation 

    //load lm73_wr_buf[0] with temperature pointer address
    lm73_wr_buf[0] = LM73_PTR_TEMP;
    //start the TWI write process (twi_start_wr())
    twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2); 
    sei();             //enable interrupts to allow start_wr to finish
    //string2lcd("hello");
    cursor_off();
    while(1){          //main while loop
	_delay_ms(1000);  //tenth second wait
	//clear_display();
	//Keep reading UART until gets command to get temperature
	// 0-do nothing
	// 1-Send celcius
	// 2-Send Farenheigh

        clear_display();
	uart_putc(2);
	//str[0] = uart_getc();
	//str[1] = uart_getc();
        lo = uart_getc();
	hi = uart_getc();
        tmp = (hi<<8) |lo;
	itoa(tmp, str, 10);
		//str[1] = '1';
	//str[0] = '2';
	string2lcd(str);
	//rad temperature data from LM73 (2 bytes)  (twi_start_rd())
	//twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2);
	//_delay_ms(2);    //wait for it to finish
	//now assemble the two bytes read back into one 16-bit value
	//save high temperature byte into lm73_temp
	//lm73_temp = lm73_rd_buf[0] << 8;
	//shift it into upper byte 
	//"OR" in the low temp byte to lm73_temp 
	//lm73_temp |= lm73_rd_buf[1];
	//convert to string in array with itoa() from avr-libc                           
	//lm73_temp = lm73_temp >> 7;
	//lm73_temp = lm73_temp_convert(lm73_temp, temp_mode);

	//itoa(lm73_temp, str, 10);

	//uart_putc(str[0]);
	//uart_putc(str[1]);


	//send the string to LCD (lcd_functions)
    } //while
} //main
