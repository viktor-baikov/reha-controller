#pragma once
#include <stdint.h>
namespace remote {
// Atomic 32-bit selection reference: CC104..108 (five 7-bit chunks), then
// CC109=sum(chunks)&127. Zero means no selected take. Never infer from LCD text.
struct SelectionReference {
    uint32_t value=0, staging=0;
    uint8_t index=0, checksum=0;
    bool valid=false;
    void reset() {value=staging=0;index=checksum=0;valid=false;}
    void cc(uint8_t number,uint8_t data) {
        if(number==104) {index=0;staging=0;checksum=0;valid=false;}
        if(number>=104 && number<=108) {
            if(number!=104+index || data>127 || (index==4 && data>15)) {index=0;valid=false;return;}
            staging|=uint32_t(data)<<(index*7);checksum=(checksum+data)&127;++index;
        } else if(number==109) {
            valid=index==5 && data==checksum && staging!=0;
            if(valid)value=staging;
            index=0;
        }
    }
};
}
