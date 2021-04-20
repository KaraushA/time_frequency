/*
    Source file of module which controls SR620 universal time interval counter
    through comm port.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "visa.h"
#include "key53230.h"

#if WIN32 // WIN32

//static const auto& snprintf = _snprintf;

#else // WIN32

#include <termios.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>

#endif // WIN32


/*
    Opens comm port by name and configures it to communicate with the
    instrument. Takes port name (e.g. "COM1") and external clock value
    (SR_EXT_CLK_FREQ_10MHZ or SR_EXT_CLK_FREQ_5MHZ), returns handle of the
    port opened and configured. If the function fails, it returns
    'INVALID_HANDLE_VALUE' and the error code can be retrieved by calling
    GetLastError(). Returned handle should be closed with CloseHandle() on
    exit.
*/


int TicKey53230::connect(const char *name)
{

#if WIN32

    int err;

    err = viOpenDefaultRM(&defaultRM);

    if (err != VI_SUCCESS)
        return err;

    printf("viOpenDefaultRM succeded\n");

    err = viOpen(defaultRM, name, VI_NULL, VI_NULL, &tic);

    if (err != VI_SUCCESS)
        return err;

    printf("viOpen succeded: %s\n", name);

    const char *init_strings[] = {
        "*RST\n",
        "*CLS\n",
        "*ESE 0\n",
        "SYST:TIM 5\n",
        "CONF:TINT (@2), (@1)\n",
        "INP:IMP 50\n",
        "INP2:IMP 50\n",
        "INP:COUP DC\n",
        "INP2:COUP DC\n",
        "INP:SLOP POS\n",
        "INP2:SLOP POS\n"
    };

    for (int i = 0; i < sizeof(init_strings) / sizeof(init_strings[0]); ++i) {
        err = viWrite(tic, (ViConstBuf) init_strings[i], (ViUInt32) strlen(init_strings[i]), VI_NULL);
        if (err != VI_SUCCESS)
            return err;
    }

    // Set trigger levels
    const char trig_level_str = "INP%1d:LEV %.2f\n";
    char trig_level[50];

    sprintf(trig_level, trig_level_str, 1, chan1_lvl);
    err = viWrite(tic, (ViConstBuf) trig_level, (ViUInt32) strlen(trig_level), VI_NULL);
    if (err != VI_SUCCESS)
        return err;

    sprintf(trig_level, trig_level_str, 2, chan2_lvl);
    err = viWrite(tic, (ViConstBuf) trig_level, (ViUInt32) strlen(trig_level), VI_NULL);
    if (err != VI_SUCCESS)
        return err;


    printf("Init succeded\n");

    return 0;

#else

    int fd = open( name, O_RDWR | O_NOCTTY );
    if (fd == -1)
        return fd;

    struct termios config;

    if ( tcgetattr( fd, &config ) < 0 )
        STF_RETURN_ERROR(fd);

    //
    // Input flags - Turn off input processing
    //
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    //
    config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK | ISTRIP | IXON);

    //
    // Output flags - Turn off output processing
    //
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    //
    // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    config.c_oflag = 0;

    //
    // No line processing
    //
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    //
    config.c_lflag &= ~(ECHO | ECHONL | IEXTEN | ISIG);
    config.c_lflag |= ICANON;

    //
    // Turn off character processing
    //
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input
    //
    config.c_cflag &= ~(CSIZE | PARENB);
    config.c_cflag |= CS8 | CSTOPB;

    //
    // One input byte is enough to return from read()
    // Inter-character timer off
    //
    config.c_cc[VMIN]  = 1;
    config.c_cc[VTIME] = 0;

    //
    // Communication speed (simple version, using the predefined
    // constants)
    //
    config.c_ospeed = B9600;
    config.c_ispeed = B9600;

    //
    // Finally, apply the configuration
    //
    if ( tcsetattr(fd, TCSAFLUSH, &config) < 0 )
        STF_RETURN_ERROR(fd);

    tcflush( fd, TCIOFLUSH );

    char sr_mode_str[255];
    snprintf((char*) sr_mode_str, 255,
            "MODE0;CLCK1;CLKF%1d;LOCL1;TCPL0;SRCE0;AUTM0;ARMM1;SIZE1"
            "LEVL1,1;LEVL2,1;TSLP1,0;TSLP2,0\n",
            sr_ext_clk_freq);

    if ( write(fd, (const void *) sr_mode_str, strlen(sr_mode_str)) < 0 )
        STF_RETURN_ERROR(fd);

    return fd;

#endif
}


/*
    Start measurement and returns the result.
    Takes port handle. The port must be opened and configured by calling
    sr620_open_config_port_by_name() . On failure returns error	code.
*/
int TicKey53230::measure(double &meas)
{
    const char *meas_str = "READ?\n";

    char buf[80];

    meas = 0.0;

#if WIN32

    ViUInt32 written, rd;
    int err;

    err = viWrite(tic, (ViConstBuf) meas_str, (ViUInt32) strlen(meas_str), &written);
    if (err != VI_SUCCESS)
        return err;

    err = viRead(tic, (ViPBuf) buf, 80, &rd);
    if (err != VI_SUCCESS) {
        viStatusDesc(tic, err, buf);
        printf("Read error: %s\n", buf);
        return err;
    }

#else

    ssize_t rd;

    if ( write(hport, sr_meas_str, strlen(sr_meas_str)) < 0 )
        return errno;

    rd = read(hport, buf, 80);

#endif

    if ( rd < 2 )
        return -1;

    buf[rd-1] = '\0';


    meas = strtod(buf, NULL);

    return 0;
}

int TicKey53230::set_ext_freq(EXT_CLK_FREQ freq)
{
    ext_freq = freq;

    return 0;
}

int TicKey53230::set_trigger_level(int channel, double level)
{
    if (channel == 1)
        chan1_lvl = level;
    else if ( channel == 2)
        chan2_lvl = level;
    else
        return -1;

    return 0;
}

void TicKey53230::close()
{
#if WIN32
    if (tic != VI_NULL)
        viClose(tic);
    if (defaultRM != VI_NULL)
        viClose(defaultRM);

    tic = defaultRM = VI_NULL;
#else
    close(hport);
#endif
}
