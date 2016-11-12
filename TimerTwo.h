/*==============================================================================================================*
 
    @file     TimerTwo.h
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

#ifndef TimerTwo_h
#define TimerTwo_h

#include <avr/interrupt.h>
#include <Arduino.h>

#define TIMER2_PIN  11

namespace Timertwo {


    class TimerTwo {
        public:
            static void (*onOverflow)(); // not really public, but I can't work out the 'friend' for the SIGNAL
            static void setPeriod(unsigned long);
            static unsigned long getPeriod();
            static void setOnOverflow( void (*)() );
            static void enable();
            static void disable();
        private:
            static uint8_t enabled;
    };
}

using namespace Timertwo;

#endif
