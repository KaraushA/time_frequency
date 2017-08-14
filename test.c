
#include <stdio.h>

#if WIN32

#include <windows.h>

#else

#include <errno.h>

#endif

#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "vch603.h"
#include "sr620.h"

int test_vch603(int argc, char**argv)
{
	HANDLE hport = vch603_open_config_port_by_name("COM1");

	const int DELAY_MS = 250;

	const int N = 5;

	for (int i = 0; i < N; ++i)
	{
		vch603_reset(hport);

		sleep(DELAY_MS);

		vch603_set_input(hport, (i % 50) + 1);
		vch603_set_output(hport,(i % 5) + 1);
		vch603_switch(hport, VCH_SWITCH_ON);

		sleep(DELAY_MS);

		vch603_switch(hport, VCH_SWITCH_OFF);
	}

	vch603_close(hport);
}

int test_sr620(int argc, char** argv)
{
	HANDLE hport = 
		sr620_open_config_port_by_name("COM2", SR_EXT_CLK_FREQ_5MHZ);	

	if (hport == -1) {
#if WIN32
		int err = GetLastError();
		fprintf(stderr, "error code %d\n", err);
#else
		int err = errno;
		fprintf(stderr, "error code %d\n", err);
#endif
		return 1;
	}

	const int N = 15;
	double rez;

	for (int i = 0; i < N; ++i)
	{
		printf("%d : ", i);
		fflush(stdout);
		int ret = sr620_measure(hport, rez);

		printf("%e\n", rez);
		fflush(stdout);
		sleep(rand()%10000);
	}

	sr620_close(hport);

	return 0;
}

int main(int argc, char **argv)
{
	srand(time(NULL));

	test_sr620(argc, argv);
	test_vch603(argc, argv);

	return 0;
}
