#include <Bela.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "config.h"
#include "wootrx.h"
#include "wootpkt.h"


WootRx::WootRx() :
	m_readPos(0),
	m_writePos(0),
	m_rxQueueSize(8),
	m_rxLevel(0.9),
	m_lastSeq(0)
{
	m_resampler = resamp2_crcf_create(4, 0.0f, 60.0f);
}

WootRx::~WootRx() {
	resamp2_crcf_destroy(m_resampler);
}

LevelMeter * const WootRx::levelMeter() {
	return &m_meter;
}

void WootRx::setRxQueueSize(int size) {
	if (size < 1 || size > MAX_RX_QUEUE_SIZE) {
		return;
	}
	m_rxQueueSize = size;
}

int WootRx::rxQueueSize() const {
	return m_rxQueueSize;
}

void WootRx::setRxLevel(float level) {
	if (level < 0.0f) {
		return;
	}
	m_rxLevel = level;
}

float WootRx::rxLevel() const {
	return m_rxLevel;
}

void WootRx::handleFrame(const WootPkt &pkt) {
	uint16_t expected = m_lastSeq + 1;
	if (pkt.header.seq > expected) {
		printf("WARN: Seq skip %d - %d\n", m_lastSeq, pkt.header.seq);
	}
	else if (pkt.header.seq < m_lastSeq) {
		printf("WARN: Out of order seq %d - %d\n", m_lastSeq, pkt.header.seq);
	}
	m_lastSeq = pkt.header.seq;

	for (int i = 0; i < NETBUFF_SAMPLES; i++) {
		m_buf[m_writePos + i] = ((float)pkt.samples[i]) / 32768.0f;
	}
	m_writePos += NETBUFF_SAMPLES;
	m_writePos %= RINGBUFF_SAMPLES;
}

void WootRx::writeReceivedFrame(BelaContext *context, Mixer &mixer) {
	int bufSamples = m_writePos - m_readPos;
	if (bufSamples < 0) {
		bufSamples += RINGBUFF_SAMPLES;
	}
	if (bufSamples > (NETBUFF_SAMPLES * m_rxQueueSize)) {
		m_readPos += (RINGBUFF_SAMPLES + bufSamples) - (NETBUFF_SAMPLES * m_rxQueueSize);
		m_readPos %= RINGBUFF_SAMPLES;
	}

	mixer.addLayer(m_rxLevel);
	if (bufSamples >= NETBUFF_SAMPLES) {
		liquid_float_complex upsampled[2];
		liquid_float_complex sample;
		sample.imag = 0.0f;
		for(unsigned int n = 0; n < NETBUFF_SAMPLES; n++) {
			sample.real = m_buf[m_readPos + n];
			// upsample/interpolate from 22.05 back to 44.1
			resamp2_crcf_interp_execute(m_resampler, sample, upsampled);
			mixer.writeSample(n * 2, upsampled[0].real);
			mixer.writeSample(n * 2 + 1, upsampled[1].real);
			m_meter.feedSample(upsampled[0].real);
			m_meter.feedSample(upsampled[1].real);
		}
		m_readPos += NETBUFF_SAMPLES;
		m_readPos %= RINGBUFF_SAMPLES;
	}
}
