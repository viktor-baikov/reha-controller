#include "../rehearsal_remote/DfrLcd.hpp"
#include <assert.h>
#include <initializer_list>
#include <string.h>
TestSerial Serial;
TestWire Wire;
uint32_t clockNow=0;
uint32_t millis(){return clockNow;}
void run(DfrLcd &lcd,uint32_t to) {
    const char rows[2][17]={"Test song       ","READY           "};
    for(;clockNow<to;clockNow+=2) lcd.tick(clockNow,rows,24,16,8);
}
void expectDisplay(const char rows[2][17]) {
    char visible[32];memset(visible,' ',32);uint8_t cursor=0;
    for (const auto &tx:Wire.sent) {
        if (tx.address!=0x3E) continue;
        assert(tx.bytes.size()>=2 && tx.bytes.size()<=5);
        const uint8_t value=tx.bytes[1];
        if (tx.bytes[0]==0x80) {
            if (value==1) {memset(visible,' ',32);cursor=0;}
            else if (value&0x80) cursor=value&0x7F;
        } else if (tx.bytes[0]==0x40) {
            for(size_t n=1;n<tx.bytes.size();++n) {
                if(cursor<16) visible[cursor]=tx.bytes[n];
                else if(cursor>=0x40 && cursor<0x50) visible[16+cursor-0x40]=tx.bytes[n];
                ++cursor;
            }
        }
    }
    assert(!memcmp(visible,rows[0],16) && !memcmp(visible+16,rows[1],16));
}
int main() {
    for(uint8_t rgb: {0x60,0x6B,0x30,0x2D}) {
        Wire=TestWire{};Wire.rgb=rgb;clockNow=0;DfrLcd lcd;lcd.begin(18,19);
        run(lcd,600);assert(lcd.present());
        int chars=0,colours=0;bool clear=false;
        for(auto &tx:Wire.sent) {
            if(tx.address==0x3E && tx.bytes[0]==0x40) chars+=tx.bytes.size()-1;
            if(tx.address==0x3E && tx.bytes[0]==0x80 && tx.bytes[1]==1) clear=true;
            if(tx.address==rgb) ++colours;
        }
        assert(chars==32 && colours>=3 && clear);
        size_t before=Wire.sent.size();run(lcd,800);assert(Wire.sent.size()==before); // no flicker/no redundant writes
        Wire.failed=true;const char changed[2][17]={"Changed         ","READY           "};
        lcd.tick(clockNow,changed,24,16,8);assert(!lcd.present());
        Wire.failed=false;run(lcd,2500);assert(!lcd.present());
        run(lcd,4500);assert(lcd.present());
        // An idle, unchanged display must still notice an unplugged module.
        Wire.missing=true;run(lcd,5700);assert(!lcd.present());
        Wire.missing=false;run(lcd,10000);assert(lcd.present());
    }
    // Full redraw: two cursor commands + eight 4-character writes, <=16ms.
    Wire=TestWire{};clockNow=0;DfrLcd fast;fast.begin(18,19);run(fast,600);
    const char full[2][17]={"1234567890123456","abcdefghijklmnop"};
    assert(!fast.matches(full));
    const size_t start=Wire.sent.size();
    for(uint32_t until=clockNow+16;clockNow<until;++clockNow) fast.tick(clockNow,full,24,16,8);
    expectDisplay(full);assert(fast.matches(full));
    assert(fast.lastTextWriteAt()>=clockNow-16 && fast.lastTextWriteAt()<clockNow);
    assert(Wire.sent.size()-start==10);
    // Latest content wins even if the target changes after positioning.
    const char abandoned[2][17]={"X234567890123456","abcdefghijklmnop"};
    fast.tick(clockNow++,abandoned,24,16,8);assert(!fast.matches(abandoned));
    const char newest[2][17]={"Y               ","READY           "};
    for(uint32_t until=clockNow+40;clockNow<until;++clockNow) fast.tick(clockNow,newest,24,16,8);
    expectDisplay(newest);
    // Sparse edit in the second row, then an unchanged row: no clear/flicker.
    const char sparse[2][17]={"Y               ","READY!          "};
    const size_t beforeSparse=Wire.sent.size();
    for(uint32_t until=clockNow+5;clockNow<until;++clockNow) fast.tick(clockNow,sparse,24,16,8);
    expectDisplay(sparse);assert(Wire.sent.size()-beforeSparse==2);
    const size_t idle=Wire.sent.size();
    for(uint32_t until=clockNow+20;clockNow<until;++clockNow) fast.tick(clockNow,sparse,24,16,8);
    assert(Wire.sent.size()==idle);
    // Failure during a batched DATA write (not just set-cursor) invalidates
    // the cached display and recovers by initializing and repainting.
    fast.tick(clockNow++,full,24,16,8); // position at first changed cell
    Wire.failed=true;fast.tick(clockNow++,full,24,16,8);assert(!fast.present());
    Wire.failed=false;
    for(uint32_t until=clockNow+3400;clockNow<until;++clockNow) fast.tick(clockNow,full,24,16,8);
    assert(fast.present());expectDisplay(full);
    Wire=TestWire{};Wire.missing=true;clockNow=0;DfrLcd lcd;lcd.begin(18,19);run(lcd,1000);
    assert(!lcd.present() && Wire.sent.empty());
}
