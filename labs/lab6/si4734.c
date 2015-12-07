//Si4734 i2C functions     
//Roger Traylor 11.13.2011
//device driver for the si4734 chip.
//TODO: unify the properties if possible between modes
//TODO: unify power up commands...all 0x01?, think so, power ups look the same
//TODO: document how this is now running with interrupt mode
//      and not blind timing. 

// header files
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <util/twi.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "uart_functions.h"

#include "twi_master.h" //my defines for TWCR_START, STOP, RACK, RNACK, SEND
#include "si4734.h"

uint8_t si4734_wr_buf[9];          //buffer for holding data to send to the si4734 
uint8_t si4734_rd_buf[15];         //buffer for holding data recieved from the si4734
uint8_t si4734_tune_status_buf[8]; //buffer for holding tune_status data  
uint8_t si4734_revision_buf[16];   //buffer for holding revision  data  

enum radio_band{FM, AM, SW};
extern volatile enum radio_band current_radio_band;

extern volatile uint8_t STC_interrupt;  //flag bit to indicate tune or seek is done

//extern uint16_t eeprom_fm_freq;
//extern uint8_t  eeprom_volume;

extern uint16_t current_fm_freq;
extern uint8_t  current_volume;

//Used in debug mode for UART1
//******************************************************************


//********************************************************************************
//                            get_int_status()
//
//Fetch the interrupt status available from the status byte.
//
//TODO: update for interrupts
// 
uint8_t get_int_status(){

    si4734_wr_buf[0] = GET_INT_STATUS;              
    twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 1); //send get_int_status command
    while( twi_busy() ){}; //spin while previous TWI transaction finshes
    _delay_us(300);        //si4734 process delay
    twi_start_rd(SI4734_ADDRESS, si4734_rd_buf, 1); //get the interrupt status 
    while( twi_busy() ){}; //spin while previous TWI transaction finshes
    return(si4734_rd_buf[0]);
}
//********************************************************************************

//********************************************************************************
//                            fm_tune_freq()
//
//takes current_fm_freq and sends it to the radio chip
//

void fm_tune_freq(){
  si4734_wr_buf[0] = 0x20;  //fm tune command
  si4734_wr_buf[1] = 0x00;  //no FREEZE and no FAST tune
  current_fm_freq = 10630;
  si4734_wr_buf[2] = (uint8_t)(current_fm_freq >> 8); //freq high byte
  si4734_wr_buf[3] = (uint8_t)(current_fm_freq);      //freq low byte
  si4734_wr_buf[4] = 0x00;  //antenna tuning capactior
  //send fm tune command
  STC_interrupt = FALSE;
 // set_property(GPO_IEN, GPO_IEN_STCIEN); //seek_tune complete interrupt
  twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 5);
 // while(!STC_interrupt ){}; //spin until the tune command finishes 
}
//********************************************************************************

//********************************************************************************
//                            fm_pwr_up()
//
void fm_pwr_up(){
	//restore the previous fm frequency  
	//current_fm_freq = eeprom_read_word(&eeprom_fm_freq); //TODO: only this one does not work 
	//current_volume  = eeprom_read_byte(&eeprom_volume); //TODO: only this one does not work 

	//send fm power up command
	si4734_wr_buf[0] = FM_PWR_UP; //powerup command byte
	si4734_wr_buf[1] = 0x50;      //GPO2O enabled, STCINT enabled, use ext. 32khz osc.
	si4734_wr_buf[2] = 0x05;      //OPMODE = 0x05; analog audio output
	twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 3);
	_delay_ms(120);               //startup delay as specified 
	//The seek/tune interrupt is enabled here. If the STCINT bit is set, a 1.5us
	//low pulse will be output from GPIO2/INT when tune or seek is completed.
	set_property(GPO_IEN, GPO_IEN_STCIEN); //seek_tune complete interrupt
}
//********************************************************************************

//********************************************************************************
//                            radio_pwr_dwn()
//

void radio_pwr_dwn(){

	//save current frequency to EEPROM
	switch(current_radio_band){
		//case(FM) : eeprom_write_word(&eeprom_fm_freq, current_fm_freq); break;
		default  : break;
	}//switch      

	//eeprom_write_byte(&eeprom_volume, current_volume); //save current volume level

	//send fm power down command
	si4734_wr_buf[0] = 0x11;
	twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 1);
	_delay_us(310); //power down delay
}
//********************************************************************************

//********************************************************************************
//                            fm_rsq_status()
//
//Get the status on the receive signal quality. This command returns signal strength 
//(RSSI), signal to noise ratio (SNR), and other info. This function sets the
//FM_RSQ_STATUS_IN_INTACK bit so it clears RSQINT and some other interrupt flags
//inside the chip. 
//TODO: Dang, thats a big delay, could cause problems, best check out.
//
void fm_rsq_status(){

	si4734_wr_buf[0] = FM_RSQ_STATUS;            //fm_rsq_status command
	si4734_wr_buf[1] = FM_RSQ_STATUS_IN_INTACK;  //clear STCINT bit if set
	twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 2);
	while(twi_busy()){}; //spin while previous TWI transaction finshes
	_delay_us(300);      //delay for si4734 to process
	//This is a blind wait. Waiting for CTS interrupt here would tell you 
	//when the command is received and has been processed.
	//get the fm tune status 
	twi_start_rd(SI4734_ADDRESS, si4734_tune_status_buf, 8);
	while(twi_busy()){}; //spin while previous TWI transaction finshes
}


//********************************************************************************
//                            fm_tune_status()
//
//Get the status following a fm_tune_freq command. Returns the current frequency,
//RSSI, SNR, multipath and antenna capacitance value. The STCINT interrupt bit
//is cleared.
//TODO: Dang, thats a big delay, could cause problems, best check out.
//
void fm_tune_status(){

	si4734_wr_buf[0] = FM_TUNE_STATUS;            //fm_tune_status command
	si4734_wr_buf[1] = FM_TUNE_STATUS_IN_INTACK;  //clear STCINT bit if set
	twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 2);
	while(twi_busy()){}; //spin while previous TWI transaction finshes
	_delay_us(300);        //delay for si4734 to process
	//get the fm tune status 
	twi_start_rd(SI4734_ADDRESS, si4734_tune_status_buf, 8);
	while( twi_busy() ){}; //spin till TWI read transaction finshes
}

//********************************************************************************
//                            set_property()
//
//The set property command does not have a indication that it has completed. This
//command is guarnteed by design to finish in 10ms. 
//
void set_property(uint16_t property, uint16_t property_value){

	si4734_wr_buf[0] = SET_PROPERTY;                   //set property command
	si4734_wr_buf[1] = 0x00;                           //all zeros
	si4734_wr_buf[2] = (uint8_t)(property >> 8);       //property high byte
	si4734_wr_buf[3] = (uint8_t)(property);            //property low byte
	si4734_wr_buf[4] = (uint8_t)(property_value >> 8); //property value high byte
	si4734_wr_buf[5] = (uint8_t)(property_value);      //property value low byte
	twi_start_wr(SI4734_ADDRESS, si4734_wr_buf, 6);
	_delay_ms(10);  //SET_PROPERTY command takes 10ms to complete
}//set_property()


