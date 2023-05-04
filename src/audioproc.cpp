#include "audioproc.h"

#include <vector>
#include <algorithm>
#include <mutex>
#include <iostream>
#include <optional>

#include "output_queue.h"

// The sample queue is a buffer of samples that we've captured from the mic,
// hidden behind a mutex. When listening, there is no limit to its size.
// When we are not actively listening, we maintain a few seconds of audio
// to compensate for latency (we need to go *back* a bit from when we first
// started listening to get the beginning of the user's clip).

std::mutex audioproc_mutex;
std::vector<int16_t> sample_queue;

// This is tricky.
//
// Because of latency, we need to look back into the past and forward into the future
// relative to when we receive the user's intent to listen or stop listening.
//
// When user says "start":
//   If start == nullopt:
//     (start, end) = (t - latency, nullopt)
//   If start != nullopt and end != nullopt:
//     end = nullopt.

// When user says "end":
//   Set end = t + latency

// When we receive new audio:
//   Add it to the queue.
//   If start, end both not nullopt and if queue is longer than end:
//     Extract the samples.
//     Shrink buffer.
//     Set (start, end) = (nullopt, nullopt).

std::optional<size_t> capture_start_idx = std::nullopt;
std::optional<size_t> capture_end_idx = std::nullopt;


// DOES NOT ACQUIRE THE LOCK!!! Do it from the caller!
void nolock_maybe_shrink() {
    if (sample_queue.size() > SAMPLE_QUEUE_NOT_LISTENING_MAX_SIZE) {
        size_t overflow_size = sample_queue.size() - SAMPLE_QUEUE_SHRINK_TO;
        sample_queue.erase(sample_queue.begin(), sample_queue.begin() + overflow_size);
    }
}

// Called from audio thread. We acquire the lock for the sample queue and 
// push the samples into it, then release the lock.
void audio_samples_acquired(int16_t * samples, size_t sample_count) {
    std::lock_guard<std::mutex> lock(audioproc_mutex);

    sample_queue.insert(sample_queue.end(), samples, samples + sample_count);
    //std::cout << "sample_queue.size() = " << sample_queue.size() << std::endl;

    if (capture_start_idx.has_value() 
        && capture_end_idx.has_value() 
        && sample_queue.size() > capture_end_idx.value()
    ) {
        std::cout << "WE HAVE CAPTURE from " << capture_start_idx.value() << " to " << capture_end_idx.value() << std::endl;

        output_queue_push(
            sample_queue.data() + capture_start_idx.value(), 
            capture_end_idx.value() - capture_start_idx.value(), 
            SAMPLE_RATE
        );

        capture_start_idx = std::nullopt;
        capture_end_idx = std::nullopt;
    }

    if (!capture_start_idx.has_value()) {
        nolock_maybe_shrink();
    }
}

// Signal to the audioproc module that the user has requested us to listen
void audio_begin_capture() {
    std::lock_guard<std::mutex> lock(audioproc_mutex);

    if (capture_start_idx.has_value()) {
        capture_end_idx = std::nullopt;  // don't end anytime soon
        std::cout << "Capture RESTARTS from " << capture_start_idx.value() << std::endl;
    }
    else {
        capture_start_idx = (SAMPLE_QUEUE_LATENCY > sample_queue.size() ? 
            0 : sample_queue.size() - SAMPLE_QUEUE_LATENCY);
        std::cout << "Capture starts at " << capture_start_idx.value() << std::endl;
    }
    
}

// Signal to the audioproc module that the user has requested us to stop
// listening
void audio_end_capture() {
    std::lock_guard<std::mutex> lock(audioproc_mutex);

    if (!capture_start_idx.has_value()) {
        return;
    }

    capture_end_idx = sample_queue.size() + SAMPLE_QUEUE_LATENCY;
}

bool audio_is_capturing() {
    std::lock_guard<std::mutex> lock(audioproc_mutex);
    return capture_start_idx.has_value();
}

