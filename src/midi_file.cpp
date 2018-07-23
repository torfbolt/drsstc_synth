/*
 * midi_file.cpp
 *
 *  Created on: 21.05.2018
 *      Author: dk
 */

#include <mgos.h>
#include "midi_file.hpp"
#include "MidiFile.h"

using namespace std;
using namespace smf;

extern "C" {
#include "midi_synth.h"

void midi_file_play_task(void *pvParameters) {
	MidiFile midifile;
	midifile.read((char *)pvParameters);
	midifile.joinTracks();
	midifile.doTimeAnalysis();
	MidiEvent* mev;
	int deltatick, toffs = xTaskGetTickCount();
	for (int event = 0; event < midifile[0].size(); event++) {
		mev = &midifile[0][event];
		deltatick = (int)(mev->seconds * 1e3 / portTICK_PERIOD_MS) - xTaskGetTickCount() + toffs;
		if (deltatick > 0)
			vTaskDelay(deltatick);
		if (mev->isNote())
			call_midi_handler(mev->getP0(), mev->getP1(), mev->getP2());
	}
	midifile.clear();
	vTaskDelete(NULL);
}

}
