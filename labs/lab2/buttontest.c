// testled1.c
// R. Traylor
// 10.3.05
// Tests wiring of LED board to mega128.
// Select a digit with PORTB upper nibble then with
// port D push buttons illuminate a single segment.

// Port mapping:
// Port A:  bit0 brown  segment A
//          bit1 red    segment B
//          bit2 orange segment C
//          bit3 yellow segment D
//          bit4 green  segment E
//          bit5 blue   segment F
//          bit6 purple segment G
//          bit7 grey   decimal point
//               black  Vdd
//               white  Vss

// Port B:  bit4 green  seg0
//          bit5 blue   seg1
//          bit6 purple seg2
//          bit7 grey   pwm 
//               black  Vdd
//               white  Vss

#include <avr/io.h>

int8_t debounce_switch(){
    static uint16_t state = 0; //holds present state
    state = (state << 1) | (! bit_is_clear(PINA, 0)) | 0xE000;
    if (state == 0xF000) return 1;
    return 0;
}

uint8_t chk_buttons(uint8_t button){
    static uint16_t state = 0; //holds present state
    state = (state << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
    if (state == 0xF000) {
        return 1;
    }
    return 0;   
}
int main()
{
    DDRA  = 0x00;   //set port A to all Inputs
    DDRB  = 0xF0;   //set port bits 4-7 B as outputs
    DDRD  = 0x00;   //set port D all inputs 
    PORTD = 0xFF;   //set port D all pullups 
    //PORTA = 0xFF;   //set port A to all ones  (off, active low)

    while(1){
	PORTB = 0x70; // Turn button board on
	DDRA  = 0x00;  //PortA input mode
	PORTA = 0xFF;  // PORTA input mode
	__asm__ __volatile__ ("nop"); //buffer
	__asm__ __volatile__ ("nop"); //buffer
	if (chk_buttons(0)){ //If a button is pressed
	    DDRA = 0xFF; //PortA output
	    __asm__ __volatile__ ("nop");
	    __asm__ __volatile__ ("nop");
	    PORTA = 0x00; //LED on
	    PORTB = 0x00; // Selector 	
	} 
	else if (debounce_switch()){ //If a button is pressed
	    DDRA = 0xFF; //PortA output
	    __asm__ __volatile__ ("nop");
	    __asm__ __volatile__ ("nop");
	    PORTA = 0x00; //LED on
	    PORTB = 0x10; // Selector 	
	} 
    } //while
}  //main
