#pragma once

#if WIN32

#include <windows.h>
//#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#else

#include <sys/socket.h>	/* basic socket definitions */
#include <netinet/in.h>	/* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>	/* inet(3) functions */
#include <netdb.h>


typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;

#endif



class VCH315 {

public:

	enum Command {
		SOM = 0x01,         // Заголовок. Начало любого сообщения или команды.
		EOM = 0x1A,         // Конец любого сообщения или команды.
		SYNCHR = 0x33,      // Синхронизация шкал. Посылается при запуске программы. 
		TEST = 0x36,        // Проверка связи. Посылается при отсутствии ответа от компаратора в течении 2 секунд
		GETVER = 0x37,      // Дать версию ПО
		REGETSEN = 0x3C,    // Повторить посылку. Посылается при несовпадении контрольной суммы
		GETCOUNT = 0x3A,    // Передать имеющиеся данные из буфера FIFO
		RESETFIFO = 0x3D,   // Сбросить буфер FIFO канала. Параметр – номер канала, например, два байта 0x32 0x33 – канал 23 (нумеруются с 0).
							// Команда посылается при запуске «измерений» в канале. На самом деле измерения в компараторе происходят всегда. 
							// Программ лишь использует или не использует их результаты.
		RESET = 0x3F,       // Сброс процессора. Посылается при запуске программы
		SETTIME0 = 0x40,    // Установить время. В компараторе фиксируется момент взятия выборок фазы. Счетчик времени – 4 байта. 
							// Данная команда устанавливает младший байт счетчика времени. Параметры команды – два полубайта + 0x30. 
							// Например, послед. байт 0x1 0x40 0x31 0x3f установит младший байт счетчика времени в состояние 0x1f.
		SETTIME1 = 0x41,    // Аналогично для 1 – 3 байта счетчика времени.
		SETTIME2 = 0x42,
		SETTIME3 = 0x43,
	};

	static const int CONTROL_DATA = '0';  // Передача данных
	static const int CONTROL_CHAN = '1';  // Изменилось число каналов
	static const int CONTROL_OVFL = '2';  // Переполнен буфер FIFO

	struct Meas {
		time_t time;
		int chan;
		double value;
	};

	VCH315();

	int connect( const char *address, const char *port );
	int disconnect();
	int init( int active_channels );
	int meas( Meas meas[], int meas_len );
	int cmd( Command cmd, unsigned param1, unsigned param2 );

	int read( char *buf, int buf_size );
	int write(const char *str);

private:

	unsigned packed2num( const char *ptr );

	int cleanup();

	char address[100];
	char port[100];
	SOCKET ConnectSocket;
};
