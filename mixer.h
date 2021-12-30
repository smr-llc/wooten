#pragma once

#include <Bela.h>

#include "config.h"
#include <liquid/liquid.h>
#include "recorder.h"

class Mixer {
public:
    enum SampleRate {
        Rate_44100 = 0,
        Rate_22050 = 1
    };

    Mixer();
    ~Mixer();

    void addLayer(float gainFactor = 1.0, int rate = Rate_44100);
    void writeSample(int offset, float sample);

    void flushToDac(BelaContext *context, Recorder *recorder);

private:
    void sum();
    void reset();

    float m_buf[BLOCK_SIZE];
    float m_mixed[BLOCK_SIZE];
    float m_mixed22050[BLOCK_SIZE/2];
    int m_layers;
    float m_layerGainFactor;
    int m_layerSampleRate;
    resamp2_crcf m_resampler;
};