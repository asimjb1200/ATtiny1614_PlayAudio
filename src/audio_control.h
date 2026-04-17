#pragma once
#include <stdint.h>
#define DAC PIN_PA6

extern volatile bool playbackEnabled;
extern uint32_t lastMemoryAddress;
extern volatile uint16_t bytesSent;
extern volatile bool bufferOneNeedsFill;
extern volatile bool bufferTwoNeedsFill;
extern volatile bool usingBufferOne;

void setupDAC();
void initHardwareTimer();
void fillBuffer();