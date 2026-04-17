#include <Arduino.h>
#include <stdint.h>
#include <extmem.h>
#include <audio_control.h>
#include <SPI.h>

#define AUDIO_BUFFER_SIZE 256

volatile bool playbackEnabled = false;

void initMCUClock() 
{
  // disable prescaler
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.MCLKCTRLB = 0 << CLKCTRL_PEN_bp;

  // Set the clock to use 20MHz
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc;

  // give time for clock to switch if necessary
  while (CLKCTRL.MCLKSTATUS & CLKCTRL_SOSC_bm){}
}

void setup() 
{
  initMCUClock();

  // Serial defaults to Physical Pin 7 (PB2) in this core
  Serial.begin(115200);

  // Setup Chip Select
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH); // Ensure flash is 'OFF' (Active Low)

  // Initialize SPI hardware (Pins 11, 12, 13)
  SPI.begin();

  // start the audio data from the beginning
  lastMemoryAddress = 0;

  delay(10000);

  Serial.println("Filling audio buffers");

  // fill both buffers to start
  fillBuffer();

  Serial.println("buffers full");
  

  Serial.println("DAC init begin");
  setupDAC();
  Serial.println("DAC init complete");

  Serial.println("Timer set up begin");
  initHardwareTimer();
  Serial.println("Timer set up complete");
}

void loop() {
  // see if a buffer needs filled
  if (bufferOneNeedsFill || bufferTwoNeedsFill)
  {
    fillBuffer();
  }

  if (Serial.available() > 0)
  {
    char response = Serial.read();
    if (response == 'p') playbackEnabled = true;

    if (response == 's') playbackEnabled = false;
  }
}
