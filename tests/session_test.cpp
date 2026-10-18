#include "../rehearsal_remote/Session.hpp"
#include <assert.h>
#include <stdio.h>
using namespace remote;
void ready(Session &s, uint32_t now) {
    s.cc(VersionCC, Version, now);
    s.cc(StateCC, uint8_t(State::Ready), now);
    s.cc(HeartbeatCC, 1, now);
}
int main() {
    // Warm-host state wins the startup race, but cannot suppress HELLO.
    Session sync;
    assert(!sync.helloDue(0,false));
    sync.connection(true);ready(sync,0);sync.cc(112,31,0);
    assert(sync.fresh(0) && sync.helloDue(0,false));
    assert(!sync.helloDue(1000,false)); // allow complete row transmission
    assert(sync.helloDue(2500,false)); // retry missing or partial metadata
    sync.cc(HeartbeatCC,1,3000);
    assert(!sync.helloDue(3000,true)); // both committed: no needless HELLO
    assert(!sync.helloDue(5000,true));
    sync.pending=true;sync.cc(HeartbeatCC,1,5500);
    assert(!sync.helloDue(5500,false)); // never invalidate an in-flight command
    sync.pending=false;assert(sync.helloDue(5500,false));
    sync.connection(false);assert(!sync.helloDue(5501,false));
    sync.connection(true);ready(sync,5502);
    assert(sync.helloDue(5502,true)); // rapid reconnect always gets a new HELLO
    Session absent;absent.connection(true);assert(absent.helloDue(0,false));
    assert(!absent.helloDue(999,false) && absent.helloDue(1000,false));
    Session s;
    assert(!s.begin(Action::RecordStop, 0));
    s.connection(true);
    s.cc(StateCC, 0, 0); s.cc(HeartbeatCC, 1, 0);
    assert(!s.fresh(0)); // old probe host cannot grant authority
    ready(s, 100);
    assert(!s.canSend(Action::SeekForward, 100));
    s.cc(112, 1, 100); assert(s.canSend(Action::SeekForward, 100));
    assert(s.begin(Action::RecordStop, 100));
    const auto first = s.token;
    assert(!s.begin(Action::PlayStop, 101));
    s.cc(AckTokenCC, (first + 1) & 127, 101); s.cc(ResultCC, 1, 101);
    assert(s.pending); // wrong/late ack cannot clear current command
    s.cc(AckTokenCC, first, 102); s.cc(ResultCC, 1, 102);
    assert(!s.pending && s.result == Result::Accepted);
    assert(s.awaitingState && !s.canSend(Action::RecordStop,102));
    s.cc(StateCC, uint8_t(State::Recording), 103);
    assert(!s.awaitingState);
    assert(!s.canSend(Action::PlayStop, 103));
    assert(s.ignoresPress(Action::PlayStop));
    assert(!s.ignoresPress(Action::RecordStop));
    assert(s.canSend(Action::RecordStop, 103));
    s.cc(StateCC, uint8_t(State::Saving), 104);
    assert(!s.canSend(Action::RecordStop, 104));
    assert(s.ignoresPress(Action::PlayStop));
    s.cc(StateCC,uint8_t(State::Ready),105);assert(!s.ignoresPress(Action::PlayStop));
    s.cc(StateCC,uint8_t(State::Playing),106);assert(!s.ignoresPress(Action::PlayStop));
    assert(!s.fresh(3600));
    s.connection(false);
    s.cc(HeartbeatCC, 1, 4000);
    assert(!s.fresh(4000));
    s.connection(true); ready(s, 0xFFFFFF00);
    assert(s.fresh(100)); // millis() rollover
    assert(s.begin(Action::RecordStop, 0xFFFFFF00));
    s.tick(1300);
    assert(!s.pending && s.result == Result::Timeout);
    s.cc(VersionCC, 2, 1300);
    assert(!s.fresh(1300));
    Session delayed;delayed.connection(true);ready(delayed,100);
    assert(delayed.begin(Action::RecordStop,100));
    delayed.cc(AckTokenCC,delayed.token,110);delayed.cc(ResultCC,1,110);
    delayed.cc(HeartbeatCC,1,3000);delayed.tick(3000);assert(delayed.awaitingState && !delayed.pending);
    delayed.tick(3600);assert(!delayed.awaitingState && delayed.result==Result::Timeout);
    Session failed;failed.connection(true);ready(failed,0);
    assert(failed.begin(Action::RecordStop,0));
    failed.cc(114,failed.token,1);failed.cc(116,1,1);assert(failed.awaitingState);
    failed.cc(114,failed.token,2);failed.cc(116,4,2);
    assert(!failed.awaitingState && failed.result==Result::Error);
    puts("Firmware session tests passed");

    Session extension;extension.connection(true);ready(extension,0);
    assert(!extension.canSend(Action::StopRehearsal,0));
    extension.cc(119,6,0);assert(extension.state==State::Ready); // no implicit idle
    extension.cc(112,127,0);assert(extension.canSend(Action::StopRehearsal,0));
    assert(!extension.canSend(Action::RemoveTake,0));
    extension.cc(104,42,0);for(int cc=105;cc<=108;++cc)extension.cc(cc,0,0);extension.cc(109,42,0);
    assert(extension.canSend(Action::RemoveTake,0) && !extension.canSend(Action::StartRehearsal,0));
    extension.cc(119,6,0);assert(extension.canSend(Action::StartRehearsal,0));
    assert(!extension.canSend(Action::RecordStop,0) && !extension.canSend(Action::RemoveTake,0));
    assert(extension.begin(Action::StartRehearsal,0));
    extension.cc(114,extension.token,1);extension.cc(116,1,1);assert(extension.awaitingState);
    extension.cc(119,0,2);assert(!extension.awaitingState);
}
