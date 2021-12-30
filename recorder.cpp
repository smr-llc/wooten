#include "recorder.h"
#include <iomanip>
#include <ctime>
#include <sstream>

typedef struct WavHeader {
    char      riff[4];
    uint32_t  riffSize;
    char      wave[4];
    char      fmt[4];  
    uint32_t  fmtSize;                            
    uint16_t  audioFormat;
    uint16_t  nChannels;              
    uint32_t  nSamplesPerSec;                        
    uint32_t  nAvgBytesPerSec;
    uint16_t  nBlockAlign;
    uint16_t  wBitsPerSample;
    char      data[4];
    uint32_t  dataSize;

} WavHeader; 

WavHeader HEADER_BASE = {
    {'R', 'I', 'F', 'F'},
    0U,
    {'W', 'A', 'V', 'E'},
    {'f', 'm', 't', ' '},
    16U,
    0x0003U, // WAVE_FORMAT_IEEE_FLOAT
    1U,
    44100U,
    176400U,
    4U,
    32U,
    {'d', 'a', 't', 'a'},
    0U
};

bool Recorder::isRecording() {
    return m_outfile.is_open();
}

void Recorder::startRecording() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << "/root/recording-" << std::put_time(&tm, "%Y%m%d-%H%M%S") << ".wav";

    m_outfile.open(oss.str(), std::ios::out | std::ios::binary | std::ios::trunc);
    m_outfile.write((const char *)&HEADER_BASE, sizeof(HEADER_BASE));
}

void Recorder::stopRecording() {
    WavHeader realHeader = HEADER_BASE;

    realHeader.dataSize = m_samplesWritten * 4;
    realHeader.riffSize = 4 + 24 + 8 + realHeader.dataSize;

    m_outfile.seekp(0);
    m_outfile.write((const char *)&realHeader, sizeof(realHeader));

    m_outfile.close();
}

void Recorder::writeSamples(const float * samples, uint32_t sampleLength) {
    m_outfile.write((const char *) samples, sampleLength * 4);
    m_samplesWritten += sampleLength;
}