#pragma once

#include <libraries/Gui/Gui.h>

#include "config.h"

#define LEVEL_BUFF_SIZE 30

class LevelMeter {

public:
    LevelMeter();

    void feedSample(float sample);
    bool shouldRead();

    float takePeak();
    float avg() const;
    float heldPeak() const;

    void writeToGuiBuffer(Gui &gui, int offset);

private:
    int m_feedCount;
    float m_feedSum;

    int m_avgPos;
    float m_avgBuf[LEVEL_BUFF_SIZE];

    int m_heldPeakPos;
    float m_heldPeakBuf[LEVEL_BUFF_SIZE];

    float m_peakLvl;

    bool m_shouldRead;
};