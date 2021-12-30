#pragma once

#include <iostream>
#include <fstream>

class Recorder {
public:
    bool isRecording();
    void startRecording();
    void stopRecording();
    void writeSamples(const float * samples, uint32_t sampleLength);

private:
    std::ofstream m_outfile;
    uint32_t m_samplesWritten;
};