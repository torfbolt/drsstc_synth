/*
 * midi_uart.c
 *
 *  Created on: 20.05.2018
 *      Author: dk
 */

#include <mgos.h>
#include "midi_synth.h"

#define UART_NO 1

static QueueHandle_t uart_queue;

static void uart_dispatcher(int uart_no, void *arg) {
	assert(uart_no == UART_NO);
	uint8_t rxb[32];
	size_t rx_av = mgos_uart_read(uart_no, rxb, sizeof(rxb));
	for (int i = 0; i < rx_av; i++)
		xQueueSendToBack(uart_queue, rxb + i, 0);
	(void) arg;
}

uint8_t uart_getc() {
	uint8_t result;
	xQueueReceive(uart_queue, &result, portMAX_DELAY);
	return result;
}

void init_midi_uart(void) {
	struct mgos_uart_config ucfg;
	mgos_uart_config_set_defaults(UART_NO, &ucfg);
	ucfg.baud_rate = 31250;
	mgos_uart_configure(UART_NO, &ucfg);
	mgos_uart_set_dispatcher(UART_NO, uart_dispatcher, NULL);
	mgos_uart_set_rx_enabled(UART_NO, true);
}

void midi_uart_task(void *pvParameters) {
	uint8_t status = 0, byte1 = 0, byte2 = 0;
	uart_queue = xQueueCreate(32, 1);
	init_midi_uart();

	while (1) {
		while ((status = uart_getc(0)) < 0x80)
			;
		byte1 = uart_getc(0);
		switch (status & 0xF0) {
		case 0x80:
		case 0x90:
		case 0xA0:
		case 0xB0:
		case 0xE0:
			byte2 = uart_getc(0);
		}
		call_midi_handler(status, byte1, byte2);
	}
}

