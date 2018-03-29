/* 
   Header file of module which controls Keysight 53230a time interval counter
   through VISA interface.
*/

#pragma once

#include "tic.h"
#include "visa.h"

#if WIN32

#else

typedef int HANDLE;

#endif

class TicKey53230: public TimeIntervalCntr {

public:
    int connect(const char *name);
    int set_ext_freq(EXT_CLK_FREQ freq);
    int set_trigger_level(int channel, double level);
    int measure(double &meas);
    void close();

private:
    ViSession tic = VI_NULL, defaultRM = VI_NULL;
    EXT_CLK_FREQ ext_freq = EXT_CLK_FREQ_5MHZ;
    double chan1_lvl = 0.5, chan2_lvl = 0.5;
};



