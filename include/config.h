#pragma once

constexpr int WINDOW_WIDTH = 200;
constexpr int WINDOW_HEIGHT = 50;

constexpr int SAMPLE_RATE = 44100;
constexpr int CHANNELS = 1;
constexpr int BUFFER_SIZE = 1536;

constexpr size_t SAMPLE_QUEUE_NOT_LISTENING_MAX_SIZE = 10 * SAMPLE_RATE * CHANNELS;
constexpr size_t SAMPLE_QUEUE_SHRINK_TO = 5 * SAMPLE_RATE * CHANNELS;

constexpr size_t SAMPLE_QUEUE_LATENCY = static_cast<size_t>(0.2 * SAMPLE_RATE * CHANNELS);
