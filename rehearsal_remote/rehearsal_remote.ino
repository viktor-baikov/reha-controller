#include <Control_Surface.h>
#include "Session.hpp"
#include "BoardConfig.hpp"
#include "Volume.hpp"
#include "TextRows.hpp"
#include "Diagnostics.hpp"
ControllerDiagnostics diagnostics;
#ifdef REHA_DESKTOP_PANEL
#include <Preferences.h>
#include "DesktopPanel.hpp"
void lcdTestReport(uint8_t stage,uint32_t eventAt,uint32_t writtenAt,uint32_t elapsed) {
    diagnostics.event(millis(),"lcd_test","stage=%u event_ms=%lu final_write_ms=%lu write_delay_ms=%lu timer_ms=%lu",
        stage,(unsigned long)eventAt,(unsigned long)writtenAt,(unsigned long)uint32_t(writtenAt-eventAt),(unsigned long)elapsed);
    Serial.printf("LCD_TEST stage=%u event_ms=%lu final_write_ms=%lu write_delay_ms=%lu timer_ms=%lu\n",
        stage,(unsigned long)eventAt,(unsigned long)writtenAt,(unsigned long)uint32_t(writtenAt-eventAt),(unsigned long)elapsed);
}
void lcdTestMarker(uint8_t stage) {
    rgbLedWrite(RGB_BUILTIN,stage==2?48:0,stage==1?48:0,0);
}
uint8_t loadPanelBrightness() {
    Preferences settings;
    if(!settings.begin("reha-panel",true))return 10;
    uint8_t value=settings.getUChar("brightness",10);settings.end();
    return value<=100?value:10;
}
bool savePanelBrightness(uint8_t value) {
    Preferences settings;
    if(!settings.begin("reha-panel",false))return false;
    bool saved=settings.putUChar("brightness",value)==1;settings.end();
    if(!saved)Serial.println("Brightness save failed");
    return saved;
}
#endif

BluetoothMIDI_Interface midi;
remote::Session session;
remote::VolumePickup volume;
remote::TextRows text;
#ifdef REHA_DESKTOP_PANEL
DesktopPanel desktop;
#endif
Button recordButton {REHA_RECORD_PIN};
#ifdef REHA_PLAY_PIN
Button playButton {REHA_PLAY_PIN}; // optional momentary switch to ground
#endif
#ifdef REHA_NAV_BUTTONS
Button backButton {REHA_BACK_PIN}, forwardButton {REHA_FORWARD_PIN}, shiftButton {REHA_SHIFT_PIN};
bool shiftHeld = false, shiftUsed = false;
uint32_t shiftAt = 0;
#endif
bool recordSent = false, playSent = false;

void control(uint8_t number, uint8_t value) {
    midi.sendControlChange({number, Channel_16}, value);
}
void sendRelativeVolume(uint8_t value) {
    diagnostics.event(millis(),"volume","relative=%d",int(value)-64);
    control(21,value); midi.sendNow();
}
void requestBrowseCatalog(){control(89,1);midi.sendNow();diagnostics.event(millis(),"browse_request","missing=1");}
bool press(remote::Action action) {
    if(session.ignoresPress(action)) {
        diagnostics.event(millis(),"ignored","note=%u reason=%s",uint8_t(action),
            session.pending || session.awaitingState ? "pending" : "play_during_record_or_save");
        return false; // Silent on LCD; diagnostics only.
    }
    if(action==remote::Action::RemoveTake && (!session.selection.valid || !session.removalTarget || session.removalTarget!=session.selection.value)) {
        diagnostics.event(millis(),"blocked","note=73 reason=selection_changed");return false;
    }
    if (!session.begin(action, millis())) {
        diagnostics.event(millis(),"blocked","note=%u state=%u fresh=%u caps=%u",uint8_t(action),uint8_t(session.state),session.fresh(millis()),session.capabilities);
#ifdef REHA_DESKTOP_PANEL
        desktop.tell(remote::PanelError::ActionUnavailable,millis());
#endif
        return false;
    }
#ifdef REHA_DESKTOP_PANEL
    if(desktop.menu.page==remote::MenuPage::Confirm) desktop.menu.home();
#endif
    if(action==remote::Action::RemoveTake) {
        uint32_t tag=session.removalTarget;uint8_t checksum=0;
        for(uint8_t i=0;i<5;++i) {uint8_t part=(tag>>(7*i))&127;control(104+i,part);checksum=(checksum+part)&127;}
        control(109,checksum);session.removalTarget=0;
    }
    if(action==remote::Action::SelectTake || action==remote::Action::SelectSong){
        uint8_t kind=action==remote::Action::SelectTake?4:5, sum=kind+7;
        control(90,kind);control(91,7);
        for(uint8_t i=0;i<5;++i){uint8_t part=(session.browseGeneration>>(7*i))&127;sum+=part;control(92,part);}
        for(uint8_t i=0;i<2;++i){uint8_t part=(session.browseIndex>>(7*i))&127;sum+=part;control(92,part);}
        control(93,sum&127);
    }
    control(remote::TokenCC, session.token);
    midi.sendNoteOn({uint8_t(action), Channel_16}, 127);
    midi.sendNow();
    diagnostics.event(millis(),"command","note=%u token=%u from=%u",uint8_t(action),session.token,uint8_t(session.commandFrom));
    return true;
}
void release(remote::Action action) {
    if (session.connected) {
        midi.sendNoteOff({uint8_t(action), Channel_16}, 0);
        midi.sendNow();
    }
}

struct Callbacks : MIDI_Callbacks {
    void onChannelMessage(MIDI_Interface &, ChannelMessage message) override {
        if (message.getMessageType() != MIDIMessageType::ControlChange ||
            (message.header & 15) != 15) return;
        session.cc(message.data1, message.data2, millis());
        if (session.versionKnown && message.data1 == 20) volume.feedback(message.data2);
#ifdef REHA_DESKTOP_PANEL
        if (session.versionKnown) desktop.feedback(message.data1,message.data2,millis());
#endif
        if (session.versionKnown && text.cc(message.data1, message.data2, millis()))
            diagnostics.event(millis(),"metadata","row0=[%s] row1=[%s]",text.rows[0],text.rows[1]);

    }
} callbacks;

void setup() {
    Serial.begin(115200);
    midi.setName("Reha Probe"); // BLE advertising identity
    midi.ble_settings.initiate_security = true;
    midi.requireEncryption(true);
    midi.setCallbacks(callbacks);
#ifdef REHA_RECORD_LED_PIN
    digitalWrite(REHA_RECORD_LED_PIN, LOW);
    pinMode(REHA_RECORD_LED_PIN, OUTPUT);
#endif
#ifdef REHA_PLAY_LED_PIN
    digitalWrite(REHA_PLAY_LED_PIN, LOW);
    pinMode(REHA_PLAY_LED_PIN, OUTPUT);
#endif
    recordButton.begin();
#ifdef REHA_PLAY_PIN
    playButton.begin();
#endif
#ifdef REHA_NAV_BUTTONS
    backButton.begin(); forwardButton.begin(); shiftButton.begin();
#endif
#ifdef REHA_DESKTOP_PANEL
    desktop.begin();
#endif
    midi.begin();
#ifdef REHA_DESKTOP_PANEL
    Serial.printf("Desktop panel Rev %s: Record=%u Play=%u Encoder=%u/%u/%u Take=%u Song=%u Back=%u LED=%u LCD=%u/%u\n",
        REHA_PANEL_WIRING_REV, REHA_RECORD_PIN, REHA_PLAY_PIN,
        REHA_LEVEL_A, REHA_LEVEL_B, REHA_LEVEL_PUSH, REHA_TAKE_PIN,
        REHA_SONG_PIN, REHA_PANEL_BACK_PIN, REHA_PANEL_RECORD_LED, REHA_SDA, REHA_SCL);
#else
    Serial.println("Reha remote protocol v1; BOOT=Record/Stop; serial r=Record/Stop, p=Play/Stop");
#endif
}

void loop() {
    const bool connected = midi.isConnected();
    if (connected != session.connected) {
        session.connection(connected);
        volume.reset(); text.reset();
#ifdef REHA_DESKTOP_PANEL
        desktop.reset();
#endif
        recordSent = playSent = false;
        diagnostics.event(millis(),"ble","connected=%u",connected);
    }
    midi.update();
    const uint32_t now = millis();
    session.tick(now); text.tick(now);
    static bool hostWasFresh=false;
    const bool hostFresh=session.fresh(now);
    if(hostWasFresh && !hostFresh){
        text.reset();volume.reset();
        diagnostics.event(now,"host_lost","waiting=1");
    }
    hostWasFresh=hostFresh;
    static unsigned lastSession=0xffffffff;static uint32_t resultSeq=0;
    const unsigned stateBits=unsigned(session.state)|(unsigned(session.fresh(now))<<4)|(unsigned(session.pending)<<5)|(unsigned(session.awaitingState)<<6)|(unsigned(session.capabilities)<<8);
    if(lastSession!=stateBits || resultSeq!=session.resultSequence) {
        lastSession=stateBits;resultSeq=session.resultSequence;
        diagnostics.event(now,"session","state=%u fresh=%u caps=%u token=%u pending=%u awaiting=%u result=%u ack=%u",uint8_t(session.state),session.fresh(now),session.capabilities,session.token,session.pending,session.awaitingState,uint8_t(session.result),session.ackToken);
    }
    const bool metadataReady=!(session.capabilities&4) || text.complete();
    if (session.helloDue(now,metadataReady)) {
        diagnostics.event(now,"hello","fresh=%u text=%u/%u",session.fresh(now),text.received[0],text.received[1]);
        control(remote::VersionCC, remote::Version);
        control(111, 15); // text, atomic selection reference, explicit idle/rehearsal actions
        midi.sendNow();
    }
    auto edge = recordButton.update();
#ifdef REHA_RECORD_NC
    const auto recordPress = Button::Rising, recordRelease = Button::Falling;
#else
    const auto recordPress = Button::Falling, recordRelease = Button::Rising;
#endif
    if (edge == recordPress) recordSent = press(remote::Action::RecordStop);
    if (edge == recordRelease && recordSent) {
        release(remote::Action::RecordStop); recordSent = false;
    }
#ifdef REHA_PLAY_PIN
    edge = playButton.update();
#ifdef REHA_PLAY_NC
    const auto playPress = Button::Rising, playRelease = Button::Falling;
#else
    const auto playPress = Button::Falling, playRelease = Button::Rising;
#endif
    if (edge == playPress) playSent = press(remote::Action::PlayStop);
    if (edge == playRelease && playSent) {
        release(remote::Action::PlayStop); playSent = false;
    }
#endif
#ifdef REHA_NAV_BUTTONS
    const auto shiftEdge = shiftButton.update();
    if (shiftEdge == Button::Falling) { shiftHeld = true; shiftUsed = false; shiftAt = now; }
    if (shiftEdge == Button::Rising) {
        if (!shiftUsed && uint32_t(now-shiftAt) < 800 && press(remote::Action::Restart)) release(remote::Action::Restart);
        shiftHeld = false;
    }
    const auto backEdge = backButton.update(), forwardEdge = forwardButton.update();
    if (backEdge == Button::Falling || forwardEdge == Button::Falling) {
        const bool forward = forwardEdge == Button::Falling;
        remote::Action action = forward ? remote::Action::SeekForward : remote::Action::SeekBack;
        if (shiftHeld) {
            shiftUsed = true;
            action = uint32_t(now-shiftAt) >= 800 ?
                (forward ? remote::Action::NextTake : remote::Action::PreviousTake) :
                (forward ? remote::Action::NextSong : remote::Action::PreviousSong);
        }
        if (press(action)) release(action);
    }
#endif
    if (Serial.available()) {
        const char command = Serial.read();
        if (command == 'p' && press(remote::Action::PlayStop)) release(remote::Action::PlayStop);
        if (command == 'r' && press(remote::Action::RecordStop)) release(remote::Action::RecordStop);
        remote::Action navigation = remote::Action::Restart;
        bool navigate = true;
        switch (command) {
            case 'b': navigation = remote::Action::SeekBack; break;
            case 'f': navigation = remote::Action::SeekForward; break;
            case 't': navigation = remote::Action::PreviousTake; break;
            case 'T': navigation = remote::Action::NextTake; break;
            case 's': navigation = remote::Action::PreviousSong; break;
            case 'S': navigation = remote::Action::NextSong; break;
            case 'h': break;
            default: navigate = false;
        }
        if (navigate && press(navigation)) release(navigation);
        if (command == 'm' && press(remote::Action::MuteToggle)) release(remote::Action::MuteToggle);
        if ((command == '+' || command == '-') && session.fresh(now) && (session.capabilities & 8))
            sendRelativeVolume(command == '+' ? 66 : 62);
#ifdef REHA_DESKTOP_PANEL
        if (command == 'i') {
            Serial.printf("STATUS t=%lu rev=%s ble=%u fresh=%u state=%u caps=%u token=%u pending=%u awaiting=%u result=%u hello=%u text=%u/%u ext=%u browse=%u tag=%lu songs=%u takes=%u mode=%u LCD=%s rows=[%s] [%s]\n",
                (unsigned long)now,REHA_PANEL_WIRING_REV,session.connected,session.fresh(now),uint8_t(session.state),session.capabilities,
                session.token,session.pending,session.awaitingState,uint8_t(session.result),session.helloSent,text.received[0],text.received[1],session.extensions,desktop.catalog.valid,(unsigned long)desktop.catalog.generation,desktop.catalog.songCount,desktop.catalog.takeCount,uint8_t(desktop.mode),desktop.lcd.present()?"online":"offline",desktop.rows[0],desktop.rows[1]);
        }
#endif
        if(command=='d') diagnostics.dump();
        if(command=='l') {diagnostics.live=!diagnostics.live;Serial.printf("TRACE live=%u\n",diagnostics.live);}
        if(command=='?') Serial.println("Read-only: i=status d=last32 l=toggle trace. Action commands: r/p/t/T/s/S/h/+/-/m/0-9.");
        // Diagnostic absolute volume; real panel ADC uses pickup below.
        if (command >= '0' && command <= '9' && session.fresh(now) && (session.capabilities & 2)) {
            control(20, (uint16_t(command - '0') * 127) / 9); midi.sendNow();
        }
    }
#ifdef REHA_DESKTOP_PANEL
    desktop.update(session,text,now);
    if(desktop.lastSteps) diagnostics.event(now,"encoder","detents=%d page=%u mode=%u",desktop.lastSteps,uint8_t(desktop.menu.page),uint8_t(desktop.mode));
    static char lastRows[2][17]={};static uint32_t lastRenderLog=0;
    if(uint32_t(now-lastRenderLog)>=100 && memcmp(lastRows,desktop.rows,sizeof lastRows)) {
        memcpy(lastRows,desktop.rows,sizeof lastRows);lastRenderLog=now;
        diagnostics.event(now,"screen","lcd=%u row0=[%s] row1=[%s]",desktop.lcd.present(),desktop.rows[0],desktop.rows[1]);
    }
    static uint32_t inputAt=0;static unsigned lastInputs=0xffff;
    if(uint32_t(now-inputAt)>=20) {
        inputAt=now;unsigned inputs=0;
        const uint8_t pins[]={REHA_RECORD_PIN,REHA_PLAY_PIN,REHA_TAKE_PIN,REHA_SONG_PIN,REHA_PANEL_BACK_PIN,REHA_LEVEL_PUSH};
        for(uint8_t i=0;i<6;++i) if(digitalRead(pins[i])) inputs|=1u<<i;
        if(inputs!=lastInputs) {lastInputs=inputs;diagnostics.event(now,"inputs","released_mask=0x%02X order=rec,play,take,song,back,sw",inputs);}
    }
#endif
#ifdef REHA_RECORD_LED_PIN
    digitalWrite(REHA_RECORD_LED_PIN, session.fresh(now) &&
        (session.state == remote::State::Recording ||
         (session.state == remote::State::Saving && (now / 200) % 2)));
#endif
#ifdef REHA_PLAY_LED_PIN
    digitalWrite(REHA_PLAY_LED_PIN, session.fresh(now) && session.state == remote::State::Playing);
#endif

#ifdef REHA_VOLUME_PIN
    static uint32_t lastVolume = 0;
    static uint32_t filtered = 0;
    static bool initialized = false;
    if (uint32_t(now - lastVolume) >= 50) {
        lastVolume = now;
        const uint32_t reading = analogRead(REHA_VOLUME_PIN);
        filtered = initialized ? (filtered * 3 + reading) / 4 : reading;
        initialized = true;
        if (session.fresh(now) && (session.capabilities & 2) && !session.pending && volume.sample((filtered * 127) / 4095)) {
            control(20, volume.lastSent); midi.sendNow();
        }
    }
#endif
    uint8_t r = 0, g = 0, b = 0;
    if (!session.connected) b = (now / 500) % 2 ? 12 : 0;
    else if (!session.fresh(now)) { r = 12; g = 5; }
    else switch (session.state) {
        case remote::State::Ready: g = 12; break;
        case remote::State::Idle: b = 12; break;
        case remote::State::Recording: r = 12; break;
        case remote::State::Playing: g = 8; b = 12; break;
        case remote::State::Saving: r = (now / 200) % 2 ? 12 : 0; g = r / 2; break;
        default: r = (now / 250) % 2 ? 12 : 0; break;
    }
    const uint32_t colour = (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
    static uint32_t previous = 0xFFFFFFFF;
#ifdef REHA_DESKTOP_PANEL
    if(desktop.menu.page==remote::MenuPage::LcdTest) {previous=0xFFFFFFFF;return;}
#endif
    if (colour != previous) {
        rgbLedWrite(RGB_BUILTIN, r, g, b);
        previous = colour;
    }
}
