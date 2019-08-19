/*
 * midi.h
 *
 *  Created on: May 19, 2014
 *      Author: Owner
 */
#include <stdint.h>
#ifndef byte
#define byte uint8_t
#endif
#ifndef MIDI_H_
#define MIDI_H_
byte handle_com(byte com, byte arg_cnt, byte *args);
byte handle_glob_com(byte com, byte arg_cnt, byte *args);
byte skip_com(byte com, byte arg_cnt, byte *args);
void handle_midi(void);
void handle_realtime(byte);
void set_MIDI_key_press(void (*ptr)(byte, byte));
void set_MIDI_key_release(void (*ptr)(byte));
void set_song_pos_ptr(void (*ptr)(int));
void set_rt_clock(void (*ptr)());
void set_rt_start(void (*ptr)());
void set_rt_cont(void (*ptr)());
void set_rt_stop(void (*ptr)());
void set_cntrl_chng(void (*ptr)(byte,byte));
void set_prg_chng(void (*ptr)(byte));
void set_MIDI_pitch_bend(void (*ptr)(int));

void (*MIDI_key_press)(byte, byte);
void (*MIDI_key_release)(byte);
void (*song_pos_ptr)(int);
void (*rt_clock)();
void (*rt_start)();
void (*rt_cont)();
void (*rt_stop)();
void (*cntrl_chng)(byte, byte);
void (*prg_chng)(byte);
void (*MIDI_pitch_bend)(int);

#endif /* MIDI_H_ */
