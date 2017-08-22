#include <time.h>

#include "vch315.h"

#if WIN32

#pragma comment(lib, "Ws2_32.lib")
#define errno WSAGetLastError()

#else

#include <errno.h>

#endif

const int eth_timeout = 10;

VCH315::Command cmd;

VCH315::VCH315()
{
}

int VCH315::connect( const char *address, const char *port )
{
	const int BUF_LEN = 1024;

	int iResult;

#if WIN32
	WSADATA wsaData;
	// Initialize Winsock
	if (iResult = WSAStartup(MAKEWORD(2,2), & wsaData)) {
		return -1;
	}
#endif

	struct addrinfo *result = NULL,
	                *ptr = NULL,
	                hints;

	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	if ( (iResult = getaddrinfo(address, port, & hints, & result)) ) {
		cleanup();
		return -1;
	}

	ConnectSocket = INVALID_SOCKET;

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (ConnectSocket == INVALID_SOCKET) {
		freeaddrinfo(result);
		cleanup();
		return -1;
	}

	// Connect to server.
	iResult = ::connect(ConnectSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		int err = errno;
		ConnectSocket = INVALID_SOCKET;
	}

	// Should really try the next address returned by getaddrinfo
	// if the connect call failed
	// But for this simple example we just free the resources
	// returned by getaddrinfo and print an error message

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		int err = errno;
		cleanup();
		return -1;
	}


	return 0;
}

int VCH315::disconnect()
{
	cleanup();

	return 1;
}

int VCH315::init( int active_channels )
{
	cmd( VCH315::SYNCHR, 0, 0 );
	cmd( VCH315::RESET, 0, 0 );

	for ( int chan = 0; chan < active_channels; ++chan )
		cmd( VCH315::RESETFIFO, 0, chan );

	return 1;
}

int VCH315::cmd( VCH315::Command cmd, unsigned param1, unsigned param2 )
{
	char buf[1024];

	buf[0] = VCH315::SOM;
	buf[1] = cmd;
	buf[2] = '0' + param1;
	buf[3] = '0' + param2;
	buf[4] = VCH315::EOM;
	buf[5] = 0;

	return write( buf );
}

int VCH315::meas( Meas meas[], int meas_len )
{
	unsigned time;
	unsigned val;

	char buf[4096];

	cmd( VCH315::GETCOUNT, 0, 0 );

	Sleep( 200 );

	int offset = 0;
	int rd;

	do {

		rd = read( buf + offset, 4096 - offset );
		offset += rd;

	} while( rd > 0 && buf[offset-1] != VCH315::EOM );

	// Incorrect response
	if ( buf[0] != VCH315::SOM )
		return -1;

	// buf[1-4] - checksum

	time_t rawtime;
	struct tm timeinfo;
	timeinfo.tm_year = 82;
	timeinfo.tm_mon = 1;
	timeinfo.tm_mday = 18;
	timeinfo.tm_hour = 0;
	timeinfo.tm_min = 0;
	timeinfo.tm_sec = 0;
	rawtime = mktime( &timeinfo );

	if ( buf[5] != CONTROL_DATA )
		return -1;

	int channels_cnt = (buf[6] - '0') * 10 + (buf[7] - '0');

	for ( int i = 0; i < channels_cnt && i < meas_len; ++i ) {
		char *data = buf + i * 21 + 8;
		if ( data[0] != ' ' )
			return -1;

		int chan = (data[1] - '0') * 10 + (data[2] - '0');

		if ( data[3] != ' ' )
			return -1;

		time = packed2num( data + 4 );
		val  = packed2num( data + 13 );

		struct tm *meas_tm;
		time_t meas_time = rawtime + time;
		meas_tm = gmtime( &meas_time );

		meas[i].time = meas_time;
		meas[i].value = val / 99900000.0;
		meas[i].chan = chan;

		if ( data[12] != ' ' )
			return -1;
	}

	return channels_cnt;
}

unsigned VCH315::packed2num( const char *ptr )
{
	union {
		struct {
			unsigned b1:4;
			unsigned b2:4;
			unsigned b3:4;
			unsigned b4:4;
			unsigned b5:4;
			unsigned b6:4;
			unsigned b7:4;
			unsigned b8:4;
		} data;

		unsigned num;

	} res;

	res.data.b1 = ptr[0] - '0';
	res.data.b2 = ptr[1] - '0';
	res.data.b3 = ptr[2] - '0';
	res.data.b4 = ptr[3] - '0';
	res.data.b5 = ptr[4] - '0';
	res.data.b6 = ptr[5] - '0';
	res.data.b7 = ptr[6] - '0';
	res.data.b8 = ptr[7] - '0';

	return res.num;
}

int VCH315::read( char *buf, int buf_size )
{
	fd_set sock_set;
	int nfds;
	timeval timeout;

	timeout.tv_sec  = eth_timeout;
	timeout.tv_usec = 0;
	
#if WIN32

	sock_set.fd_array[0] = ConnectSocket;
	sock_set.fd_count = 1;

	nfds = 0;

#else

	FD_ZERO( &sock_set );
	FD_SET( ConnectSocket, &sock_set );

	nfds = ConnectSocket + 1;

#endif

	int res = select( nfds, &sock_set, NULL, NULL, &timeout );

	if ( res == 0 )
		return 0;
	else if ( res == SOCKET_ERROR )
		return -1;

	if ( res != 1 )
		return 0;

	res = recv( ConnectSocket, buf, buf_size, 0 );

	//if ( res > 0)
	//	fprintf( stdout, "Bytes received: %d\n", res );
	//else if ( res == 0 )
	//	log_error( "Connection closed", 0 );
	//else
	//	log_error( "recv failed", errno );

	return res;
}

int VCH315::write(const char *str)
{
	return send(ConnectSocket, str, (int) strlen(str), 0);
}

int VCH315::cleanup()
{
#if WIN32
	if ( ConnectSocket != INVALID_SOCKET )
		closesocket(ConnectSocket);

	WSACleanup();

#endif

	return 0;
}



