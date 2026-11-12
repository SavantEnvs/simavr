// AVR core and peripherals: the input is a flash image for one of several simulated MCUs
// (the first byte picks the MCU), executed for a fixed number of instructions with the
// UART console echo enabled.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_avr.h"
#include "avr_uart.h"

#define RUN_STEPS 20000

static const char *const mcus[] = {
	"atmega328p", "atmega2560", "attiny85", "atmega644", "atmega88",
	"attiny84", "atmega32u4", "attiny2313", "atmega128", "attiny1634",
};

static void no_sleep(avr_t *avr, avr_cycle_count_t how_long)
{
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	if (!(stdout = fopen("/dev/null", "r+")))
		abort();
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 3)
		return -1;

	avr_t *avr = avr_make_mcu_by_name(mcus[data[0] % (sizeof mcus / sizeof mcus[0])]);
	if (!avr)
		abort();
	avr_init(avr);
	avr->log = LOG_NONE;
	avr->sleep = no_sleep;

	uint32_t flags = AVR_UART_FLAG_STDIO;
	avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);

	size_t n = size - 1;
	if (n > (size_t)avr->flashend + 1)
		n = (size_t)avr->flashend + 1;
	memcpy(avr->flash, data + 1, n);

	for (int i = 0; i < RUN_STEPS && avr->state != cpu_Done &&
			avr->state != cpu_Crashed; i++)
		avr_run(avr);

	avr_terminate(avr);
	free(avr);
	return 0;
}
