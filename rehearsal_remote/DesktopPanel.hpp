#pragma once
#include "Panel.hpp"
#include "PanelDisplay.hpp"
#include "TextRows.hpp"
#include "Browse.hpp"
#include "DfrLcd.hpp"
class PanelEncoder {
    uint8_t a, b;
    remote::Quadrature decoder;
    volatile int16_t pending = 0;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    static void changed(void *arg) {
        auto &self = *static_cast<PanelEncoder *>(arg);
        portENTER_CRITICAL_ISR(&self.mux);
        int count=self.pending+self.decoder.sample((digitalRead(self.a)<<1)|digitalRead(self.b));
        self.pending=count < -32 ? -32 : count > 32 ? 32 : count;
        portEXIT_CRITICAL_ISR(&self.mux);
    }
public:
    PanelEncoder(uint8_t aPin,uint8_t bPin): a(aPin),b(bPin) {}
    void begin() {
        pinMode(a,INPUT_PULLUP);pinMode(b,INPUT_PULLUP);
        decoder.edgesPerDetent=REHA_ENCODER_EDGES;
        decoder.reset((digitalRead(a)<<1)|digitalRead(b));
        attachInterruptArg(a,changed,this,CHANGE);attachInterruptArg(b,changed,this,CHANGE);
    }
    int take() {portENTER_CRITICAL(&mux);int result=pending;pending=0;portEXIT_CRITICAL(&mux);return result;}
};
// Uses the existing serialized command/token and CC send functions in the sketch.
bool press(remote::Action action);
void release(remote::Action action);
void sendRelativeVolume(uint8_t value);
void requestBrowseCatalog();
uint8_t loadPanelBrightness();
bool savePanelBrightness(uint8_t value);
void lcdTestReport(uint8_t stage,uint32_t eventAt,uint32_t writtenAt,uint32_t elapsed);
void lcdTestMarker(uint8_t stage); // 0=off/reset, 1=green/run, 2=red/stop
struct DesktopPanel {
    PanelEncoder encoder {REHA_LEVEL_A,REHA_LEVEL_B};
    Button encoderPush {REHA_LEVEL_PUSH}, takePush {REHA_TAKE_PIN}, songPush {REHA_SONG_PIN}, backPush {REHA_PANEL_BACK_PIN};
    bool hostWasFresh=false,catalogRefreshNeeded=false;
    int lastSteps=0; // Consumed detents for non-ISR diagnostics.
    uint8_t brightness=10,brightnessEdit=10;
    uint8_t testStage=0;
    uint32_t testStarted=0,testElapsed=0,testFrameAt=0,testEventAt=0;
    bool testWritePending=false;
    void testEvent(uint32_t now,bool advance) {
        if(advance) {
            if(testStage==0){testStage=1;testStarted=now;testElapsed=0;}
            else if(testStage==1){testStage=2;testElapsed=uint32_t(now-testStarted);}
            else {testStage=0;testElapsed=0;}
        } else {testStage=0;testElapsed=0;}
        // Optical reference is set before formatting/writing the target screen.
        lcdTestMarker(testStage);testEventAt=millis();testFrameAt=now;testWritePending=true;
    }
    remote::PanelMenu menu;
    remote::HeldSelection selection;
    remote::BrowseCatalog catalog;
    remote::BrowsePreview preview;
    remote::HoldFilter takeFilter,songFilter;
    remote::HeldMode mode=remote::HeldMode::None;
    remote::PlaybackLevel level;
    DfrLcd lcd;
    remote::PanelError notice=remote::PanelError::None;
    uint32_t noticeAt=0,lastLevel=0,seenResultSequence=0,lastCatalogRequest=0;
    char rows[2][17] = {"Starting...     ","Reha controller "};
    void begin() {
        brightness=loadPanelBrightness();if(brightness>100)brightness=10;
        // The base pull-down holds the driver off during reset. Configure the
        // output before Arduino digitalWrite (required by ESP32 core 3.x).
        // Never initialize GPIO9 as output.
        pinMode(REHA_PANEL_RECORD_LED,OUTPUT);digitalWrite(REHA_PANEL_RECORD_LED,LOW);
        encoder.begin();encoderPush.begin();takePush.begin();songPush.begin();backPush.begin();
        lcd.begin(REHA_SDA,REHA_SCL);
    }
    void reset() {
        menu.home();level.reset();encoder.take();selection.cancel();mode=remote::HeldMode::None;
        testWritePending=false;hostWasFresh=false;catalogRefreshNeeded=false;notice=remote::PanelError::None;catalog.reset();preview.cancel();takeFilter.reset();songFilter.reset();
        digitalWrite(REHA_PANEL_RECORD_LED,LOW);
    }
    void feedback(uint8_t cc,uint8_t value,uint32_t now) { level.feedback(cc,value);catalog.cc(cc,value,now); }
    void tell(remote::PanelError code,uint32_t now) {notice=code;noticeAt=now;}
    void action(remote::Action a,remote::Session &session,uint32_t now) {
        if(session.ignoresPress(a)) return;
        if(menu.page==remote::MenuPage::Confirm) menu.home();
        if(press(a)) {notice=remote::PanelError::None;release(a);}
        else tell(remote::PanelError::ActionUnavailable,now);
    }
    void update(remote::Session &session,const remote::TextRows &text,uint32_t now) {
        const bool fresh=session.fresh(now);
        if(hostWasFresh && !fresh)reset();
        hostWasFresh=fresh;
        if(fresh && (session.extensions&1) && !catalog.valid && uint32_t(now-catalog.activity)>=2500 && uint32_t(now-lastCatalogRequest)>=2500){
            lastCatalogRequest=now;requestBrowseCatalog();
        }
        const auto takeEdge=takePush.update(),songEdge=songPush.update();
        const auto backEdge=backPush.update(),pushEdge=encoderPush.update();
        const bool takeWas=takeFilter.held,songWas=songFilter.held;
        const bool takeHeld=takeFilter.update(takeEdge==Button::Falling || takeEdge==Button::Pressed,now);
        const bool songHeld=songFilter.update(songEdge==Button::Falling || songEdge==Button::Pressed,now);
        if(backEdge==Button::Falling) {
            if(fresh && session.state==remote::State::Idle) menu.home();else menu.back();
            selection.cancel();preview.cancel();notice=remote::PanelError::None;
        }
        const auto previousMode=mode;
        if(menu.page==remote::MenuPage::LcdTest)selection.cancel();
        mode=selection.update(takeHeld,songHeld);
        int steps=encoder.take()*REHA_LEVEL_DIRECTION;lastSteps=steps;
        // Discard movement spanning a button/mode transition, never reinterpret
        // an old volume turn as song selection or replay it after reconnect.
        const bool takeChanged=takeWas!=takeHeld;
        const bool songChanged=songWas!=songHeld;
        const bool pressed=!session.hostBlocked && !session.resyncWaiting && pushEdge==Button::Falling && backEdge!=Button::Falling;
        if(session.hostBlocked || session.resyncWaiting || mode!=previousMode || takeChanged || songChanged || backEdge==Button::Falling || pressed) steps=0;
        menu.setTake(remote::selectedTake(text.rows[0]));
        const bool idle=fresh && !session.hostBlocked && !session.resyncWaiting && session.state==remote::State::Idle;
        const bool confirming=menu.page==remote::MenuPage::Confirm;
        const bool selectionChanged=confirming && menu.item==remote::MenuItem::RemoveTake &&
            (menu.take!=menu.confirmedTake || strcmp(menu.confirmedIdentity,text.rows[0]) ||
             !session.selection.valid || session.selection.value!=menu.confirmedReference);
        if(selectionChanged) {
            menu.home();tell(remote::PanelError::ActionUnavailable,now);
        }
        const bool navigationReady=fresh && !session.hostBlocked && !session.resyncWaiting && (session.state==remote::State::Ready || session.state==remote::State::Playing);
        // A recording can add a take while the button is held. Do not reuse
        // pre-recording choices before the host publishes a complete new list.
        if(fresh && (session.state==remote::State::Recording || session.state==remote::State::Saving)) catalogRefreshNeeded=true;
        if(navigationReady && catalogRefreshNeeded && (session.extensions&1)) {
            catalogRefreshNeeded=false;catalog.valid=false;catalog.staging=false;
            lastCatalogRequest=now;requestBrowseCatalog();
        }
        if(!fresh || (!idle && session.state!=remote::State::Ready && session.state!=remote::State::Playing)) menu.home();
        if(menu.page==remote::MenuPage::Confirm && !idle && session.state!=remote::State::Ready &&
            !(menu.item==remote::MenuItem::Restart && session.state==remote::State::Playing)) menu.home();
        if(idle && menu.page!=remote::MenuPage::Home && menu.item!=remote::MenuItem::StartRehearsal) menu.home();
        if(seenResultSequence!=session.resultSequence) {
            seenResultSequence=session.resultSequence;
            const auto error=remote::resultPanelError(session.result,session.commandAction);
            if(error!=remote::PanelError::None) tell(error,now);
            else if(session.result==remote::Result::None || session.result==remote::Result::Accepted || session.result==remote::Result::Busy) notice=remote::PanelError::None;
        }
        if(confirming) { steps=0; }
        const bool heldMode=mode==remote::HeldMode::Take || mode==remote::HeldMode::Song;
        const bool wasHeld=previousMode==remote::HeldMode::Take || previousMode==remote::HeldMode::Song;
        if(!fresh || !navigationReady || !catalog.valid || mode==remote::HeldMode::Conflict || selection.cancelled) preview.cancel();
        if(wasHeld && mode==remote::HeldMode::None && !takeHeld && !songHeld && !selection.cancelled) {
            if(preview.committable(catalog) && navigationReady && !session.pending && !session.awaitingState){
                session.browseGeneration=preview.generation;session.browseIndex=preview.index;
                action(preview.song?remote::Action::SelectSong:remote::Action::SelectTake,session,now);
            }
            preview.cancel();steps=0;
        }
        if(!confirming && heldMode) {
            if(navigationReady && (session.extensions&1) && !session.pending && !session.awaitingState){
                if(!preview.active || preview.generation!=catalog.generation) {
                    preview.begin(mode==remote::HeldMode::Song,catalog);
                    // Turns made while loading must not land on the new list.
                    steps=0;
                }
                if(steps)preview.turn(steps,catalog);
            }
        } else if(mode!=remote::HeldMode::Conflict && !selection.cancelled) {
            if(pressed && !selectionChanged && !takeHeld && !songHeld && (navigationReady || idle) && !session.pending && !session.awaitingState) {
                notice=remote::PanelError::None;
                if(idle) {
                    menu.home();action(remote::Action::StartRehearsal,session,now);
                } else if(menu.page==remote::MenuPage::Confirm) {
                    const auto item=menu.item;
                    // Deletion is tied to the displayed selection captured before
                    // confirmation, never silently retargeted by new metadata.
                    const bool valid=item!=remote::MenuItem::RemoveTake ||
                        (menu.take>0 && menu.take==menu.confirmedTake && !strcmp(menu.confirmedIdentity,text.rows[0]) &&
                         session.selection.valid && session.selection.value==menu.confirmedReference);
                    if(item==remote::MenuItem::RemoveTake) session.removalTarget=menu.confirmedReference;
                    menu.home();
                    if(!valid) tell(remote::PanelError::ActionUnavailable,now);
                    else if(item==remote::MenuItem::Restart) {menu.page=remote::MenuPage::Browse;action(remote::Action::Restart,session,now);}
                    else if(item==remote::MenuItem::StartRehearsal) tell(remote::PanelError::StartUnsupported,now);
                    else if(!(session.capabilities&(item==remote::MenuItem::RemoveTake?64:32))) tell(remote::PanelError::ExtensionUnsupported,now);
                    else action(item==remote::MenuItem::RemoveTake?remote::Action::RemoveTake:remote::Action::StopRehearsal,session,now);
                } else if(menu.page==remote::MenuPage::LcdTest) {
                    testEvent(now,true);
                } else if(menu.page==remote::MenuPage::Brightness) {
                    // Flash is written only on confirmation, never per detent.
                    if(brightnessEdit==brightness || savePanelBrightness(brightnessEdit)) {
                        brightness=brightnessEdit;menu.page=remote::MenuPage::Browse;
                    }
                } else if(menu.page==remote::MenuPage::Browse) {
                    if(menu.item==remote::MenuItem::LcdTest) {
                        menu.page=remote::MenuPage::LcdTest;testEvent(now,false);
                    } else if(menu.item==remote::MenuItem::Brightness) {
                        brightnessEdit=brightness;menu.page=remote::MenuPage::Brightness;
                    } else if(session.state==remote::State::Ready || menu.item==remote::MenuItem::Restart) {
                        if(menu.item!=remote::MenuItem::RemoveTake || session.selection.valid) {
                            menu.confirm(text.rows[0]);menu.confirmedReference=session.selection.value;
                        }
                    }
                    else tell(remote::PanelError::ActionUnavailable,now);
                } else {remote::Action a;if(menu.click(a)) action(a,session,now);}
            }
            if(steps && menu.page==remote::MenuPage::Brightness) {
                int value=int(brightnessEdit)+steps*5;
                brightnessEdit=value<0?0:value>100?100:value;
            } else if(steps && menu.page==remote::MenuPage::Browse) {
                remote::Action unused;menu.turn(steps,unused);
            } else if(steps && menu.page==remote::MenuPage::Home && navigationReady) {
                if(fresh && (session.capabilities&8)) level.turn(steps);
                else if(fresh) tell(remote::PanelError::VolumeUnsupported,now);
            }
        }
        if(!fresh || idle || menu.page==remote::MenuPage::Confirm) level.queued=0;
        if(level.queued && fresh && (session.capabilities&8) && uint32_t(now-lastLevel)>=50) {
            sendRelativeVolume(level.take()); lastLevel=now;
        }
        digitalWrite(REHA_PANEL_RECORD_LED,remote::recordLamp(session,now)?HIGH:LOW);
        char title[65];remote::homeTitle(title,text.rows[0]);
        remote::lcdRow(rows[0],title);
        // Notices only replace the bottom row; retain the user's context.
        if(heldMode) remote::lcdRow(rows[0],mode==remote::HeldMode::Take?"select take":"select song");
        else if(menu.page==remote::MenuPage::Confirm) remote::lcdRow(rows[0],"are you sure?");
        else if(menu.page!=remote::MenuPage::Home) remote::lcdRow(rows[0],menu.name());
        auto error=remote::persistentPanelError(session,now);
        if(error==remote::PanelError::None && mode==remote::HeldMode::Conflict) error=remote::PanelError::ConflictingKeys;
        if(error==remote::PanelError::None && !(heldMode && !catalog.valid) && notice!=remote::PanelError::None && uint32_t(now-noticeAt)<2000) error=notice;
        if(error==remote::PanelError::Bluetooth) {
            remote::lcdRow(rows[0],"Bluetooth MIDI");remote::lcdRow(rows[1],"Disconnected");
        } else if(error==remote::PanelError::Feedback) {
            remote::lcdRow(rows[0],"Bluetooth MIDI");remote::lcdRow(rows[1],"Waiting recorder");
        } else if(error!=remote::PanelError::None) remote::errorRow(rows[1],error);
        else if(menu.page==remote::MenuPage::Confirm) {
            remote::lcdRow(rows[0],"are you sure?");remote::lcdRow(rows[1],"");
        } else if(idle) {
            remote::lcdRow(rows[0],"connected");remote::lcdRow(rows[1],"start rehersal?");
        } else if(mode==remote::HeldMode::Take || mode==remote::HeldMode::Song) {
            char selected[65];
            if(catalog.valid && preview.active && preview.generation==catalog.generation)strcpy(selected,preview.label);
            else strcpy(selected,(session.extensions&1)?"Updating list":"Update recorder");
            remote::lcdRow(rows[0],mode==remote::HeldMode::Take?"select take":"select song");
            remote::lcdRow(rows[1],selected);
        } else if(menu.page==remote::MenuPage::LcdTest) {
            remote::lcdRow(rows[0],testStage==1?"LCD timer RUN":testStage==2?"LCD timer STOP":"LCD timer ZERO");
            if(testStage==1 && uint32_t(now-testFrameAt)>=10) {
                testFrameAt=now;testElapsed=uint32_t(now-testStarted);
            }
            char timer[17];snprintf(timer,sizeof timer,"%010lu ms",(unsigned long)testElapsed);
            remote::lcdRow(rows[1],timer);
        } else if(menu.page==remote::MenuPage::Brightness) {
            remote::lcdRow(rows[0],"Brightness");
            char value[17];snprintf(value,sizeof value,"%u%%  Press save",unsigned(brightnessEdit));
            remote::lcdRow(rows[1],value);
        } else if(menu.page!=remote::MenuPage::Home) {
            remote::lcdRow(rows[0],menu.name());
            remote::lcdRow(rows[1],menu.item==remote::MenuItem::Restart?"Press to restart":menu.item==remote::MenuItem::StopRehearsal?"Press to stop":menu.item==remote::MenuItem::RemoveTake?"Press to remove":menu.item==remote::MenuItem::Brightness?"Press to adjust":menu.item==remote::MenuItem::LcdTest?"Press to test":"Press to start");
        } else remote::statusRow(rows[1],remote::requestedStatus(session),level,session.pending || session.awaitingState || session.hostBlocked);
        // Menu/idle pending uses top-right *. Held headings remain static.
        // Home appends * to status, never overwriting the song.
        if(error==remote::PanelError::None && !heldMode && (session.pending || session.awaitingState) &&
            (idle || menu.page!=remote::MenuPage::Home)) rows[0][15]='*';
        uint8_t r=0,g=0,b=0;
        if(!fresh) {r=24;g=10;}
        else if(session.state==remote::State::Recording) r=48;
        else if(session.state==remote::State::Playing) {g=32;b=24;}
        else if(session.state==remote::State::Ready || idle) {r=g=b=24;}
        else if(session.state==remote::State::Saving) {r=32;g=16;}
        else r=32;
        // Scale all status colours to the chosen peak PWM, preserving their hue.
        const uint8_t peak=r>g?(r>b?r:b):(g>b?g:b);
        const uint8_t percent=menu.page==remote::MenuPage::Brightness?brightnessEdit:brightness;
        const uint16_t pwm=(uint16_t(percent)*255+50)/100;
        if(peak) {r=(uint16_t(r)*pwm+peak/2)/peak;g=(uint16_t(g)*pwm+peak/2)/peak;b=(uint16_t(b)*pwm+peak/2)/peak;}
        lcd.tick(now,rows,r,g,b);
        if(menu.page!=remote::MenuPage::LcdTest) testWritePending=false;
        // Running content changes continuously: report only stable STOP/ZERO
        // targets, after all 32 character cells have been acknowledged on I2C.
        if(testWritePending && testStage!=1 && error==remote::PanelError::None && lcd.matches(rows)) {
            lcdTestReport(testStage,testEventAt,lcd.lastTextWriteAt(),testElapsed);
            testWritePending=false;
        }
    }
};
