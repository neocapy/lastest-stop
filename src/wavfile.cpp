#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "wavfile.h"

bool write_wav(int16_t *samples, size_t sample_count, int sample_rate, const std::string &filename) {
    
    // Set up WAV header
    WAVHeader header;
    memcpy(header.riff, "RIFF", 4);
    header.chunkSize = 36 + sample_count * sizeof(int16_t);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.subchunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = 1;
    header.sampleRate = sample_rate;
    header.bitsPerSample = 16;
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    memcpy(header.data, "data", 4);
    header.subchunk2Size = sample_count * sizeof(int16_t);

    // Write WAV header and samples to file
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Unable to open output file " << filename << std::endl;
        return false;
    }

    outFile.write(reinterpret_cast<const char *>(&header), sizeof(WAVHeader));
    outFile.write(reinterpret_cast<const char *>(samples), sample_count * sizeof(int16_t));

    if (!outFile) {
        throw std::runtime_error("Error writing .wav to output file");
    }

    outFile.close();
    return true;
}
