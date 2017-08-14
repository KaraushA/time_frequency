/* 
	Source file of module which controls Vremya-Ch high-frequency signals switch 
	through comm port.
*/

#include "vch603.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if WIN32

#ifndef STF_RETURN_ERROR
#define STF_RETURN_ERROR(handle) { \
	DWORD err = GetLastError(); \
	if (handle != INVALID_HANDLE_VALUE) \
		CloseHandle(handle); \
	SetLastError(err); \
	return INVALID_HANDLE_VALUE; }
#endif

static const auto& snprintf = _snprintf;

#else

#include <termios.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>

#ifndef STF_RETURN_ERROR
#define STF_RETURN_ERROR(fd) { \
        int err = errno; \
		if (fd != -1) \
			close(fd); \
        errno = err; \
        return -1; }
#endif

#endif

HANDLE vch603_open_config_helper(char *name)
{

#if WIN32

	HANDLE hport = CreateFileA(
							pname,
							GENERIC_READ | GENERIC_WRITE, 
							0, 
							NULL, 
							OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL,
							NULL);

	if (hport == INVALID_HANDLE_VALUE)
		return hport;

	DCB ComDCM;
	memset(&ComDCM, 0, sizeof(ComDCM));

	ComDCM.DCBlength = sizeof(DCB);

	ComDCM.BaudRate = 9600;
	ComDCM.ByteSize = 8;
	ComDCM.Parity = NOPARITY;
	ComDCM.fBinary = 1;
	ComDCM.StopBits = ONESTOPBIT;

	if (!SetCommState(hport, &ComDCM))
		STF_RETURN_ERROR(hport);	

	BOOL ret =
		PurgeComm(hport,
				PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);


	return hport;

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

    return fd;

#endif
}

/*	
	Opens comm port by name and configures it to communicate with the instrument. 
	Takes port name (e.g. "COM1"), returns handle of the port opened and 
	configured. If the function fails, it returns 'INVALID_HANDLE_VALUE' and 
	the error code can be retrieved by calling GetLastError(). Returned handle 
	should be closed with CloseHandle() on exit. 
*/
HANDLE vch603_open_config_port_by_name(char *name)
{
	char pname[80];

#if WIN32
    strcpy(pname, "\\\\.\\");
    strcat(pname, name);
#else
    strcpy(pname, "/dev/");
    strcat(pname, name);
#endif

	return vch603_open_config_helper(pname);
}

/* 
	Selects working input of the instrument. Takes port handle and the 
	input number (from 1 to 50 for VCH-603). The number is not validated. 
	The port must be opened and configured by calling 
	vch603_open_config_port_by_name(). Returns 0 on success, on failure returns 1 
	and the error code can be retrieved with GetLastError() 
*/
int vch603_set_input(HANDLE hport, int inputNum)
{
	char buf[5];
	snprintf((char*) buf, 5, "A%02d\r\0", inputNum);

#if WIN32

	DWORD written;
	if (!WriteFile(hport, buf, strlen(buf), &written, NULL))
		STF_RETURN_ERROR(INVALID_HANDLE_VALUE);

#else

	if ( write(hport, (const void *) buf, strlen(buf)) < 0 )
		STF_RETURN_ERROR(-1);

#endif

	return 0;
}

/* 
	Selects working output of the instrument. Takes port handle and the 
	output number (from 1 to 5 for VCH-603). Prerequisites, behaviour and return 
	value are the same as for vch603_set_input()
*/
int vch603_set_output(HANDLE hport, int outputNum)
{
	char buf[5];
	snprintf((char*) buf, 5, "B%1d\r\0", outputNum);

#if WIN32

	DWORD written;
	if (!WriteFile(hport, buf, strlen(buf), &written, NULL))
		STF_RETURN_ERROR(INVALID_HANDLE_VALUE);

#else

	if ( write(hport, (const void *) buf, strlen(buf)) < 0 )
		STF_RETURN_ERROR(-1);

#endif

	return 0;
}

/* 
	Switches input to ouput on or off. Takes port handle and onOff parameter. 
	Pass 1 as onOff to switch on and 0 to switch off. The port must be opened 
	and configured by calling vch603_open_config_port_by_name(). 
	Returns 0 on success, 1 - on failure. Use GetLastError() to retrieve 
	the error code.
*/
int vch603_switch(HANDLE hport, enum VCH_SWITCH_ON_OFF switchOnOff)
{
	char buf[5];
	snprintf((char*) buf, 5, "C%1d\r\0", switchOnOff);

#if WIN32

	DWORD written;
	if (!WriteFile(hport, buf, strlen(buf), &written, NULL))
		STF_RETURN_ERROR(INVALID_HANDLE_VALUE);

#else

	if ( write(hport, (const void *) buf, strlen(buf)) < 0 )
		STF_RETURN_ERROR(-1);

#endif

	return 0;
}

/* 
	Resets the instrument - switches off previously selected terminals.
	Takes port handle. The port must be opened and configured by calling 
	vch603_open_config_port_by_name() . Returns 0 on success, on failure returns
	1 and the error code can be retrieved with GetLastError() .
	This might be similar to vch603_switch(port_handle, 0) .
*/
int vch603_reset(HANDLE hport) 
{
	char buf[5];
	snprintf((char*) buf, 5, "D\r\0");

#if WIN32

	DWORD written;
	if (!WriteFile(hport, buf, strlen(buf), &written, NULL))
		STF_RETURN_ERROR(INVALID_HANDLE_VALUE);

#else

	if ( write(hport, (const void *) buf, strlen(buf)) < 0 )
		STF_RETURN_ERROR(-1);

#endif

	return 0;
}

int vch603_close(HANDLE hport)
{
#if WIN32

	CloseHandle(hport);

#else

	close(hport);

#endif
}
