#pragma once
#include <stdint.h>
namespace remote {
struct VolumePickup {
    bool known = false, armed = false, sampled = false;
    uint8_t target = 127, previous = 0, lastSent = 255;
    void reset() { known = armed = sampled = false; lastSent = 255; }
    void feedback(uint8_t value) {
        if (!known || value != target) {
            if (value != lastSent) armed = false;
            target = value;
            known = true;
        }
    }
    bool sample(uint8_t value) {
        if (!known) return false;
        const int distance = int(value) - target;
        const bool crossed = sampled && ((int(previous) - target) * distance <= 0);
        previous = value; sampled = true;
        if (!armed && (distance >= -3 && distance <= 3 || crossed)) armed = true;
        if (!armed || value == lastSent) return false;
        lastSent = value;
        return true;
    }
};
}
