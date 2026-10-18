#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
// Event-only UART trace. Retain the last 32 events even when live output is
// disabled/full. Never call from an ISR; skip a live write rather than wait.
struct ControllerDiagnostics {
    static constexpr uint8_t Capacity=32;
    static constexpr unsigned Width=160;
    char records[Capacity][Width] = {};
    uint8_t next=0,count=0;
    uint32_t liveDropped=0;
    bool live=true;
    void event(uint32_t now,const char *name,const char *format="",...) {
        char detail[108];va_list args;va_start(args,format);
        vsnprintf(detail,sizeof detail,format,args);va_end(args);
        char *line=records[next];
        snprintf(line,Width,"LOG t=%lu event=%s %s",(unsigned long)now,name,detail);
        next=(next+1)%Capacity;if(count<Capacity) ++count;
        if(live) {
            if(Serial.availableForWrite()>=int(strlen(line)+2)) Serial.println(line);
            else ++liveDropped;
        }
    }
    void dump() const {
        Serial.printf("TRACE count=%u live_dropped=%lu\n",count,(unsigned long)liveDropped);
        const uint8_t first=(next+Capacity-count)%Capacity;
        for(uint8_t i=0;i<count;++i) Serial.println(records[(first+i)%Capacity]);
    }
};
