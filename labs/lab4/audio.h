#define MUTE   0x04
#define UNMUTE 0xFB

void sound_init(void){
    DDRC   |= (1<<PC2);
    TCCR1A = 0x00;
    TCCR1B |= (1<<WGM12);
    TCCR1C = 0x00;	

    OCR1A = 0x0031;

}

void sound_off(void){
   // notes = 0;
    TCCR1B &= ~((1<<CS11)|(1<<CS10));
    PORTC &= MUTE;
}

void sound_in(void){
    TCCR1B |= (1<<CS11)|(1<<CS10);
    PORTC  &= UNMUTE;
}

