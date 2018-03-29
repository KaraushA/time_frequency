#pragma once


/*
        Opens VI session by device name and configures it to communicate with the instrument.
    Takes device name (e.g. "COM1" or "ttyS0") and external clock value
    (SR_EXT_CLK_FREQ_10MHZ or SR_EXT_CLK_FREQ_5MHZ), returns VI session identificator
    opened and configured. If the function fails, it returns
    'INVALID_HANDLE_VALUE' and the error code can be retrieved by calling
    GetLastError(). Returned session should be closed with key53230_close() on exit.
*/

/*
        Start measurement and returns the result.
        Takes VI session. The session must be opened and configured by calling
        key53230_open_config(). On failure returns error code.
*/

/*
        Close VI session.
*/


class TimeIntervalCntr {

public:

    /* External clock frequency */
    enum EXT_CLK_FREQ {
            EXT_CLK_FREQ_10MHZ = 0,
            EXT_CLK_FREQ_5MHZ = 1
    };

    virtual int connect(const char *name) = 0;
    virtual int set_ext_freq(EXT_CLK_FREQ freq) = 0;
    virtual int set_trigger_level(int channel, double level) = 0;
    virtual int measure(double &meas) = 0;
    virtual void close() = 0;

};
