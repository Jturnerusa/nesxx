#ifndef ROM_H
#define ROM_H
#include <cstdint>
#include <vector>
#include <config.hxx>

class Rom {
private:
    static const int HEADER_SIZE = 16;
    static const int TRAINER_SIZE = 512;
    static const int PRGROM_UNIT_SIZE = 16384;
    static const int CHRROM_UNIT_SIZE = 8192;
public:
    enum class MirroringType{
        horizontal,
        vertical
    };
private:
    enum class Mapper {
        nrom
    };
private:
    Mapper mapper;
    std::vector<uint8_t> rom;
    std::vector<uint8_t> prgrom;
    std::vector<uint8_t> chrrom;
    uint16_t trunacate_prgrom_address(uint16_t);
    MirroringType mirroring_type;
public:
    void load_from_file(const char*);
    uint8_t read_prgrom(uint16_t);
    uint8_t read_chrrom(uint16_t);
    MirroringType get_mirroring_type() {return this->mirroring_type;};

};

#ifdef UNITTEST

class DummyRom : public Rom {
public:
    DummyRom();
    uint8_t read_prgrom(uint16_t);
    uint8_t read_chrrom(uint16_t);
};

#endif

#endif