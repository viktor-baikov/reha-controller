#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "Session.hpp"
namespace remote {
// Full quadrature cycles, with reversible contact bounce and invalid-edge rejection.
struct Quadrature {
    uint8_t previous = 3;
    int8_t partial = 0;
    uint8_t edgesPerDetent = 4;
    void reset(uint8_t ab) { previous = ab & 3; partial = 0; }
    int8_t sample(uint8_t ab) {
        static const int8_t transitions[16] = {0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0};
        ab &= 3;
        if ((previous ^ ab) == 3) partial = 0;
        else partial += transitions[(previous << 2) | ab];
        previous = ab;
        if (partial >= edgesPerDetent) { partial = 0; return 1; }
        if (partial <= -int(edgesPerDetent)) { partial = 0; return -1; }
        return 0;
    }
};
struct PlaybackLevel {
    bool known = false, muteKnown = false, muted = false;
    uint8_t level = 0;
    int16_t queued = 0;
    void reset() { known = muteKnown = muted = false; queued = 0; }
    void feedback(uint8_t cc, uint8_t value) {
        if (cc == 20) { level = value; known = true; }
        if (cc == 22 && (value == 0 || value == 127)) { muted = value == 127; muteKnown = true; }
    }
    void turn(int steps) { int n = queued + steps * 2; queued = n < -63 ? -63 : n > 63 ? 63 : n; }
    uint8_t take() { uint8_t v = uint8_t(64 + queued); queued = 0; return v; }
};
enum class MenuPage : uint8_t { Home, Browse, Confirm, Brightness, LcdTest };
enum class MenuItem : uint8_t { Restart, StartRehearsal, StopRehearsal, RemoveTake, Brightness, LcdTest };
struct PanelMenu {
    MenuPage page = MenuPage::Home;
    MenuItem item = MenuItem::Restart;
    int take=-1, confirmedTake=-1;
    uint32_t confirmedReference=0;
    char confirmedIdentity[33]={}, label[65]={};
    void home() { page = MenuPage::Home;confirmedTake=-1;confirmedReference=0;confirmedIdentity[0]=0; }
    void setTake(int value) {
        take=value;
        if(page==MenuPage::Browse && item==MenuItem::RemoveTake && take<=0) item=MenuItem::StopRehearsal;
    }
    bool turn(int steps, Action &) {
        if(page!=MenuPage::Browse) return false;
        const int count=take>0?5:4;
        int index=item==MenuItem::Restart?0:item==MenuItem::StopRehearsal?1:item==MenuItem::RemoveTake?2:item==MenuItem::Brightness?count-2:count-1;
        index=((index+steps)%count+count)%count;
        item=index==0?MenuItem::Restart:index==1?MenuItem::StopRehearsal:(take>0 && index==2)?MenuItem::RemoveTake :index==count-2?MenuItem::Brightness:MenuItem::LcdTest;
        return false;
    }
    bool click(Action &action) {
        if(page==MenuPage::Home) {page=MenuPage::Browse;item=MenuItem::Restart;return false;}
        return false;
    }
    void confirm(const char *identity) {
        page=MenuPage::Confirm;confirmedTake=take;
        strncpy(confirmedIdentity,identity,32);confirmedIdentity[32]=0;
    }
    void back() { if(page==MenuPage::Confirm || page==MenuPage::Brightness || page==MenuPage::LcdTest) page=MenuPage::Browse;else home(); }
    const char *name() {
        if(item==MenuItem::LcdTest) return "LCD timing test";
        if(item==MenuItem::Brightness) return "Brightness";
        if(item==MenuItem::Restart) return "Restart take";
        if(item==MenuItem::StartRehearsal) return "Start rehearsal";
        if(item==MenuItem::StopRehearsal) return "Stop rehearsal";
        snprintf(label,sizeof label,"Remove %02d take",take);return label;
    }
};
// A held mode never alters the menu underneath it. BACK cancels held modes
// until both buttons are released; two held buttons never issue navigation.
enum class HeldMode : uint8_t { None, Take, Song, Conflict };
struct HeldSelection {
    bool cancelled = false;
    void cancel() { cancelled = true; }
    HeldMode update(bool take, bool song) {
        if (!take && !song) cancelled = false;
        if (cancelled) return HeldMode::None;
        if (take && song) return HeldMode::Conflict;
        return take ? HeldMode::Take : song ? HeldMode::Song : HeldMode::None;
    }
};
inline bool recordLamp(const Session &session, uint32_t now) {
    return session.fresh(now) && (session.state == State::Recording ||
        (session.state == State::Saving && (now / 200) % 2));
}
// Static fixed-width rows: truncate long names, pad short names, printable ASCII.
inline void lcdRow(char out[17], const char *source) {
    bool ended = false;
    for (uint8_t i = 0; i < 16; ++i) {
        const char c = ended ? ' ' : source[i];
        if (!c) ended = true;
        out[i] = ended ? ' ' : c >= 32 && c <= 126 ? c : '?';
    }
    out[16] = 0;
}
}
