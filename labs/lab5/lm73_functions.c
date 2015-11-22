// lm73_functions.c       
// Roger Traylor 11.28.10

#include <util/twi.h>
#include "lm73_functions.h"
#include <util/delay.h>

//TODO: initalize with more resolution and disable the smb bus timeout
//TODO: write functions to change resolution, alarm etc.

uint8_t lm73_wr_buf[2];
uint8_t lm73_rd_buf[2];

//********************************************************************************

//******************************************************************************
uint16_t lm73_temp_convert(uint16_t lm73_temp, uint8_t f_not_c){
    //given a temperature reading from an LM73, the address of a buffer
    //array, and a format (deg F or C) it formats the temperature into ascii in 
    //the buffer pointed to by the arguement.
    lm73_temp = (lm73_temp >> 7);
    //temp_digits = 'C';
    //When f_not_c is 1 -> send F
    //when f_not_c is 2 -> send C
    if(f_not_c == 1){
	lm73_temp = (lm73_temp*5/9)+32;
	//temp_digits = 'F';
    }
    return lm73_temp;
    //Yeah, this is for you to do! ;^)

}//lm73_temp_convert
//******************************************************************************
void lm73_init(){
	lm73_wr_buf[0] = LM73_PTR_TEMP;
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 2);

}

uint16_t get_local_temp(uint8_t f_not_c){
	uint16_t ret, lm73_temp;
	twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2);
	_delay_ms(2);    //wait for it to finish
	//now assemble the two bytes read back into one 16-bit value
	//save high temperature byte into lm73_temp
	lm73_temp = lm73_rd_buf[0] << 8;
	//shift it into upper byte 
	//"OR" in the low temp byte to lm73_temp 
	lm73_temp |= lm73_rd_buf[1];
	//convert to string in array with itoa() from avr-libc                           
	lm73_temp = lm73_temp >> 7;
	ret = lm73_temp_convert(lm73_temp, f_not_c);

	return ret;
}
