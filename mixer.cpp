#include "mixer.h"

#include <string.h>


Mixer::Mixer() :
    m_layers(0) {
    memset(&m_buf, 0, sizeof(float) * BLOCK_SIZE);
    memset(&m_mixed, 0, sizeof(float) * BLOCK_SIZE);
}

void Mixer::addLayer(float gainFactor) {
    if (m_layers > 0) {
        sum();
    }
    m_layers++;
    m_layerGainFactor = gainFactor;
}

void Mixer::writeSample(int offset, float sample) {
    m_buf[offset] = sample * m_layerGainFactor;
}

void Mixer::flushToDac(BelaContext *context) {
    float factor = 1.0f;

    if (m_layers > 0 ) {
        factor /= (float)m_layers;
    }
    
    sum();
    for (int i = 0; i < BLOCK_SIZE; i++) {
        audioWrite(context, i, 0, m_mixed[i] * factor);
        audioWrite(context, i, 1, m_mixed[i] * factor);
    }
    reset();
}


void Mixer::sum() {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        m_mixed[i] += m_buf[i];
    }
    memset(&m_buf, 0, sizeof(float) * BLOCK_SIZE);
}

void Mixer::reset() {
    m_layers = 0;
    memset(&m_mixed, 0, sizeof(float) * BLOCK_SIZE);
}

