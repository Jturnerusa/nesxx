#include <fstream>
#include "cpu.hxx"
#include "bus.hxx"
using namespace std;

int main(int argc, char **argv) {
    fstream f{argv[1]};
    f.
    Bus bus;
    auto cpu = Cpu(&bus);
    return 0;
}
