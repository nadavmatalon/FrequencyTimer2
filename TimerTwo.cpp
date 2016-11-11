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

#if defined(__AVR__)

    #if defined(TIMER2_COMPA_vect)
        ISR(TIMER2_COMPA_vect)
    #elif defined(TIMER2_COMP_vect)
        ISR(TIMER2_COMP_vect)
    #else
        #error "This board does not have a hardware timer which is compatible with the TimerTwo Library"
    #endif

    {
        static uint8_t inHandler = 0;               // protects from recursion in case handler enables interrupts
        if (!inHandler && TimerTwo::onOverflow) {
            inHandler = 1;
            (TimerTwo::onOverflow)();
            inHandler = 0;
        }
    }

    void TimerTwo::setOnOverflow(void (*func)()) {
        onOverflow = func;
        #if defined(TIMSK2)
            if (func) TIMSK2 |= _BV(OCIE2A);
            else TIMSK2 &= ~_BV(OCIE2A);
        #elif defined(TIMSK)
            if (func) TIMSK |= _BV(OCIE2);
            else TIMSK &= ~_BV(OCIE2);
        #endif
    }

    void TimerTwo::setPeriod(unsigned long period) {
        uint8_t pre, top;
        if (period == 0) period = 1;
        period *= clockCyclesPerMicrosecond();
        period /= 2;                                                            // working with half-cycles before the toggle
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
        #if defined(TCCR2A)
            TCCR2B = 0;
            TCCR2A = 0;
            TCNT2  = 0;
        #if defined(ASSR) && defined(AS2)
            ASSR &= ~_BV(AS2);                                                      // use clock (not T2 pin)
        #endif
            OCR2A = top;
            TCCR2A = (_BV(WGM21) | (TimerTwo::enabled ? _BV(COM2A0) : 0));
            TCCR2B = pre;
        #elif defined(TCCR2)
            TCCR2 = 0;
            TCNT2 = 0;
            ASSR &= ~_BV(AS2);                                                      // use clock (not T2 pin)
            OCR2  = top;
            TCCR2 = (_BV(WGM21) | (TimerTwo::enabled ? _BV(COM20) : 0) | pre);
        #endif
    }

    unsigned long TimerTwo::getPeriod() {
        uint8_t p, shift;
        unsigned long v;
        #if defined(TCCR2B)
            p = (TCCR2B & 7);
            v = OCR2A;
        #elif defined(TCCR2)
            p = (TCCR2 & 7);
            v = OCR2;
        #endif
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
        #if defined(TCCR2A)
            TCCR2A |= _BV(COM2A0);
        #elif defined(TCCR2)
            TCCR2 |= _BV(COM20);
        #endif
    }

    void TimerTwo::disable() {
        TimerTwo::enabled = 0;
        #if defined(TCCR2A)
            TCCR2A &= ~_BV(COM2A0);
        #elif defined(TCCR2)
            TCCR2 &= ~_BV(COM20);
        #endif
    }

#elif defined(__arm__) && defined(TEENSYDUINO)

    void TimerTwo::setPeriod(unsigned long period) {
        uint8_t bdiv;
        uint8_t cdiv = 0;
        if (period == 0) period = 1;
        period *= (F_BUS / 1000000);
        if      (period < 65535 *  16)        bdiv =  0;
        else if (period < 65535 *   2 * 16)   bdiv =  1;
        else if (period < 65535 *   3 * 16)   bdiv =  2;
        else if (period < 65535 *   4 * 16)   bdiv =  3;
        else if (period < 65535 *   5 * 16)   bdiv =  4;
        else if (period < 65535 *   6 * 16)   bdiv =  5;
        else if (period < 65535 *   7 * 16)   bdiv =  6;
        else if (period < 65535 *   8 * 16)   bdiv =  7;
        else if (period < 65535 *   9 * 16)   bdiv =  8;
        else if (period < 65535 *  10 * 16)   bdiv =  9;
        else if (period < 65535 *  11 * 16)   bdiv = 10;
        else if (period < 65535 *  12 * 16)   bdiv = 11;
        else if (period < 65535 *  13 * 16)   bdiv = 12;
        else if (period < 65535 *  14 * 16)   bdiv = 13;
        else if (period < 65535 *  15 * 16)   bdiv = 14;
        else if (period < 65535 *  16 * 16)   bdiv = 15;
        else if (period < 65535 *  18 * 16) { bdiv =  8; cdiv = 1; }
        else if (period < 65535 *  20 * 16) { bdiv =  9; cdiv = 1; }
        else if (period < 65535 *  22 * 16) { bdiv = 10; cdiv = 1; }
        else if (period < 65535 *  24 * 16) { bdiv = 11; cdiv = 1; }
        else if (period < 65535 *  26 * 16) { bdiv = 12; cdiv = 1; }
        else if (period < 65535 *  28 * 16) { bdiv = 13; cdiv = 1; }
        else if (period < 65535 *  30 * 16) { bdiv = 14; cdiv = 1; }
        else if (period < 65535 *  32 * 16) { bdiv = 15; cdiv = 1; }
        else if (period < 65535 *  36 * 16) { bdiv =  8; cdiv = 2; }
        else if (period < 65535 *  40 * 16) { bdiv =  9; cdiv = 2; }
        else if (period < 65535 *  44 * 16) { bdiv = 10; cdiv = 2; }
        else if (period < 65535 *  48 * 16) { bdiv = 11; cdiv = 2; }
        else if (period < 65535 *  52 * 16) { bdiv = 12; cdiv = 2; }
        else if (period < 65535 *  56 * 16) { bdiv = 13; cdiv = 2; }
        else if (period < 65535 *  60 * 16) { bdiv = 14; cdiv = 2; }
        else if (period < 65535 *  64 * 16) { bdiv = 15; cdiv = 2; }
        else if (period < 65535 *  72 * 16) { bdiv =  8; cdiv = 3; }
        else if (period < 65535 *  80 * 16) { bdiv =  9; cdiv = 3; }
        else if (period < 65535 *  88 * 16) { bdiv = 10; cdiv = 3; }
        else if (period < 65535 *  96 * 16) { bdiv = 11; cdiv = 3; }
        else if (period < 65535 * 104 * 16) { bdiv = 12; cdiv = 3; }
        else if (period < 65535 * 112 * 16) { bdiv = 13; cdiv = 3; }
        else if (period < 65535 * 120 * 16) { bdiv = 14; cdiv = 3; }
        else                                { bdiv = 15; cdiv = 3; }
        period /= (bdiv + 1);
        period >>= (cdiv + 4);
        if (period > 65535) period = 65535;
        // high time = (CMD1:CMD2 + 1) ÷ (fCMTCLK ÷ 8)
        // low time  = CMD3:CMD4 ÷ (fCMTCLK ÷ 8)
        SIM_SCGC4 |= SIM_SCGC4_CMT;
        CMT_MSC  = 0;
        CMT_PPS  = bdiv;
        CMT_CMD1 = ((period - 1) >> 8) & 255;
        CMT_CMD2 = (period - 1) & 255;
        CMT_CMD3 = (period >> 8) & 255;
        CMT_CMD4 = period & 255;
        CMT_OC   = 0x60;
        CMT_MSC  = (cdiv << 5) | 0x0B;          // baseband mode
    }

    unsigned long TimerTwo::getPeriod() {
        uint32_t period;
        period = ((CMT_CMD3 << 8) | CMT_CMD4);
        period *= (CMT_PPS + 1);
        period <<= ((CMT_MSC >> 5) & 3) + 4;
        period /= (F_BUS / 1000000);
        return period;
    }

    void TimerTwo::enable() {
        TimerTwo::enabled = 1;
        CORE_PIN5_CONFIG = (PORT_PCR_MUX(2) | PORT_PCR_DSE | PORT_PCR_SRE);
    }

    void TimerTwo::disable() {
        TimerTwo::enabled = 0;
        CORE_PIN5_CONFIG = (PORT_PCR_MUX(1) | PORT_PCR_DSE | PORT_PCR_SRE);
        digitalWriteFast(5, LOW);
    }

    void TimerTwo::setOnOverflow(void (*func)()) {
        if (func) {
            onOverflow = func;
            NVIC_ENABLE_IRQ(IRQ_CMT);
        } else {
            NVIC_DISABLE_IRQ(IRQ_CMT);
            onOverflow = func;
        }
    }

    void TimerTwo::cmt_isr(void) {
        static uint8_t inHandler = 0;
        uint8_t __attribute__((unused)) tmp = CMT_MSC;
        tmp = CMT_CMD2;
        if (!inHandler && TimerTwo::onOverflow) {
            inHandler = 1;
            (*onOverflow)();
            inHandler = 0;
        }
    }

#endif
