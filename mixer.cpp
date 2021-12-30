#include "mixer.h"

#include <string.h>


Mixer::Mixer() :
    m_layers(0),
    m_layerGainFactor(1.0f),
    m_layerSampleRate(Rate_44100)
{
    memset(&m_buf, 0, sizeof(float) * BLOCK_SIZE);
    memset(&m_mixed, 0, sizeof(float) * BLOCK_SIZE);
    memset(&m_mixed22050, 0, sizeof(float) * BLOCK_SIZE / 2);

	m_resampler = resamp2_crcf_create(4, 0.0f, 60.0f);
}

Mixer::~Mixer() {
	resamp2_crcf_destroy(m_resampler);
}

void Mixer::addLayer(float gainFactor, int rate) {
    if (m_layers > 0) {
        sum();
    }
    m_layers++;
    m_layerGainFactor = gainFactor;
    m_layerSampleRate = rate;
}

void Mixer::writeSample(int offset, float sample) {
    m_buf[offset] = sample * m_layerGainFactor;
}

void Mixer::flushToDac(BelaContext *context, Recorder *recorder) {
    float factor = 1.0f;

    if (m_layers > 0 ) {
        factor /= (float)m_layers;
    }
    
    // mix in last layer
    sum();

    // upsample and integrate 22.05kHz mix
    liquid_float_complex upsampled[2];
    liquid_float_complex sample;
    sample.imag = 0.0f;
    for (int i = 0; i < BLOCK_SIZE/2; i++) {
        sample.real = m_mixed22050[i];
        // upsample/interpolate from 22.05 back to 44.1
        resamp2_crcf_interp_execute(m_resampler, sample, upsampled);
        m_mixed[i*2] += upsampled[0].real;
        m_mixed[i*2 + 1] += upsampled[1].real;
    }

    // write mix to DAC
    for (int i = 0; i < BLOCK_SIZE; i++) {
        audioWrite(context, i, 0, m_mixed[i] * factor);
        audioWrite(context, i, 1, m_mixed[i] * factor);
    }

    if (recorder && recorder->isRecording()) {
        recorder->writeSamples(m_mixed, BLOCK_SIZE);
    }
    reset();
}


void Mixer::sum() {
    if (m_layerSampleRate == Rate_22050) {
        for (int i = 0; i < BLOCK_SIZE/2; i++) {
            m_mixed22050[i] += m_buf[i];
        }
    }
    else {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            m_mixed[i] += m_buf[i];
        }
    }
    memset(&m_buf, 0, sizeof(float) * BLOCK_SIZE);
}

void Mixer::reset() {
    m_layers = 0;
    memset(&m_mixed, 0, sizeof(float) * BLOCK_SIZE);
    memset(&m_mixed22050, 0, sizeof(float) * BLOCK_SIZE / 2);
}

