/*
 * midi_synth.c
 *
 *  Created on: 14.11.2017
 *      Author: dk
 */

#include "midi_synth.h"

#include <math.h>
#include <driver/rmt.h>
#include <esp_attr.h>

#define RMT_BLOCK_LEN		32
#define RMT_TICKS_EMPTY		(RMT_CLOCK_FREQ / RMT_BLOCK_LEN / 1000)	// -> empty block = 1ms
const float pitch_lut[12] = { 1., 1.05946309, 1.12246205, 1.18920712,
		1.25992105, 1.33483985, 1.41421356, 1.49830708, 1.58740105, 1.68179283,
		1.78179744, 1.88774863 };

static const rmt_config_t rmt_cfg = { .rmt_mode = RMT_MODE_TX, .channel =
		RMT_CHANNEL_0, .gpio_num = 16, .mem_block_num = 1, .tx_config = {
		.loop_en = 1 }, .clk_div = 80000000 / RMT_CLOCK_FREQ };

static rmt_item32_t DRAM_ATTR rmt_empty;
rmt_item32_t DRAM_ATTR rmt_buffer[RMT_BLOCK_LEN];
volatile int DRAM_ATTR tx_offset;
tc_note_t DRAM_ATTR notes[MAX_NOTES];
volatile uint64_t DRAM_ATTR tick_cnt = 0;

unsigned int pitch_to_period(char pitch) {
	return (unsigned int) (RMT_CLOCK_FREQ
			/ (8.1757989156 * pitch_lut[pitch % 12] * (1 << (pitch / 12))) + 0.5);
}

uint64_t get_tick_count() {
	return tick_cnt;
}

void synth_note_off(char channel, char pitch, char velocity) {
	for (tc_note_t *n = notes; n < notes + MAX_NOTES; n++) {
		if (n->channel == channel && n->pitch == pitch)
			n->playing = false;
	}LOG(LL_INFO, ("Note OFF: pitch %d", pitch));
}

void synth_note_on(char channel, char pitch, char velocity) {
	tc_note_t *n = notes;
	// look for a free spot for the new note
	while (n->playing) {
		if (++n == notes + MAX_NOTES)
			return; // all spots occupied, exit
	}
	n->channel = channel;
	n->pitch = pitch;
	n->note_start = tick_cnt;
	n->period = pitch_to_period(pitch);
	n->next_pulse = n->note_start + n->period;
	n->pulse_width = 80;
	n->playing = true;
	LOG(LL_INFO, ("Note ON: pitch %d, start %lld, period %d", pitch, n->note_start, n->period));
}

void call_midi_handler(char status, char byte1, char byte2) {
	switch (status & 0xF0) {
	case 0x80:
		synth_note_off(status & 0xF, byte1, byte2);
		break;
	case 0x90:
		if (byte2)
			synth_note_on(status & 0xF, byte1, byte2);
		else
			// velocity = 0 -> note off
			synth_note_off(status & 0xF, byte1, byte2);
		break;
	case 0xA0:
		case 0xB0:
		case 0xC0:
		case 0xD0:
		case 0xE0:
		case 0xF0:
		break;
	}
}

static void IRAM_ATTR rmt_tx_stream_isr(void* arg) {
	uint32_t intr_st = RMT.int_st.val;
	uint32_t i, j;
	uint8_t channel;
	tc_note_t* next_note;
	for (i = 24; i < 32; i++) {
		if (intr_st & (BIT(i))) {
			channel = i - 24;
			RMT.int_clr.val = BIT(i);  // clear interrupt
			for (j = 0; j < RMT_BLOCK_LEN; j++) {
				next_note = NULL;
				for (tc_note_t *n = notes; n < notes + MAX_NOTES; n++)
					if (n->playing && ((!next_note) || (next_note->next_pulse > n->next_pulse)))
						next_note = n;
				if (next_note) {
					int dt = next_note->next_pulse - tick_cnt;
					if (dt > RMT_TICKS_EMPTY) {
						rmt_buffer[j] = rmt_empty;
					} else if (dt <= 0) {
						next_note->next_pulse += next_note->period;
						j--;
						continue;
					} else {
						uint32_t pw_sum = MIN(next_note->pulse_width, PULSE_WIDTH_MAX);
						for (tc_note_t *n = notes; n < notes + MAX_NOTES; n++)
							if (n->playing && (n != next_note) &&
									(next_note->next_pulse + 2 * pw_sum >= n->next_pulse)) {
								pw_sum = MIN((int )sqrt(pw_sum * pw_sum + n->pulse_width * n->pulse_width),
										PULSE_WIDTH_MAX);
								n->next_pulse += n->period + ((tick_cnt / 13) % pw_sum) - pw_sum / 2;
							}
						rmt_buffer[j].level0 = 0;
						rmt_buffer[j].duration0 = 0x7FFF & dt;
						rmt_buffer[j].level1 = 1;
						rmt_buffer[j].duration1 = 0x7FFF & pw_sum;
						next_note->next_pulse += next_note->period;
					}
				} else {
					rmt_buffer[j] = rmt_empty;
				}
				tick_cnt += rmt_buffer[j].duration0 + rmt_buffer[j].duration1;
			}

			rmt_fill_tx_items(channel, rmt_buffer, RMT_BLOCK_LEN, tx_offset);
			if (tx_offset == 0) {
				tx_offset = RMT_BLOCK_LEN;
			} else {
				tx_offset = 0;
			}
		}
	}
}

void init_rmt_output() {
	rmt_empty.duration0 = rmt_empty.duration1 = RMT_TICKS_EMPTY / 2;
	ESP_ERROR_CHECK(rmt_config(&rmt_cfg));
	for (int i = 0; i < RMT_BLOCK_LEN; i++)
		rmt_buffer[i] = rmt_empty;
	ESP_ERROR_CHECK(rmt_fill_tx_items(rmt_cfg.channel, rmt_buffer, RMT_BLOCK_LEN, 0));
	ESP_ERROR_CHECK(rmt_fill_tx_items(rmt_cfg.channel, rmt_buffer, RMT_BLOCK_LEN, RMT_BLOCK_LEN));
	tx_offset = 0;
	ESP_ERROR_CHECK(rmt_isr_register(rmt_tx_stream_isr, NULL, 0, NULL));
	ESP_ERROR_CHECK(rmt_set_tx_thr_intr_en(rmt_cfg.channel, 1, RMT_BLOCK_LEN));
	ESP_ERROR_CHECK(rmt_tx_start(rmt_cfg.channel, 1));
}

void midi_synth_task(void *pvParameters) {
	init_rmt_output();
	while (1) {
		vTaskDelay(100);
		LOG(LL_INFO, ("TC: %lld, RAM: %d", tick_cnt, mgos_get_free_heap_size())); //
	}
	vTaskDelete(NULL);
}
