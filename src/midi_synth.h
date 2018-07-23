/*
 * midi_synth.h
 *
 *  Created on: 13.11.2017
 *      Author: dk
 */

#ifndef MIDI_SYNTH_H_
#define MIDI_SYNTH_H_

#include <mgos.h>

#define RMT_CLOCK_FREQ		8000000									// 8MHz
#define PULSE_WIDTH_MAX		(RMT_CLOCK_FREQ / 10000)				// 100us
#define MAX_NOTES 			8

typedef struct {
	uint64_t note_start;
	uint64_t next_pulse;
	uint32_t period;
	uint32_t pulse_width;
	bool playing;
	uint8_t channel;
	uint8_t pitch;
} tc_note_t;

TaskHandle_t midi_synth_handle;

uint64_t get_tick_count();
void synth_note_off(char channel, char pitch, char velocity);
void synth_note_on(char channel, char pitch, char velocity);
void call_midi_handler(char status, char byte1, char byte2);
void midi_synth_task(void *pvParameters);

#endif /* MIDI_SYNTH_H_ */
