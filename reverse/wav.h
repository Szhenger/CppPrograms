// WAV file format structures
// C++ translation of wav.h

#pragma once
#include <cstdint>

// Byte type used throughout
using BYTE = uint8_t;

// WAV file header — must be packed to match the on-disk layout exactly
#pragma pack(push, 1)
struct WAVHEADER
{
    // RIFF chunk descriptor
    BYTE         chunkID[4];      // "RIFF"
    uint32_t     chunkSize;       // file size minus 8 bytes
    BYTE         format[4];       // "WAVE"

    // fmt sub-chunk
    BYTE         subchunk1ID[4];  // "fmt "
    uint32_t     subchunk1Size;   // 16 for PCM
    uint16_t     audioFormat;     // 1 = PCM
    uint16_t     numChannels;     // 1 = mono, 2 = stereo, …
    uint32_t     sampleRate;      // samples per second
    uint32_t     byteRate;        // sampleRate * numChannels * bitsPerSample / 8
    uint16_t     blockAlign;      // numChannels * bitsPerSample / 8
    uint16_t     bitsPerSample;   // 8, 16, 24, 32 …

    // data sub-chunk
    BYTE         subchunk2ID[4];  // "data"
    uint32_t     subchunk2Size;   // number of bytes in the audio data
};
#pragma pack(pop)
