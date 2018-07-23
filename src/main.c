#include "mgos.h"
#include "midi_file.hpp"
#include "midi_synth.h"
#include "midi_uart.h"
#include "http_srv.h"

enum mgos_app_init_result mgos_app_init(void) {
	xTaskCreate(midi_synth_task, "midi_synth_task", 4096, NULL, 0, &midi_synth_handle);
	xTaskCreate(midi_uart_task, "midi_uart_task", 4096, NULL, 10, NULL);
//	xTaskCreate(midi_file_play_task, "midi_file_play_task", 4096,
//			    "Monkey_Island.mid", 20, &midi_file_play_handle);
	http_server_init();

	return MGOS_APP_INIT_SUCCESS;
}
