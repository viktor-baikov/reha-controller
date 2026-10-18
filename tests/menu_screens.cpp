// Export actual renderer snapshots and its shared error catalogue for docs.
#include "desktop_panel_stubs.hpp"
#include <iostream>
#include <string>
void quote(const std::string &value) {
    std::cout<<'"';for(char c:value) {if(c=='"' || c=='\\') std::cout<<'\\';std::cout<<c;}std::cout<<'"';
}
void field(const char *key,const std::string &value) {quote(key);std::cout<<':';quote(value);}
void snapshot(const char *id,const char *description,remote::State state=remote::State::Ready,
              int held=0,int menu=0,remote::PanelError error=remote::PanelError::None,
              int volume=64,const char *name="T12 - Midnight rehearsal song",bool pending=false,int recordRequest=0,bool accepted=false,int capabilities=31) {
    session=remote::Session{};text=remote::TextRows{};clockNow=100;
    memset(Button::down,0,sizeof Button::down);Wire=TestWire{};
    pins[REHA_LEVEL_A]=pins[REHA_LEVEL_B]=1;
    DesktopPanel p;p.begin();session.connection(true);session.cc(117,1,100);
    session.cc(112,capabilities,100);session.cc(119,uint8_t(state),100);session.cc(118,1,100);
    strncpy(text.rows[0],name,32);text.rows[0][32]=0;
    if(volume>=0) p.feedback(20,volume,100);
    if(menu) p.menu.page=remote::MenuPage::Browse;
    if(menu==2 || menu==4) p.menu.item=remote::MenuItem::StopRehearsal;
    if(menu==3 || menu==5) p.menu.item=remote::MenuItem::RemoveTake;
    if(menu==6) p.menu.item=remote::MenuItem::Restart;
    p.menu.setTake(remote::selectedTake(text.rows[0]));
    session.selection.value=42;session.selection.valid=true;
    if(menu>=4) {p.menu.confirm(text.rows[0]);p.menu.confirmedReference=42;}
    if(!strcmp(id,"menu-brightness") || !strcmp(id,"edit-brightness")) {
        p.menu.item=remote::MenuItem::Brightness;
        p.menu.page=!strcmp(id,"edit-brightness")?remote::MenuPage::Brightness:remote::MenuPage::Browse;
        p.brightnessEdit=50;
    }
    if(!strcmp(id,"menu-lcd-test")) {p.menu.item=remote::MenuItem::LcdTest;p.menu.page=remote::MenuPage::Browse;}
    if(!strncmp(id,"lcd-test-",9)) {
        p.menu.item=remote::MenuItem::LcdTest;p.menu.page=remote::MenuPage::LcdTest;
        p.testStage=!strcmp(id,"lcd-test-run")?1:!strcmp(id,"lcd-test-stop")?2:0;
        p.testElapsed=p.testStage?1234:0;p.testFrameAt=clockNow;
    }
    session.cc(110,1,clockNow);p.catalog.valid=true;p.catalog.generation=42;
    p.catalog.songCount=1;p.catalog.songIndex=0;
    char previewSong[65];remote::songTitle(previewSong,text.rows[0]);
    strncpy(p.catalog.songs[0],previewSong,32);p.catalog.songs[0][32]=0;
    int selected=remote::selectedTake(text.rows[0]);
    p.catalog.takeCount=selected>0?1:0;p.catalog.takeIndex=selected>0?0:remote::BrowseCatalog::None;
    if(selected>0)p.catalog.takes[0]=selected;
    if(!strcmp(id,"held-take-updating") || !strcmp(id,"held-song-updating")) p.catalog.valid=false;
    Button::down[REHA_TAKE_PIN]=held&1;Button::down[REHA_SONG_PIN]=held&2;
    session.pending=pending;
    if(recordRequest) {
        session.pending=false;assert(session.begin(remote::Action::RecordStop,100));
        if(accepted) {session.cc(114,session.token,100);session.cc(116,1,100);}
    }
    switch(error) {
        case remote::PanelError::Bluetooth:session.connection(false);p.reset();text.reset();break;
        case remote::PanelError::Feedback:
            clockNow=4000;
            if(!strcmp(id,"flow-sync") || !strcmp(id,"waiting-initial")) {session.syncedOnce=false;session.snapshotKnown=false;}
            break;
        case remote::PanelError::Protocol:session.cc(117,2,100);break;
        case remote::PanelError::Host:session.cc(119,4,100);break;
        case remote::PanelError::Unavailable:session.cc(119,5,100);break;
        case remote::PanelError::ConflictingKeys:Button::down[REHA_TAKE_PIN]=Button::down[REHA_SONG_PIN]=true;break;
        default:if(error!=remote::PanelError::None) p.tell(error,100);break;
    }
    if(strcmp(id,"boot")) p.update(session,text,clockNow);
    char title[65],song[65],take[65];remote::homeTitle(title,text.rows[0]);
    remote::songTitle(song,text.rows[0]);remote::takeTitle(take,text.rows[0]);
    auto expand=[&](const char *visible) -> std::string {
        const char *candidates[]={title,song,take,p.menu.name()};
        for(const char *candidate: candidates) {
            char row[17];remote::lcdRow(row,candidate);
            if(!strcmp(row,visible)) return candidate;
            std::string starred=candidate;
            if(starred.size()<15) starred.resize(15,' ');
            starred.insert(15,1,'*');remote::lcdRow(row,starred.c_str());
            if(!strcmp(row,visible)) return starred;
        }
        return visible;
    };
    const std::string full0=expand(p.rows[0]),full1=expand(p.rows[1]);
    char check[17];remote::lcdRow(check,full0.c_str());assert(!strcmp(check,p.rows[0]));
    remote::lcdRow(check,full1.c_str());assert(!strcmp(check,p.rows[1]));
    static bool first=true;if(!first) std::cout<<',';first=false;
    std::cout<<'{';field("id",id);std::cout<<',';field("description",description);
    std::cout<<",\"rows\":[";quote(full0);std::cout<<',';quote(full1);std::cout<<"],\"visible\":[";
    quote(p.rows[0]);std::cout<<',';quote(p.rows[1]);std::cout<<"]}";
}
int main() {
    std::cout<<"{\"screens\":[";
    snapshot("boot","Startup buffer only; normal update may replace it before LCD initialization completes");
    snapshot("home-ready","Home: ready; volume 50 percent");
    snapshot("menu-lcd-test","Temporary LCD measurement menu");
    snapshot("lcd-test-zero","LED off; press to start");
    snapshot("lcd-test-run","LED green; press freezes elapsed milliseconds");
    snapshot("lcd-test-stop","LED red; press resets to zero");
    snapshot("menu-brightness","Local backlight setting");
    snapshot("edit-brightness","Turn to preview; press saves, BACK cancels");
    snapshot("home-rec","Home: recording; volume remains visible",remote::State::Recording);
    snapshot("home-play","Home: playback; volume remains visible",remote::State::Playing);
    snapshot("home-save","Home: saving; lamp flashes",remote::State::Saving);
    snapshot("home-zero","Home: volume zero",remote::State::Ready,0,false,remote::PanelError::None,0);
    snapshot("home-full","Home: volume 100 percent",remote::State::Ready,0,false,remote::PanelError::None,127);
    snapshot("home-volume-unknown","Home: no volume feedback yet",remote::State::Ready,0,false,remote::PanelError::None,-1);
    snapshot("home-take-unknown","Host supplied song without take number: do not invent one",remote::State::Ready,0,false,remote::PanelError::None,64,"Midnight rehearsal song");
    snapshot("waiting-initial","Feedback not fresh, with no identity or volume available",remote::State::Ready,0,false,remote::PanelError::Feedback,-1,"");
    snapshot("home-empty","No identity metadata yet",remote::State::Ready,0,false,remote::PanelError::None,64,"");
    snapshot("menu-restart","Restart take item: press requests restart",remote::State::Ready,0,true);
    snapshot("menu-stop","Stop rehearsal menu item; press opens confirmation",remote::State::Ready,0,2);
    snapshot("menu-remove","Remove selected take; hidden for zero/unknown",remote::State::Ready,0,3);
    snapshot("confirm-stop","Stop rehearsal confirmation: encoder yes, BACK no; blank second row",remote::State::Ready,0,4);
    snapshot("confirm-remove","Remove selected take confirmation: encoder yes, BACK no",remote::State::Ready,0,5);
    snapshot("idle-start","Explicit host idle plus capability: connected / start rehersal?",remote::State::Idle,0,0,remote::PanelError::None,64,"",false,0,false,127);
    snapshot("idle-start-pending","Start sends immediately; pending star without confirmation",remote::State::Idle,0,0,remote::PanelError::None,64,"",true,0,false,127);
    snapshot("confirm-restart","Restart take confirmation: encoder yes, BACK no",remote::State::Ready,0,6);
    snapshot("menu-start-unavailable","Recorder not ready overrides and closes the menu",remote::State::Unavailable,0,2);
    snapshot("home-pending","Home: pending acknowledgement adds a star to status, not song",remote::State::Ready,0,0,remote::PanelError::None,64,"T12 - Midnight rehearsal song",true);
    snapshot("menu-pending","Menu: command awaiting acknowledgement",remote::State::Ready,0,true,remote::PanelError::None,64,"T12 - Midnight rehearsal song",true);
    snapshot("held-take","Hold TAKE; turn for previous/next take",remote::State::Ready,1);
    snapshot("held-song","Hold SONG; turn for previous/next song",remote::State::Ready,2);
    snapshot("held-take-updating","Take list loading: knob ignored, heading unchanged",remote::State::Ready,1);
    snapshot("held-song-updating","Song list loading: knob ignored, heading unchanged",remote::State::Ready,2);
    snapshot("held-take-unknown","TAKE: number not supplied by host",remote::State::Ready,1,0,remote::PanelError::None,64,"");
    snapshot("held-song-unknown","SONG: name not supplied by host",remote::State::Ready,2,0,remote::PanelError::None,64,"");
    snapshot("held-take-pending","TAKE request awaiting acknowledgement",remote::State::Ready,1,false,remote::PanelError::None,64,"T12 - Midnight rehearsal song",true);
    snapshot("held-song-pending","SONG request awaiting acknowledgement",remote::State::Ready,2,false,remote::PanelError::None,64,"T12 - Midnight rehearsal song",true);
    snapshot("flow-power","Controller powers up before BLE connects",remote::State::Ready,0,0,remote::PanelError::Bluetooth);
    snapshot("flow-sync","BLE connected; waiting for initial recorder feedback",remote::State::Ready,0,0,remote::PanelError::Feedback,-1,"");
    snapshot("flow-idle","Healthy recorder connected; no rehearsal open",remote::State::Idle,0,0,remote::PanelError::None,64,"",false,0,false,127);
    snapshot("home-no-takes","Host explicitly reports no takes as T0",remote::State::Ready,0,0,remote::PanelError::None,64,"T0 Midnight");
    snapshot("flow-ready","Laptop has started rehearsal; song Midnight, no kept takes yet",remote::State::Ready,0,0,remote::PanelError::None,64,"T0 Midnight");
    snapshot("flow-record-request","Record request sent; REC* means requested, not confirmed",remote::State::Ready,0,0,remote::PanelError::None,64,"T0 Midnight",false,1);
    snapshot("flow-record-ack","Acknowledgement accepted; retain REC* until recording state arrives",remote::State::Ready,0,0,remote::PanelError::None,64,"T0 Midnight",false,1,true);
    snapshot("flow-recording","Recorder confirms recording take 1; lamp now on",remote::State::Recording,0,0,remote::PanelError::None,64,"T1 Midnight");
    snapshot("flow-stop-request","Record pressed again; stop/keep requested",remote::State::Recording,0,0,remote::PanelError::None,64,"T1 Midnight",false,1);
    snapshot("flow-stop-ack","Stop accepted; waiting for recorder Saving/Ready state",remote::State::Recording,0,0,remote::PanelError::None,64,"T1 Midnight",false,1,true);
    snapshot("flow-saving","Recorder confirms Saving; lamp blinks",remote::State::Saving,0,0,remote::PanelError::None,64,"T1 Midnight");
    snapshot("flow-kept","Recorder confirms Ready; take kept; sequence ends",remote::State::Ready,0,0,remote::PanelError::None,64,"T1 Midnight");
    size_t count=0;const auto *errors=remote::panelErrorList(count);
    for(size_t i=0;i<count;++i) {
        char id[12];snprintf(id,sizeof id,"error-%03u",unsigned(errors[i].code));
        snapshot(id,errors[i].meaning,remote::State::Ready,0,false,errors[i].code);
    }
    std::cout<<"],\"errors\":[";
    for(size_t i=0;i<count;++i) {
        if(i) std::cout<<',';std::cout<<"{\"code\":"<<unsigned(errors[i].code)<<',';
        field("meaning",errors[i].meaning);std::cout<<',';field("recovery",errors[i].recovery);std::cout<<'}';
    }
    std::cout<<"]}\n";
}
