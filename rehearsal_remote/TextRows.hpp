#pragma once
#include <stdint.h>
#include <string.h>
namespace remote {
// Atomic bounded two-row ASCII metadata, transported as ordinary channel CCs.
struct TextRows {
    char rows[2][33] = {{0}, {0}}, pending[33] = {0};
    uint8_t row = 255, length = 0, index = 0, checksum = 0;
    bool sized = false, received[2] = {false,false};
    bool complete() const { return received[0] && received[1]; }
    uint32_t started = 0;
    void reset() { row = 255; sized = false; rows[0][0] = rows[1][0] = 0; received[0]=received[1]=false; }
    void tick(uint32_t now) { if (row != 255 && uint32_t(now-started) >= 1500) row = 255; }
    bool cc(uint8_t number, uint8_t value, uint32_t now) {
        tick(now);
        if (number == 100) {
            row = value <= 1 ? value : 255; sized = false; index = 0; started = now;
        } else if (number == 101 && row != 255) {
            if (value > 32) { row = 255; return false; }
            length = value; checksum = row + length; sized = true; index = 0;
        } else if (number == 102 && row != 255 && sized) {
            if (index >= length || value < 32 || value > 126) { row = 255; return false; }
            pending[index++] = value; checksum = (checksum + value) & 127;
        } else if (number == 103 && row != 255 && sized) {
            const auto destination = row; row = 255;
            if (index != length || value != checksum) return false;
            pending[index] = 0; memcpy(rows[destination], pending, index+1); received[destination]=true; return true;
        }
        return false;
    }
};
}
