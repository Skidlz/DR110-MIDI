/*
 * main.c
 *
 *  Created on: Dec 13, 2014
 *      Author: Owner
 */

// MIDI to trigger converter, customized for the DR 110
// triggers are inverted and open collector

#define F_CPU 16000000UL
#include <stdint.h>
#include <stdlib.h>
#define byte uint8_t
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "uart.h"
#include "midi.h"
//#include <avr/iom328p.h>

//cy, oh, ch, bd, sn, cpII, cpI, ac
#define tr_cnt 10
byte trig_status_L = 0;
byte trig_status_H = 0;
/*
byte clap_stage = 0;// 0= off, 1=init trig, 2=trig2, 3=trig3, 4=trig4
#define cp_dly_1	10
#define cp_dly_2	1
*/

byte clap_vcs = 5;
byte clap_vcs_II = 8;

//byte clap_wait = 0;

struct VOICE{
	byte note;
	volatile uint8_t *port;
	byte bit;
	byte status;
	byte timer;
}VOICE;

int vcs_cnt = 10;
struct VOICE vcs[10];// list of voice
/*
 * 	vcs = {// list of voice
		{35, PORTD, 1, 0, 0}, //kick
		{36, PORTD, 2, 0, 0}, //snare
		{37, PORTD, 2, 0, 0}, //o-hat
		{38, PORTD, 2, 0, 0}, //c-hat
		{39, PORTD, 2, 0, 0}, //cymbal
		{40, PORTD, 2, 0, 0}, //clap
		{41, PORTD, 2, 0, 0}, //accent
		{42, PORTD, 2, 0, 0}, //trig out
		{43, PORTD, 2, 0, 0}, //cpI
		{44, PORTD, 2, 0, 0}, //cpII
	};
 *
 */

const byte acnt_lvl = 90; //velocity for accent to turn on
#define acnt_pin 2 //pin to activate accent on PORTD
#define trig_len 9 //tigger length in 1/10ms
#define CTC_MATCH_OVERFLOW ((F_CPU / 10000) / 8) //10khz
ISR (TIMER1_COMPA_vect) {
	for (int i = 0; i < tr_cnt; i++) if (vcs[i].status) vcs[i].timer++; //inc counts
	PORTB = trig_status_L;
	PORTD = trig_status_H;
}

void key_press(byte note, byte vel) {
	//set accent
	//PORTD = (vel >= acnt_lvl) ? (PORTD | (1<<acnt_pin)): (PORTD & ~(1<<acnt_pin));
	if ( vel >= acnt_lvl) key_press(41, 0);//prevent recursive calls to accent
	// note, port, bit, status, timer
	for (int i = 0; i < vcs_cnt; i++){
		if (vcs[i].note == note){
			*vcs[i].port |= (1 << vcs[i].bit); // turn on trigger
			vcs[i].status = (i == clap_vcs) ? 2: 1; // clap status = 2
			//vcs[i].status = 1; // playing
			vcs[i].timer = 0; // reset count
			if (vcs[i].port == &PORTB){
				trig_status_L |= (1 << vcs[i].bit);
			} else if (vcs[i].port == &PORTD){
				trig_status_H |= (1 << vcs[i].bit);
			}
			break;
		}
	}
}

void poll_trig(){
	for (int i = 0; i < vcs_cnt; i++){
		if (vcs[i].timer >= trig_len ){
			//turn off pin
			if (vcs[i].port == &PORTB) trig_status_L &= ~(1 << vcs[i].bit);
			else if (vcs[i].port == &PORTD) trig_status_H &= ~(1 << vcs[i].bit);

			switch (vcs[i].status){
			case 2: // clap I 1 end
			case 4: // clap I 2 end
			case 6: // clap I 3 end
				vcs[i].timer = 0; // reset count
				vcs[i].status++;
				break;
			case 3: // clap I wait 1
			case 5: // clap I wait 2
				if(vcs[i].timer >= 100){ //10ms delay
					vcs[i].timer = 0;
					vcs[i].status++;
					//turn back on
					if (vcs[i].port == &PORTB) trig_status_L |= (1 << vcs[i].bit);
					else if (vcs[i].port == &PORTD) trig_status_H |= (1 << vcs[i].bit);
				}
				break;
			case 7: // clap I wait 3
				if(vcs[i].timer >= 100){ //10ms delay
					vcs[i].timer = 0;
					vcs[i].status++;
				}
				break;
			case 8:
				key_press(vcs[clap_vcs_II].note, 0); // start tail
			case 1:
				vcs[i].status = 0;
				vcs[i].timer = 0; // reset count
			}

			break;
		}
	}
}

void midi_control(byte cc, byte val){
	switch(cc){
	case 7://volume
		//PORTD &= ~0b11111000;
		//PORTD |= (val<<1) & 0b11111000;
		break;
	}
}



int main() {

	memcpy(vcs, (struct VOICE[]) {
		{35, &PORTB, 1, 0, 0}, //kick
		{36, &PORTB, 2, 0, 0}, //snare
		{37, &PORTD, 7, 0, 0}, //o-hat
		{38, &PORTB, 0, 0, 0}, //c-hat
		{39, &PORTD, 6, 0, 0}, //cymbal
		{40, &PORTB, 4, 0, 0}, //clap ???
		{41, &PORTB, 5, 0, 0}, //accent
		{42, &PORTD, 5, 0, 0}, //trig out
		{43, &PORTB, 3, 0, 0}, //cpII
		{44, &PORTB, 4, 0, 0}, //cpI
	}, 10 * sizeof *vcs);

	uart_init(31250);

	set_MIDI_key_press(key_press);
	//todo button to learn chan
	//todo store chan in eprom
	set_cntrl_chng(midi_control);

	UCSR0B &= ~(1 << TXEN0); //Disable transmitter

	cli();// disable all interrupts

	TCCR1B |= (1 << WGM12) | (1 << CS11); // CTC mode, Clock/8
	OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	OCR1AL = CTC_MATCH_OVERFLOW;
	TIMSK1 |= (1 << OCIE1A); // Enable the compare match interrupt

	sei();// enable all interrupts

	DDRB = 0xff;
	DDRC = 0xff;
	DDRD = 0b11111110; //0=input
	PORTB = 0;//BD,SD,CH,OH,LT,HT
	PORTC = 0;//RS,CY
	PORTD = 0b11111100;//VOL4-0, ACC, MIDIIO

	//-------------------------------

	while (1){
		poll_trig();
		if (uart_test()) handle_midi();
	}
	return 1;
}

