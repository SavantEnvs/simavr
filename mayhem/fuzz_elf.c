// ELF firmware loader (elf_read_firmware). File-input AFL target: the loader exit()s on a
// short program-header read, so it cannot run in-process under libFuzzer.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_elf.h"

int main(int argc, char **argv)
{
	if (argc < 2)
		return 2;

	elf_firmware_t fw;
	memset(&fw, 0, sizeof fw);
	if (elf_read_firmware(argv[1], &fw) != 0)
		return 1;

	while (fw.chunks) {
		fw_chunk_t *next = fw.chunks->next;
		free(fw.chunks);
		fw.chunks = next;
	}
	return 0;
}
