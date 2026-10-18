#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <string.h>
// DFR0464 wire protocol and RGB register maps reviewed against DFRobot's
// MIT-licensed driver: https://github.com/DFRobot/DFRobot_RGBLCD1602
// This bounded state machine keeps LCD waits out of BLE/input processing.
class DfrLcd {
    uint8_t phase = 0, probeIndex = 0, rgb = 0, initIndex = 0, colourIndex = 0;
    uint32_t deadline = 0, lastIo = 0, lastHealth = 0, colourSent = 0xFFFFFFFF, colourWorking = 0;
    bool online = false;
    uint32_t textWrittenAt=0;
    int8_t cursor = -1; // Next visible cell at the LCD, invalid at row boundaries.
    char written[32] = {};
    bool probe(uint8_t address) {
        Wire.beginTransmission(address); return Wire.endTransmission() == 0;
    }
    bool send(uint8_t address, uint8_t reg, uint8_t value, uint32_t now) {
        Wire.beginTransmission(address); Wire.write(reg); Wire.write(value);
        if (Wire.endTransmission() == 0) return true;
        online = false; phase = 0; deadline = now + 3000; cursor = -1;
        Serial.println("LCD I2C error; retry in 3 seconds (MIDI remains active)");
        return false;
    }
public:
    void begin(uint8_t sda, uint8_t scl) {
        Wire.begin(sda, scl, 100000); Wire.setTimeOut(4);
        phase = 0; deadline = millis() + 50;
    }
    bool present() const { return online; }
    bool matches(const char rows[2][17]) const {
        return online && !memcmp(written,rows[0],16) && !memcmp(written+16,rows[1],16);
    }
    uint32_t lastTextWriteAt() const { return textWrittenAt; }
    void tick(uint32_t now, const char rows[2][17], uint8_t r, uint8_t g, uint8_t b) {
        if (int32_t(now-deadline) < 0 || uint32_t(now-lastIo) < 1) return;
        lastIo = now;
        if (phase == 0) {
            if (!probe(0x3E)) { deadline = now+3000; return; }
            phase = 1; deadline = now+50; rgb = probeIndex = initIndex = 0;
            colourSent = 0xFFFFFFFF; cursor = -1; memset(written,0,sizeof written);
            Serial.println("LCD found at 0x3E; initializing"); return;
        }
        if (phase <= 6) {
            static const uint8_t commands[] = {0x28,0x28,0x28,0x0C,0x01,0x06};
            uint8_t step = phase-1;
            if (send(0x3E,0x80,commands[step],now)) { ++phase; deadline=now+(step==4 ? 3 : 5); }
            return;
        }
        if (phase == 7) {
            static const uint8_t addresses[] = {0x60,0x6B,0x30,0x2D};
            const uint8_t address = addresses[probeIndex++];
            if (probe(address)) { rgb=address; phase=8; Serial.printf("LCD backlight at 0x%02X\n",rgb); }
            else if (probeIndex == 4) { phase=9; online=true; Serial.println("LCD ready; no supported RGB address found"); }
            return;
        }
        if (phase == 8) {
            static const uint8_t oldMap[][2] = {{0,0},{8,255},{1,0x20}};
            static const uint8_t newMap[][2] = {{0x2F,0},{0,0x20},{1,0},{2,1},{3,4},{7,255}};
            static const uint8_t halfMap[][2] = {{1,0},{2,255},{4,0x15}};
            const uint8_t (*map)[2] = rgb==0x60 ? oldMap : rgb==0x6B ? newMap : halfMap;
            const uint8_t count = rgb==0x6B ? 6 : rgb==0x2D ? 0 : 3;
            if (initIndex == count) { phase=9; online=true; return; }
            if (send(rgb,map[initIndex][0],map[initIndex][1],now)) ++initIndex;
            return;
        }
        if (uint32_t(now-lastHealth) >= 1000) {
            lastHealth=now;
            if (!probe(0x3E)) {
                online=false;phase=0;deadline=now+3000;cursor=-1;colourIndex=0;
                Serial.println("LCD disconnected; retry in 3 seconds");
            }
            return;
        }
        const uint32_t colour = (uint32_t(r)<<16)|(uint32_t(g)<<8)|b;
        if (rgb && (colour != colourSent || colourIndex)) {
            if (!colourIndex) colourWorking = colour;
            static const uint8_t oldReg[] = {4,3,2}, newReg[] = {6,5,4}, halfReg[] = {6,7,8}, otherReg[] = {1,2,3};
            const uint8_t *registers = rgb==0x60 ? oldReg : rgb==0x6B ? newReg : rgb==0x30 ? halfReg : otherReg;
            uint8_t value = (colourWorking >> (16-colourIndex*8)) & 255;
            if (rgb==0x30) value = uint16_t(value)*192/255;
            if (send(rgb,registers[colourIndex],value,now) && ++colourIndex==3) { colourSent=colourWorking; colourIndex=0; }
            if (!phase) colourIndex=0;
            return;
        }
        // Entry mode 0x06 advances the LCD cursor. Keep it between writes so
        // adjacent dirty cells need only data, not another set-cursor command.
        // One transaction per tick keeps encoder/BLE processing responsive.
        for (uint8_t i=0; i<32; ++i) {
            const char c = rows[i/16][i%16];
            if (written[i] == c) continue;
            if (cursor != int8_t(i)) {
                if (send(0x3E,0x80,uint8_t((i<16?0x80:0xC0)+(i%16)),now)) cursor=i;
            } else {
                // Up to four adjacent changed cells in one bounded transfer.
                // At 100kHz each data byte takes ~90us on the bus; the official
                // driver also relies on the LCD auto-increment entry mode.
                char data[4];uint8_t count=0;
                while(count<4 && (i%16)+count<16 && written[i+count]!=rows[i/16][i%16+count]) {
                    data[count]=rows[i/16][i%16+count];++count;
                }
                Wire.beginTransmission(0x3E);Wire.write(0x40);
                for(uint8_t n=0;n<count;++n) Wire.write(uint8_t(data[n]));
                if(Wire.endTransmission()==0) {
                    memcpy(written+i,data,count);textWrittenAt=millis();
                    cursor=((i%16)+count==16)?-1:int8_t(i+count);
                } else {
                    online=false;phase=0;deadline=now+3000;cursor=-1;
                    Serial.println("LCD text I2C error; retry in 3 seconds");
                }
            }
            return;
        }
    }
};
