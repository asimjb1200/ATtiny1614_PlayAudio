#include "audio_control.h"
#include "Arduino.h"
#include "extmem.h"
#include <util/delay.h>
#define TIMER_PERIPHERAL TCA0
#define SAMPLE_RATE 44100

static constexpr int CLK_FREQ = 20'000'000;
static uint8_t audioBufferOne[256];
static uint8_t audioBufferTwo[256];

volatile uint16_t bytesSent = 0;

volatile bool usingBufferOne = true;
volatile bool bufferOneNeedsFill = true;
volatile bool bufferTwoNeedsFill = true;

void setupDAC()
{
    // select 4.3V reference
    VREF_CTRLA |= VREF_DAC0REFSEL_4V34_gc;
    /* DAC0/AC0 reference enable: enabled */
    VREF.CTRLB |= VREF_DAC0REFEN_bm;

    /* Disable digital input buffer */
    PORTA.PIN6CTRL &= ~PORT_ISC_gm;
    PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    /* Disable pull-up resistor */
    PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm;
    /* Enable DAC, Output Buffer, Run in Standby */
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm | DAC_RUNSTDBY_bm;

    // 25 μs delay is recommended after enabling the VREF
    _delay_us(25);

    // the DAC is now ready for conversions
    DAC0.DATA = 0x20;
}

void initHardwareTimer()
{
    // set interrupt mode to periodic
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;

    /**
     * Write a TOP value to the Compare/Capture register.
     * aka number of clock ticks, from 0, that will trigger an interrupt
     * using the TOP formula defined here: TOP = (CPU clock freq./SampleRate) - 1
     * that gives us - (CLK_FREQ / SAMPLE_RATE) - 1 = 452
     * so count 452 clock cycles then fire an interrupt
     */
    TCB0.CCMP = 452;

    /**
     * Enable the counter by writing a ‘1’ to the ENABLE bit in the Control A (TCBn.CTRLA) register.
     * The counter will start counting clock ticks according to the prescaler setting in the Clock Select (CLKSEL) bit
     * field in the Control A (TCBn.CTRLA) register.
     */
    TCB0_CTRLA |= TCB_ENABLE_bm;

    // enables the interrupt from my timer
    TCB0.INTCTRL = TCB_CAPT_bm;

    // enable global interrupts
    sei();
}

void fillBuffer()
{
    if (bufferOneNeedsFill) {
        readNextAudioChunk(256, audioBufferOne);
        bufferOneNeedsFill = false;
    }
    
    if (bufferTwoNeedsFill) {
        readNextAudioChunk(256, audioBufferTwo);
        bufferTwoNeedsFill = false;
    }
}

ISR(TCB0_INT_vect){
    TCB0.INTFLAGS = TCB_CAPT_bm; /* Clear the interrupt flag */
    if (playbackEnabled) 
    {
        uint8_t* currentBuffer = usingBufferOne ? audioBufferOne : audioBufferTwo;
        
        DAC0.DATA = currentBuffer[bytesSent++];

        if (bytesSent >= 256) {
            bytesSent = 0;
            if (usingBufferOne) {
                usingBufferOne = false;
                bufferOneNeedsFill = true; // Signal the loop to refill Buffer 1
            } else {
                usingBufferOne = true;
                bufferTwoNeedsFill = true; // Signal the loop to refill Buffer 2
            }
        }
    }
}