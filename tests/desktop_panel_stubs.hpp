#pragma once
// Exercise the actual panel input/render integration with fake GPIO and MIDI.
#include <assert.h>
#include <vector>
#include <string.h>
#define REHA_DESKTOP_PANEL
#include "../rehearsal_remote/BoardConfig.hpp"
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 3
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE=int;
#define portENTER_CRITICAL_ISR(x) ((void)0)
#define portEXIT_CRITICAL_ISR(x) ((void)0)
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
int pins[32];
int pinModes[32];
void pinMode(int pin,int mode) {pinModes[pin]=mode;}
void digitalWrite(int p,int value) {
    // Match ESP32 core 3.x: writes require an already configured output.
    assert(pinModes[p]==OUTPUT);
    pins[p]=value;
}
int digitalRead(int p) {return pins[p];}
void (*callbacks[32])(void*);void *contexts[32];
void attachInterruptArg(int pin,void (*fn)(void*),void *context,int) {callbacks[pin]=fn;contexts[pin]=context;}
class Button {
    int pin;bool previous=false;
public:
    enum State {Pressed,Released,Falling,Rising};
    static bool down[32];
    Button(int p):pin(p){}
    void begin(){}
    State update() {bool value=down[pin];State result=value?(previous?Pressed:Falling):(previous?Rising:Released);previous=value;return result;}
};
bool Button::down[32]={};
#include "../rehearsal_remote/DesktopPanel.hpp"
TestSerial Serial;TestWire Wire;uint32_t clockNow=0;
uint32_t millis(){return clockNow;}
remote::Session session;remote::TextRows text;
std::vector<remote::Action> actions;std::vector<uint8_t> levels;
bool press(remote::Action a) {if(!session.begin(a,clockNow)) return false;actions.push_back(a);return true;}
void release(remote::Action){}
void sendRelativeVolume(uint8_t value){levels.push_back(value);}
unsigned catalogRequests=0;
void requestBrowseCatalog(){++catalogRequests;}
unsigned testReports=0;uint8_t reportedStage=0;uint32_t reportedElapsed=0;
void lcdTestReport(uint8_t stage,uint32_t,uint32_t,uint32_t elapsed){++testReports;reportedStage=stage;reportedElapsed=elapsed;}
uint8_t testLedStage=0;unsigned testLedEvents=0;
void lcdTestMarker(uint8_t stage){testLedStage=stage;++testLedEvents;}
uint8_t storedBrightness=10;unsigned brightnessWrites=0;bool brightnessSaveOk=true;
uint8_t loadPanelBrightness(){return storedBrightness;}
bool savePanelBrightness(uint8_t value){++brightnessWrites;if(!brightnessSaveOk)return false;storedBrightness=value;return true;}
void releaseHeld(DesktopPanel &p,int key){Button::down[key]=false;for(int i=0;i<3;++i){clockNow+=50;session.cc(118,1,clockNow);p.update(session,text,clockNow);}}
void update(DesktopPanel &p) {clockNow+=50;session.cc(118,1,clockNow);p.update(session,text,clockNow);}
void click(DesktopPanel &p,int pin) {Button::down[pin]=true;update(p);Button::down[pin]=false;update(p);}
void turn(DesktopPanel &p) {
    for(int ab: {1,0,2,3}) {
        pins[REHA_LEVEL_A]=(ab>>1)&1;pins[REHA_LEVEL_B]=ab&1;
        callbacks[REHA_LEVEL_A](contexts[REHA_LEVEL_A]);
    }
    update(p);
}
void ack() {session.cc(114,session.token,clockNow);session.cc(116,1,clockNow);}
