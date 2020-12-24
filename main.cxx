#include "config.hxx"
#include "cpu.hxx"
#include "bus.hxx"
#include "rom.hxx"


#if UNITTEST==1
int main() {
    test_bus();
    test_cpu();
}
#endif

#if UNITTEST==0
int main(int argc, char **argv) {
    auto rom = Rom(argv[1]);
    auto bus = Bus(&rom);
    auto cpu = Cpu(&bus);
    //cpu.reset();
    while(true)
        cpu.run_instruction();
    return 0;
}
#endif

