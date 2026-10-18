#pragma once
#include <stdint.h>
#include "SelectionReference.hpp"

namespace remote {
constexpr uint8_t Version = 1;
constexpr uint8_t TokenCC = 113, AckTokenCC = 114, ResultCC = 116;
constexpr uint8_t VersionCC = 117, HeartbeatCC = 118, StateCC = 119;
enum class Action : uint8_t { RecordStop = 60, PlayStop = 62, Restart = 63, PreviousTake = 64, SeekBack = 65, NextTake = 66, SeekForward = 67, PreviousSong = 68, NextSong = 69, MuteToggle = 70, StartRehearsal = 71, StopRehearsal = 72, RemoveTake = 73, SelectTake = 74, SelectSong = 75 };
enum class State : uint8_t { Ready = 0, Recording = 1, Playing = 2,
                             Saving = 3, Error = 4, Unavailable = 5, Idle = 6 };
enum class Result : uint8_t { None = 0, Accepted = 1, Busy = 2,
                              Unavailable = 3, Error = 4, Timeout = 5 };

struct Session {
    SelectionReference selection;
    uint32_t removalTarget=0,browseGeneration=0;uint16_t browseIndex=0;
    uint8_t extensions=0;
    bool connected = false, versionKnown = false, snapshotKnown = false;
    bool syncedOnce = false, helloSent = false;
    uint32_t helloAt = 0;
    bool heartbeatKnown = false, pending = false, protocolMismatch = false, awaitingState = false;
    Action commandAction = Action::RecordStop;
    State commandFrom = State::Unavailable;
    uint32_t resultSequence = 0;
    State state = State::Unavailable;
    Result result = Result::None;
    uint8_t capabilities = 0;
    uint8_t token = 0, ackToken = 255;
    uint32_t heartbeatAt = 0, commandAt = 0;

    void connection(bool up) {
        connected = up; syncedOnce=false;helloSent=false;selection.reset();removalTarget=0;browseGeneration=0;
        versionKnown = snapshotKnown = heartbeatKnown = pending = protocolMismatch = awaitingState = false;
        state = State::Unavailable;
        result = Result::None;
        ackToken = 255;
        capabilities = extensions = 0;
    }
    bool fresh(uint32_t now) const {
        return connected && versionKnown && snapshotKnown && heartbeatKnown &&
               uint32_t(now - heartbeatAt) < 3500;
    }
    // A warm host can send state before our first HELLO. That must never
    // suppress the per-connection request that invalidates its text cache.
    bool helloDue(uint32_t now, bool metadataReady) {
        if(!connected) return false;
        if(helloSent) {
            if(pending || awaitingState) return false;
            const bool live=fresh(now);
            if(live && metadataReady) return false;
            // Both maximum-length rows need >1 s at the host's bounded rate.
            if(uint32_t(now-helloAt)<(live?2500u:1000u)) return false;
        }
        helloSent=true;helloAt=now;
        return true;
    }
    bool ignoresPress(Action action) const {
        return pending || awaitingState || (action==Action::PlayStop &&
            (state==State::Recording || state==State::Saving));
    }
    bool canSend(Action action, uint32_t now) const {
        if (!fresh(now) || pending || awaitingState) return false;
        if(action==Action::SelectTake || action==Action::SelectSong)
            return (extensions&1) && browseGeneration && (state==State::Ready || state==State::Playing);
        if(action==Action::StartRehearsal) return (capabilities&32) && state==State::Idle;
        if(action==Action::StopRehearsal) return (capabilities&32) && state==State::Ready;
        if(action==Action::RemoveTake) return (capabilities&64) && state==State::Ready && selection.valid;
        if (uint8_t(action) != 60 && uint8_t(action) != 62 && (uint8_t(action) < 63 || uint8_t(action) > 70)) return false;
        if (action == Action::MuteToggle) {
            if (!(capabilities & 16)) return false;
        } else if (uint8_t(action) >= 63 && !(capabilities & 1)) return false;
        if (state == State::Ready || state == State::Playing) return true;
        return state == State::Recording && (action == Action::RecordStop || action == Action::MuteToggle);
    }
    bool begin(Action action, uint32_t now) {
        if (!canSend(action, now)) return false;
        commandAction=action;commandFrom=state;
        awaitingState=action==Action::RecordStop || action==Action::PlayStop || action==Action::StartRehearsal || action==Action::StopRehearsal;
        token = (token + 1) & 0x7F;
        pending = true;
        ackToken = 255;
        result = Result::None;
        commandAt = now;
        return true;
    }
    void cc(uint8_t number, uint8_t value, uint32_t now) {
        if (!connected || value > 127) return;
        if (number == VersionCC) {
            versionKnown = value == Version;
            protocolMismatch = !versionKnown;
            if (!versionKnown) { snapshotKnown = heartbeatKnown = false; }
        } else if (versionKnown && number>=104 && number<=109) {
            selection.cc(number,value);
        } else if (versionKnown && number==110) {
            extensions=value;
        } else if (versionKnown && number == 112) {
            capabilities = value;
        } else if (versionKnown && number == StateCC && (value <= 5 || (value==6 && (capabilities&32)))) {
            state = State(value);
            snapshotKnown = true;
            if(awaitingState) {
                if(commandAction==Action::RecordStop)
                    awaitingState=commandFrom==State::Recording ? !(state==State::Saving || state==State::Ready) : state!=State::Recording;
                else if(commandAction==Action::PlayStop)
                    awaitingState=commandFrom==State::Playing ? state!=State::Ready : state!=State::Playing;
                else if(commandAction==Action::StartRehearsal) awaitingState=state!=State::Ready;
                else if(commandAction==Action::StopRehearsal) awaitingState=state!=State::Idle;
                if(state==State::Error || state==State::Unavailable) awaitingState=false;
            }
        } else if (versionKnown && number == HeartbeatCC) {
            heartbeatKnown = true;
            heartbeatAt = now;
        } else if (number == AckTokenCC) {
            ackToken = value;
        } else if (number == ResultCC && (pending || awaitingState) && ackToken == token && value >= 1 && value <= 4) {
            result = Result(value);
            if(result!=Result::Accepted) awaitingState=false;
            ++resultSequence;
            pending = false;
        }
        if(fresh(now)) syncedOnce=true;
    }
    void tick(uint32_t now) {
        if ((pending && uint32_t(now-commandAt)>=1500) ||
            (!pending && awaitingState && uint32_t(now-commandAt)>=3500)) {
            result = Result::Timeout;
            ++resultSequence;
            pending = awaitingState = false; // never retry a toggle automatically
        }
    }
};
}
