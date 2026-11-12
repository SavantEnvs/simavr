// Intel HEX firmware loader (read_ihex_chunks, a static function in sim_hex.c).
// The source is included, as upstream's own IHEX_TEST driver does, to reach it directly.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../simavr/sim/sim_hex.c"

static char path[4096];
static int fd = -1;

static void remove_scratch(void)
{
	unlink(path);
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
	const char *dir = getenv("TMPDIR");
	snprintf(path, sizeof path, "%s/fuzz_ihex.XXXXXX", dir && *dir ? dir : "/tmp");
	fd = mkstemp(path);
	if (fd < 0) {
		perror("mkstemp");
		abort();
	}
	atexit(remove_scratch);
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (ftruncate(fd, 0) != 0 || pwrite(fd, data, size, 0) != (ssize_t)size)
		abort();

	fw_chunk_t *chunks = NULL;
	read_ihex_chunks(path, &chunks);
	if (!chunks)
		return -1;	// the loader found no data record to load

	while (chunks) {
		fw_chunk_t *next = chunks->next;
		free(chunks);
		chunks = next;
	}
	return 0;
}
