#include "cpu.hxx"
#include "bus.hxx"
#include "rom.hxx"

int main(int argc, char **argv) {
    auto rom = Rom(argv[1]);
    auto bus = Bus(&rom);
    auto cpu = Cpu(&bus);
    return 0;
}
