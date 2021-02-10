#pragma once

#include <Bela.h>

#include "config.h"

class Mixer {
public:
    Mixer();

    void addLayer(float gainFactor = 1.0);
    void writeSample(int offset, float sample);

    void flushToDac(BelaContext *context);

private:
    void sum();
    void reset();

    float m_buf[BLOCK_SIZE];
    float m_mixed[BLOCK_SIZE];
    int m_layers;
    float m_layerGainFactor;
};