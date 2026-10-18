#include <assert.h>
#include <initializer_list>
#include "../rehearsal_remote/SelectionReference.hpp"
void feed(remote::SelectionReference &r,uint32_t value) {
    uint8_t checksum=0;
    for(int i=0;i<5;++i) {uint8_t part=(value>>(7*i))&127;checksum=(checksum+part)&127;r.cc(104+i,part);}
    r.cc(109,checksum);
}
int main() {
    remote::SelectionReference r;
    for(uint32_t value: {1u,127u,128u,0xffffffffu}) {feed(r,value);assert(r.valid && r.value==value);}
    feed(r,0);assert(!r.valid);
    r.cc(104,1);r.cc(106,2);r.cc(109,3);assert(!r.valid);
    r.cc(104,1);r.cc(105,0);r.cc(106,0);r.cc(107,0);r.cc(108,16);r.cc(109,17);assert(!r.valid);
    feed(r,42);r.cc(104,1);assert(!r.valid); // invalidate authority immediately
    r.reset();assert(!r.valid && r.value==0);
}
