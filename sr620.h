/* 
   Header file of module which controls SR 620 universal time interval counter 
   through comm port. 
*/

#pragma once

#include "tic.h"

#if WIN32

#include <windows.h>

#else

typedef int HANDLE;

#endif

class TicSR620: public TimeIntervalCntr {

public:
    int connect(const char *name);
    int set_ext_freq(EXT_CLK_FREQ freq);
    int set_trigger_level(int channel, double level);
    int measure(double &meas);
    void close();

private:
    HANDLE hport = INVALID_HANDLE_VALUE;
    EXT_CLK_FREQ ext_freq = EXT_CLK_FREQ_5MHZ;
    double chan1_lvl = 0.5, chan2_lvl = 0.5;

};
