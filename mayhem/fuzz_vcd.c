// VCD stimulus input (avr_vcd_init_input) attached to a simulated ATmega328P, then run
// for a fixed number of instructions so the queued VCD events are delivered to the IRQs.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sim_avr.h"
#include "sim_vcd_file.h"

#define RJMP_SELF 0xCFFF	// rjmp .-2
#define RUN_STEPS 20000

static char path[4096];
static int fd = -1;

static void remove_scratch(void)
{
	unlink(path);
}

static void no_sleep(avr_t *avr, avr_cycle_count_t how_long)
{
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	const char *dir = getenv("TMPDIR");
	snprintf(path, sizeof path, "%s/fuzz_vcd.XXXXXX", dir && *dir ? dir : "/tmp");
	fd = mkstemp(path);
	if (fd < 0) {
		perror("mkstemp");
		abort();
	}
	atexit(remove_scratch);
	if (!(stdout = fopen("/dev/null", "r+")))
		abort();
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (ftruncate(fd, 0) != 0 || pwrite(fd, data, size, 0) != (ssize_t)size)
		abort();

	avr_t *avr = avr_make_mcu_by_name("atmega328p");
	if (!avr)
		abort();
	avr_init(avr);
	avr->log = LOG_NONE;
	avr->sleep = no_sleep;
	for (uint32_t a = 0; a <= avr->flashend; a += 2) {
		uint16_t op = RJMP_SELF;
		memcpy(avr->flash + a, &op, sizeof op);
	}
	avr->sreg[S_I] = 1;

	avr_vcd_t input;
	int rc = avr_vcd_init_input(avr, path, &input);
	if (rc == 0) {
		avr->vcd = &input;
		for (int i = 0; i < RUN_STEPS && avr->state != cpu_Done &&
				avr->state != cpu_Crashed; i++)
			avr_run(avr);
		avr->vcd = NULL;
		avr_vcd_close(&input);
	}
	avr_terminate(avr);
	free(avr);
	return rc == 0 ? 0 : -1;
}
