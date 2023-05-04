#pragma once

#include "interface.h"
#include "config.h"

#include <vector>


// Called from audio thread. We acquire the lock for the sample queue and 
// push the samples into it, then release the lock.
void audio_samples_acquired(int16_t * samples, size_t sample_count);


// Signal to the audioproc module that the user has requested us to listen
void audio_begin_capture();

// Signal to the audioproc module that the user has requested us to stop
// listening
void audio_end_capture();


// Returns whether or not we are currently capturing audio
bool audio_is_capturing();
