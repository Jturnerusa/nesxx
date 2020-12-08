#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "rom.h"
#include "config.h"


int main(int argc, char *argv[]) {
    struct CPU cpu;
    init_cpu(&cpu);
    struct ROM rom;
    FILE *f = fopen("nestest.nes", "rb");
    fseek(f, 0, SEEK_END);
    int fsize = ftell(f);
    rewind(f);
    char *buff = malloc(sizeof(char) * fsize);
    if (!buff) {
        printf("Failed to allocate memory for reading in rom file\n");
        exit(EXIT_FAILURE);
    }
    size_t bytes_read = fread(buff, 1, fsize, f);
    if (!bytes_read) {
        exit(EXIT_FAILURE);
    }
    init_rom(&rom, buff, fsize);
    load_prgrom(&cpu, &rom);
    while (1) {
        run_instruction(&cpu);
    }
    return 0;
}
