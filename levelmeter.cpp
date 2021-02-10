#include <math.h>
#include <string.h>
#include <vector>

#include "levelmeter.h"

LevelMeter::LevelMeter() :
    m_feedCount(0),
	m_avgPos(0),
	m_heldPeakPos(0),
    m_peakLvl(0.0f),
    m_shouldRead(false)
{
	memset(m_avgBuf, 0, sizeof(float) * LEVEL_BUFF_SIZE);
	memset(m_heldPeakBuf, 0, sizeof(float) * LEVEL_BUFF_SIZE);
}

void LevelMeter::feedSample(float sample) {
    float lvl = abs(sample);
    m_feedSum += lvl;

    if (lvl > m_peakLvl) {
        m_peakLvl = lvl;
    }

    m_feedCount++;
    if (m_feedCount >= BLOCK_SIZE) {
        m_feedCount = 0;
        float avg = m_feedSum / BLOCK_SIZE;
        m_feedSum = 0.0f;

        m_avgBuf[m_avgPos] = avg;
        m_avgPos += 1;
        if (m_avgPos >= LEVEL_BUFF_SIZE) {
            m_shouldRead = true;
        }
        m_avgPos %= LEVEL_BUFF_SIZE;
    }
}

bool LevelMeter::shouldRead() {
    if (m_shouldRead) {
        m_shouldRead = false;
        return true;
    }
    return false;
}

float LevelMeter::takePeak() {
	float lvl = m_peakLvl;
	m_peakLvl = 0.0;

	m_heldPeakBuf[m_heldPeakPos] = lvl;
	m_heldPeakPos++;
	m_heldPeakPos %= LEVEL_BUFF_SIZE;

	return lvl;
}

float LevelMeter::avg() const {
	float avg = 0.0f;
	for (int i = 0; i < LEVEL_BUFF_SIZE; i++) {
		avg += abs(m_avgBuf[i]);
	}
	avg /= LEVEL_BUFF_SIZE;
	return avg;
}

float LevelMeter::heldPeak() const {
	float max = 0.0f;
	for (int i = 0; i < LEVEL_BUFF_SIZE; i++) {
		if (m_heldPeakBuf[i] > max) {
			max = m_heldPeakBuf[i];
		}
	}
	return max;
}

void LevelMeter::writeToGuiBuffer(Gui &gui, int offset) {
    std::vector<float> values;
    values.push_back(avg());
    values.push_back(takePeak());
    values.push_back(heldPeak());

    gui.sendBuffer(offset, values);
}