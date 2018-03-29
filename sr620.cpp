/*
	Source file of module which controls SR620 universal time interval counter
	through comm port.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr620.h"

#if WIN32 // WIN32

//static const auto& snprintf = _snprintf;

#else // WIN32

#include <termios.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>

#endif // WIN32

int TicSR620::connect(const char *name)
{

#if WIN32

    hport =
            CreateFileA(
                            name,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (hport == INVALID_HANDLE_VALUE)
            return -1;

    COMMTIMEOUTS CommTimeouts;
    memset(&CommTimeouts, 0, sizeof(COMMTIMEOUTS));
    CommTimeouts.ReadTotalTimeoutConstant = 10000;
    CommTimeouts.ReadIntervalTimeout = 4;

    if (!SetCommTimeouts(hport, &CommTimeouts))
            return -2;

    DCB ComDCM;
    memset(&ComDCM, 0, sizeof(ComDCM));

    ComDCM.DCBlength = sizeof(DCB);

    ComDCM.BaudRate = 9600;
    ComDCM.ByteSize = 8;
    ComDCM.Parity = NOPARITY;
    ComDCM.fBinary = 1;
    ComDCM.StopBits = TWOSTOPBITS;
    ComDCM.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(hport, &ComDCM))
            return -2;

    BOOL ret = PurgeComm(hport,
                    PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    if (ret == 0)
            return -3;

    if (!EscapeCommFunction(hport, CLRDTR))
            return -4;

    ComDCM.fDtrControl = DTR_CONTROL_HANDSHAKE;
    if (!SetCommState(hport, &ComDCM))
            return -5;

    DWORD written;
    //char *sn = "\n\n\n";
    //if (!WriteFile(hport, sn, strlen(sn), &written, NULL)) // purge SR buffer this way
            //STF_RETURN_ERROR(hport);

    char sr_mode_str[255];
    snprintf((char*) sr_mode_str, 255,
            "MODE0;CLCK1;CLKF%1d;LOCL1;TCPL0;SRCE0;AUTM0;ARMM1;SIZE1"
            "LEVL1,1;LEVL2,1;TSLP1,0;TSLP2,0\n",
            ext_freq);

    if (!WriteFile(hport, sr_mode_str, strlen(sr_mode_str), &written, NULL))
            return -6;

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
    sprintf((char*) sr_mode_str,
            "MODE0;CLCK1;CLKF%1d;LOCL1;TCPL0;SRCE0;AUTM0;ARMM1;SIZE1"
            "LEVL1,1;LEVL2,1;TSLP1,0;TSLP2,0\n",
            sr_ext_clk_freq);

    if ( write(fd, (const void *) sr_mode_str, strlen(sr_mode_str)) < 0 )
        STF_RETURN_ERROR(fd);

    return fd;

#endif
}

/*	
	Opens comm port by name and configures it to communicate with the
	instrument. Takes port name (e.g. "COM1") and external clock value
	(SR_EXT_CLK_FREQ_10MHZ or SR_EXT_CLK_FREQ_5MHZ), returns handle of the
	port opened and configured. If the function fails, it returns
	'INVALID_HANDLE_VALUE' and the error code can be retrieved by calling
	GetLastError(). Returned handle should be closed with CloseHandle() on
	exit.
*/

/*
        Opens comm port by number and configures it to communicate with the instrument.
        Takes port number (e.g. 1 for "COM1" or "ttyS0") and external clock value
        (SR_EXT_CLK_FREQ_10MHZ or SR_EXT_CLK_FREQ_5MHZ), returns handle of the port
        opened and configured. If the function fails, it returns
        'INVALID_HANDLE_VALUE' and the error code can be retrieved by calling
        GetLastError(). Returned handle should be closed with CloseHandle() on exit.
*/


/* 
	Start measurement and returns the result. 
	Takes port handle. The port must be opened and configured by calling 
	sr620_open_config_port_by_name() . On failure returns error	code.
*/

int TicSR620::measure(double &meas)
{
    // These are seem to be the same
    //const char *sr_meas_str = "STRT;*WAI;XAVG?\n";
    const char *sr_meas_str = "MEAS? 0;*WAI\n";

    char buf[80];

	meas = 0.0;

#if WIN32

	DWORD written, rd, tot = 0;
	if (!WriteFile(hport, sr_meas_str, strlen(sr_meas_str), &written, NULL))
		return GetLastError();

#else

	ssize_t rd, tot = 0;
	if ( write(hport, sr_meas_str, strlen(sr_meas_str)) < 0 )
		return errno;

#endif

	do {

#if WIN32
		if (!ReadFile(hport, buf + tot, 80 - tot, &rd, NULL))
			return GetLastError();
#else
		rd = read(hport, buf + tot, 80 - tot);
#endif

		tot += rd;

		// Correct respond is received
		if ( tot >= 2 && buf[tot-1] == '\n' && buf[tot-2] == '\r' )
			break;

	} while ( rd != 0 );

	// Nothing is read, possibly a timeout
	if ( tot < 2 )
        return -1;

    buf[tot-2] = '\0';

    meas = strtod(buf, NULL);

	return 0;
}

int TicSR620::set_ext_freq(EXT_CLK_FREQ freq)
{
    ext_freq = freq;

    return 0;
}

int TicSR620::set_trigger_level(int channel, double level)
{
    if (channel == 1)
        chan1_lvl = level;
    else if ( channel == 2)
        chan2_lvl = level;
    else
        return -1;

    return 0;
}

void TicSR620::close()
{
#if WIN32

    if (hport != INVALID_HANDLE_VALUE)
        CloseHandle(hport);

    hport = INVALID_HANDLE_VALUE;

#else
	close(hport);
#endif
}
