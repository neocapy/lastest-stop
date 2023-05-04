#pragma once

#include <cstdint>
#include <string>

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4];            // RIFF chunk
    uint32_t chunkSize;      // File size - 8
    char wave[4];            // WAVE
    char fmt[4];             // fmt  chunk
    uint32_t subchunk1Size;  // SubChunk1 size (16 for PCM)
    uint16_t audioFormat;    // Audio format (1 for PCM)
    uint16_t numChannels;    // Number of channels
    uint32_t sampleRate;     // Sample rate
    uint32_t byteRate;       // Byte rate (sampleRate * numChannels * bitsPerSample/8)
    uint16_t blockAlign;     // Block align (numChannels * bitsPerSample/8)
    uint16_t bitsPerSample;  // Bits per sample
    char data[4];            // data chunk
    uint32_t subchunk2Size;  // SubChunk2 size (numSamples * numChannels * bitsPerSample/8)
};
#pragma pack(pop)

bool write_wav(int16_t *samples, size_t sample_count, int sample_rate, const std::string &filename);