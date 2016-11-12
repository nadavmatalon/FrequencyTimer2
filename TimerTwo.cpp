/*==============================================================================================================*
 
    @file     TimerTwo.cpp
    @author   Nadav Matalon
    @license  MIT (c) 2016 Nadav Matalon

    Timer2 Interrupt & Frequency Generator

    Ver. 1.0.0 - Modified code from FrequencyTimer2 by J. Studt (11.11.16)
    (http://www.arduino.cc/playground/Code/FrequencyTimer2)
 
 *===============================================================================================================*
    LICENSE
 *===============================================================================================================*
 
    The MIT License (MIT)
    Copyright (c) 2016 Nadav Matalon

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
    documentation files (the "Software"), to deal in the Software without restriction, including without
    limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
    the Software, and to permit persons to whom the Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial
    portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
    LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 *==============================================================================================================*/

#if 1
__asm volatile ("nop");
#endif

#include "TimerTwo.h"

void (*TimerTwo::onOverflow)() = 0;

uint8_t TimerTwo::enabled = 0;

ISR(TIMER2_COMPA_vect) {
    static uint8_t inHandler = 0;               // protects from recursion in case handler enables interrupts
    if (!inHandler && TimerTwo::onOverflow) {
        inHandler = 1;
        (TimerTwo::onOverflow)();
        inHandler = 0;
    }
}

void TimerTwo::setOnOverflow(void (*func)()) {
    onOverflow = func;
    if (func) bitSet(TIMSK2, OCIE2A);
    else bitClear(TIMSK2, OCIE2A);
}

void TimerTwo::setPeriod(unsigned long period) {
    uint8_t pre, top;
    if (period == 0) period = 1;
//    period *= clockCyclesPerMicrosecond();   // clockCyclesPerMicrosecond = 16,000,000 / 1,000,000 = 16
//    period /= 2;                                                            // working with half-cycles before the toggle
    period *= 8;
    if (period <= 256) {
        pre = 1;
        top = (period - 1);
    } else if (period <= 256L * 8) {
        pre = 2;
        top = (period / 8 - 1);
    } else if (period <= 256L * 32) {
        pre = 3;
        top = (period / 32 - 1);
    } else if (period <= 256L * 64) {
        pre = 4;
        top = (period / 64 - 1);
    } else if (period <= 256L * 128) {
        pre = 5;
        top = (period / 128 - 1);
    } else if (period <= 256L * 256) {
        pre = 6;
        top = (period / 256 - 1);
    } else if (period <= 256L * 1024) {
        pre = 7;
        top = (period / 1024 - 1);
    } else {
        pre = 7;
        top = 255;
    }
    TCCR2B = 0;
    TCCR2A = 0;
    TCNT2 = 0;
    ASSR &= ~_BV(AS2);    // use clock, not T2 pin
    OCR2A = top;
    TCCR2A = (_BV(WGM21) | ( TimerTwo::enabled ? _BV(COM2A0) : 0));
    TCCR2B = pre;
}

unsigned long TimerTwo::getPeriod() {
    uint8_t p, shift;
    unsigned long v;
    p = (TCCR2B & 7);
    v = OCR2A;
    switch (p) {
          case 0 ... 1: shift =  0; break;
          case 2:       shift =  3; break;
          case 3:       shift =  5; break;
          case 4:       shift =  6; break;
          case 5:       shift =  7; break;
          case 6:       shift =  8; break;
          case 7:       shift = 10; break;
    }
    return ((((v + 1) << (shift + 1)) + 1) / clockCyclesPerMicrosecond());   // shift+1 converts from half-period to period
}

void TimerTwo::enable() {
    TimerTwo::enabled = 1;
    bitSet(TCCR2A, COM2A0);
}

void TimerTwo::disable() {
    TimerTwo::enabled = 0;
    bitClear(TCCR2A, COM2A0);
}
