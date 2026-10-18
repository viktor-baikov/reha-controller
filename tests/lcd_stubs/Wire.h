#pragma once
#include <stdint.h>
#include <vector>
struct WireTransaction {uint8_t address;std::vector<uint8_t> bytes;};
struct TestWire {
    uint8_t rgb=0x60,address=0;
    bool missing=false,failed=false;
    std::vector<uint8_t> current;
    std::vector<WireTransaction> sent;
    void begin(uint8_t,uint8_t,uint32_t) {}
    void setTimeOut(uint16_t) {}
    void beginTransmission(uint8_t a) {address=a;current.clear();}
    void write(uint8_t value) {current.push_back(value);}
    uint8_t endTransmission() {
        if(failed || missing || (address!=0x3E && address!=rgb)) return 2;
        if(!current.empty()) sent.push_back({address,current});
        return 0;
    }
};
extern TestWire Wire;
