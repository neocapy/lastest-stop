#pragma once

#include <cstdint>
#include <cstddef>

// thread safe
void output_queue_push(int16_t * samples, size_t sample_count, int sample_rate);

void output_queue_start_thread();