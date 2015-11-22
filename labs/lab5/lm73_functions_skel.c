// lm73_functions.c       
// Roger Traylor 11.28.10

#include <util/twi.h>
#include "lm73_functions_skel.h"
#include <util/delay.h>

//TODO: initalize with more resolution and disable the smb bus timeout
//TODO: write functions to change resolution, alarm etc.

uint8_t lm73_wr_buf[2];
uint8_t lm73_rd_buf[2];

//********************************************************************************

//******************************************************************************
uint16_t lm73_temp_convert(char temp_digits[], uint16_t lm73_temp, uint8_t f_not_c){
    //given a temperature reading from an LM73, the address of a buffer
    //array, and a format (deg F or C) it formats the temperature into ascii in 
    //the buffer pointed to by the arguement.
    lm73_temp = (lm73_temp >> 7);
    temp_digits = 'C';
    if(f_not_c){
	lm73_temp = (lm73_temp*5/9)+32;
	temp_digits = 'F';
    }
    return lm73_temp;
    //Yeah, this is for you to do! ;^)

}//lm73_temp_convert
//******************************************************************************


uint8_t 

