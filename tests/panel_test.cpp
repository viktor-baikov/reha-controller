#include <initializer_list>
#include "../rehearsal_remote/Panel.hpp"
#include "../rehearsal_remote/PanelDisplay.hpp"
#include <assert.h>
#include <string.h>
using namespace remote;
int main() {
    Quadrature q; q.reset(3);
    assert(!q.sample(2)); assert(!q.sample(3)); // contact bounce cancels
    int count=0; for(int v: {2,0,1,3}) count+=q.sample(v); assert(count==-1);
    count=0; for(int v: {1,0,2,3}) count+=q.sample(v); assert(count==1);
    q.reset(3); assert(!q.sample(0)); // missed/invalid diagonal edge is discarded
    q.reset(3); q.edgesPerDetent=2;
    assert(!q.sample(2)); assert(q.sample(0)==-1);
    PlaybackLevel level; level.turn(100); assert(level.take()==127);
    level.turn(-100); assert(level.take()==1);
    level.feedback(20,93); level.feedback(22,127); assert(level.known && level.muted);
    level.feedback(22,1); assert(level.muted); level.reset(); assert(!level.known && !level.muteKnown && level.queued==0);
    PanelMenu m; Action a=Action::RecordStop;
    assert(!m.click(a) && m.page==MenuPage::Browse);
    assert(!m.turn(1,a) && m.item==MenuItem::StopRehearsal);
    assert(!m.click(a) && m.page==MenuPage::Browse); // no v1 command
    assert(!strcmp(m.name(),"Stop rehearsal"));
    assert(!m.turn(-1,a) && m.item==MenuItem::Restart);
    assert(!strcmp(m.name(),"Restart take"));
    assert(!m.click(a) && m.page==MenuPage::Browse); // never bypass confirmation
    m.setTake(3);m.turn(2,a);assert(m.item==MenuItem::RemoveTake);
    assert(!strcmp(m.name(),"Remove 03 take"));
    m.confirm("T3 Song");assert(m.page==MenuPage::Confirm && m.confirmedTake==3);
    m.back();assert(m.page==MenuPage::Browse);
    m.setTake(0);assert(m.item==MenuItem::StopRehearsal);
    m.turn(1,a);assert(m.item==MenuItem::Brightness);
    m.turn(1,a);assert(m.item==MenuItem::LcdTest);
    m.turn(1,a);assert(m.item==MenuItem::Restart);
    m.setTake(-1);m.turn(2,a);assert(m.item==MenuItem::Brightness);
    m.back();assert(m.page==MenuPage::Home);
    char title[65];
    homeTitle(title,"T12 - Midnight - Live");assert(!strcmp(title,"T12-Midnight - Live"));
    homeTitle(title,"T12 Midnight");assert(!strcmp(title,"T12-Midnight"));
    homeTitle(title,"T12-Midnight");assert(!strcmp(title,"T12-Midnight"));
    homeTitle(title,"Song name");assert(!strcmp(title,"T#-Song name"));
    homeTitle(title,"T0 Midnight");assert(!strcmp(title,"T00-Midnight"));
    takeTitle(title,"T0 Midnight");assert(!strcmp(title,"take : 00"));
    homeTitle(title,"T1 Song");assert(!strcmp(title,"T01-Song"));
    homeTitle(title,"T2 Song");assert(!strcmp(title,"T02-Song"));
    homeTitle(title,"T3 Song");assert(!strcmp(title,"T03-Song"));
    homeTitle(title,"T09 Song");assert(!strcmp(title,"T09-Song"));
    homeTitle(title,"T100 Song");assert(!strcmp(title,"T100-Song"));
    takeTitle(title,"T1 Song");assert(!strcmp(title,"take : 01"));
    takeTitle(title,"T100 Song");assert(!strcmp(title,"take : 100"));
    assert(selectedTake("T01 Song")==1 && selectedTake("T00 Song")==0 && selectedTake("T100 Song")==100);
    homeTitle(title,"T99");assert(!strcmp(title,"T99-?"));
    homeTitle(title,"");assert(!strcmp(title,"T#-?"));
    homeTitle(title,"  Tune 12  ");assert(!strcmp(title,"T#-Tune 12"));
    songTitle(title,"T12 - Midnight - Live");assert(!strcmp(title,"Midnight - Live"));
    takeTitle(title,"T12 - Midnight - Live");assert(!strcmp(title,"take : 12"));
    songTitle(title,"");assert(!strcmp(title,"?"));takeTitle(title,"Song");assert(!strcmp(title,"take : #"));
    char status[17];PlaybackLevel displayLevel;
    statusRow(status,"READY",displayLevel);assert(!strcmp(status,"READY      V:---"));
    displayLevel.feedback(20,64);statusRow(status,"REC",displayLevel);assert(!strcmp(status,"REC        V:050"));
    displayLevel.feedback(20,127);statusRow(status,"PLAY",displayLevel);assert(!strcmp(status,"PLAY       V:100"));
    errorRow(status,PanelError::Host);assert(!strcmp(status,"E004            "));
    displayLevel.feedback(20,0);statusRow(status,"SAVE",displayLevel);assert(!strcmp(status,"SAVE       V:000"));
    HeldSelection held;
    assert(held.update(true,false)==HeldMode::Take);
    assert(held.update(true,true)==HeldMode::Conflict);
    assert(held.update(false,true)==HeldMode::Song);
    held.cancel();assert(held.update(false,true)==HeldMode::None);
    assert(held.update(false,false)==HeldMode::None);
    assert(held.update(true,false)==HeldMode::Take);
    char row[17]; lcdRow(row,"abc"); assert(strlen(row)==16 && !strncmp(row,"abc ",4));
    lcdRow(row,"12345678901234567890"); assert(!strcmp(row,"1234567890123456"));
    lcdRow(row,""); assert(!strcmp(row,"                "));
    lcdRow(row,"a\nb"); assert(!strncmp(row,"a?b ",4));
    Session s; s.connection(true); s.cc(117,1,0);s.cc(119,1,0);s.cc(118,1,0);
    assert(!s.canSend(Action::MuteToggle,0)); s.cc(112,16,0);
    assert(s.canSend(Action::MuteToggle,0)); assert(!s.canSend(Action::NextTake,0));
    assert(!s.canSend(Action::MuteToggle,3500));
    assert(recordLamp(s,0));assert(!recordLamp(s,3500));
    s.cc(119,3,0);assert(!recordLamp(s,0));assert(recordLamp(s,200));
    s.connection(false);assert(!recordLamp(s,200));
    assert(persistentPanelError(s,200)==PanelError::Bluetooth);
    s.connection(true);assert(persistentPanelError(s,200)==PanelError::Feedback);
    s.cc(117,2,200);assert(persistentPanelError(s,200)==PanelError::Protocol);
    s.cc(117,1,200);s.cc(119,4,200);s.cc(118,1,200);
    assert(persistentPanelError(s,200)==PanelError::Host);
    s.cc(119,5,200);assert(persistentPanelError(s,200)==PanelError::Unavailable);
    assert(resultPanelError(Result::Timeout)==PanelError::Timeout);
    size_t errorCount=0;const auto *errors=panelErrorList(errorCount);assert(errorCount==13);
    for(size_t i=0;i<errorCount;++i) assert(unsigned(errors[i].code)==i+1 && errors[i].meaning[0]);
}
