/*
 * midi_file.hpp
 *
 *  Created on: 21.05.2018
 *      Author: dk
 */

#ifndef SRC_MIDI_FILE_HPP_
#define SRC_MIDI_FILE_HPP_

#ifdef __cplusplus
extern "C" {
#endif
TaskHandle_t midi_file_play_handle;

void midi_file_play_task(void *pvParameters);
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* SRC_MIDI_FILE_HPP_ */
