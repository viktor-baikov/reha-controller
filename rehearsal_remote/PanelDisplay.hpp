#pragma once
#include "Panel.hpp"
namespace remote {
enum class PanelError : uint8_t {
    None=0, Bluetooth=1, Feedback=2, Protocol=3, Host=4, Unavailable=5,
    Busy=6, ActionUnavailable=7, VolumeUnsupported=8, CommandFailed=9,
    Timeout=10, ConflictingKeys=11, StartUnsupported=12, ExtensionUnsupported=13
};
struct PanelErrorInfo { PanelError code;const char *meaning;const char *recovery; };
inline const PanelErrorInfo *panelErrorList(size_t &count) {
    static const PanelErrorInfo entries[] = {
        {PanelError::Bluetooth,"Bluetooth MIDI disconnected","Connect the controller to one MIDI host. USB provides power, not USB MIDI."},
        {PanelError::Feedback,"Recorder feedback missing or stale (diagnostic category)","The LCD shows Bluetooth MIDI / Waiting recorder, including after app closure. Feedback expires after 3.5 seconds. Reopen the recorder and check both MIDI ports; commands stay disabled until fresh feedback returns."},
        {PanelError::Protocol,"Unsupported protocol version","Use matching controller/recorder protocol versions. No command is sent until compatible."},
        {PanelError::Host,"Recorder reported ERROR","Inspect the recorder's actual error/log and preserve pending takes. CC119=4 carries no detailed cause; this code does not diagnose it."},
        {PanelError::Unavailable,"Recorder not ready","Recorder sends state 5. It indicates a pending take or library problem; an older host also uses it for no active rehearsal. A healthy new host sends explicit idle state 6. Inspect the laptop; the code alone cannot distinguish these."},
        {PanelError::Busy,"Command busy","The recorder rejected this request as Busy; it was not queued. Wait for its current work to finish before deliberately retrying. Additional local presses while waiting are silently ignored and never show this code. No automatic retry."},
        {PanelError::ActionUnavailable,"Requested action unavailable","Check recorder state and navigation support. Also covers a host Unavailable result. Stop recording/saving before navigation."},
        {PanelError::VolumeUnsupported,"Relative volume unsupported","Update the recorder to support relative-volume capability. No guessed absolute setting is sent."},
        {PanelError::CommandFailed,"Recorder command failed","Inspect recorder error/log; retain any unsaved take. Result Error carries no detailed cause."},
        {PanelError::Timeout,"Command confirmation timed out","Check both MIDI directions and recorder health. Legacy hosts time out after 1.5 seconds for acknowledgement or 3.5 seconds for state. With readiness support, the controller stays blocked and requests status every 1.5 seconds. Record/Play toggles are never replayed."},
        {PanelError::ConflictingKeys,"TAKE and SONG held together","Release one button, or BACK to cancel held modes until both are released."},
        {PanelError::StartUnsupported,"Start rehearsal unsupported in this context","Use the explicit connected idle start screen with a lifecycle-capable host. Check saved audio setup and matching lifecycle-capable recorder."},
        {PanelError::ExtensionUnsupported,"Recorder does not support this menu action","Stop rehearsal and Remove take require the new optional host capabilities. No command is sent to an older recorder. Removal also requires matching selection-reference support."}
    };
    count=sizeof entries/sizeof entries[0];return entries;
}
inline PanelError persistentPanelError(const Session &session,uint32_t now) {
    if(!session.connected) return PanelError::Bluetooth;
    if(session.protocolMismatch) return PanelError::Protocol;
    if(!session.fresh(now)) return PanelError::Feedback;
    if(session.state==State::Error) return PanelError::Host;
    if(session.state==State::Unavailable) return PanelError::Unavailable;
    return PanelError::None;
}
inline PanelError resultPanelError(Result result, Action action=Action::RecordStop) {
    switch(result) {
        case Result::Busy:
            // Selection turns are disposable while the host is working. A
            // Busy reply must not kick the still-held view back to home/error.
            if(action==Action::PreviousTake || action==Action::NextTake ||
               action==Action::PreviousSong || action==Action::NextSong || action==Action::SelectTake || action==Action::SelectSong) return PanelError::None;
            return PanelError::Busy;
        case Result::Unavailable:return PanelError::ActionUnavailable;
        case Result::Error:return PanelError::CommandFailed;
        case Result::Timeout:return PanelError::Timeout;
        default:return PanelError::None;
    }
}
// Existing text protocol: host row 0 is typically "T12 Song" or "T12 - Song".
// Compact only that identity separator. Preserve punctuation inside song names.
// Missing take number stays explicitly unknown; never infer it from commands.
inline void homeTitle(char out[65],const char *source) {
    while(*source==' ') ++source;
    const char *end=source;
    if(*end=='T') {
        ++end;while(*end>='0' && *end<='9') ++end;
    }
    const bool number=*source=='T' && end>source+1 && (!*end || *end==' ' || *end=='-');
    size_t n=0;
    if(number) {
        out[n++]='T';++source;
        if(end-source==1) out[n++]='0';
        while(source<end && n<60) out[n++]=*source++;
    }
    else {out[n++]='T';out[n++]='#';}
    while(*source==' ') ++source;
    if(number && *source=='-') {++source;while(*source==' ') ++source;}
    out[n++]='-';
    if(!*source) out[n++]='?';
    while(*source && n<64) out[n++]=*source++;
    while(n && out[n-1]==' ') --n;
    out[n]=0;
}
// A number is usable only when explicitly provided; never infer zero.
inline int selectedTake(const char *source) {
    char title[65];homeTitle(title,source);
    const char *p=title+1;if(*p<'0' || *p>'9') return -1;
    int value=0;while(*p>='0' && *p<='9') {
        if(value>1000000) return -1;
        value=value*10+(*p++-'0');
    }
    return *p=='-'?value:-1;
}
inline const char *shortStatus(State state) {
    switch(state) {
        case State::Ready:return "READY";
        case State::Recording:return "REC";
        case State::Playing:return "PLAY";
        case State::Saving:return "SAVE";
        default:return ""; // Persistent errors are rendered numerically instead.
    }
}
inline const char *requestedStatus(const Session &session) {
    if(session.pending || session.awaitingState) {
        if(session.commandAction==Action::RecordStop) {
            if(session.commandFrom==State::Recording) return "SAVE";
            if(session.commandFrom==State::Ready || session.commandFrom==State::Playing) return "REC";
        }
        if(session.commandAction==Action::PlayStop) {
            if(session.commandFrom==State::Playing) return "READY";
            if(session.commandFrom==State::Ready) return "PLAY";
        }
    }
    return shortStatus(session.state);
}
inline void statusRow(char out[17],const char *status,const PlaybackLevel &level,bool pending=false) {
    char label[12];snprintf(label,sizeof label,"%s%s",status,pending?"*":"");
    if(level.known) snprintf(out,17,"%-11.11sV:%03u",label,unsigned((uint16_t(level.level)*100+63)/127));
    else snprintf(out,17,"%-11.11sV:---",label);
}
inline void songTitle(char out[65],const char *source) {
    char title[65];homeTitle(title,source);
    const char *song=strchr(title,'-')+1;strcpy(out,song);
}
inline void takeTitle(char out[65],const char *source) {
    char title[65];homeTitle(title,source);
    const char *separator=strchr(title,'-');
    snprintf(out,65,"take : %.*s",int(separator-title-1),title+1);
}
inline void errorRow(char out[17],PanelError code) {
    char label[8];snprintf(label,sizeof label,"E%03u",unsigned(code));lcdRow(out,label);
}
}
