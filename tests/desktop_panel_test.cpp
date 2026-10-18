#include "desktop_panel_stubs.hpp"
int main() {
    pins[REHA_LEVEL_A]=pins[REHA_LEVEL_B]=1;
    DesktopPanel p;p.begin();session.connection(true);session.cc(117,1,0);session.cc(112,31,0);session.cc(119,0,0);
    p.feedback(20,64,0);strcpy(text.rows[0],"T1 Test song");strcpy(text.rows[1],"READY");update(p);
    assert(!strcmp(p.rows[0],"T01-Test song   "));
    assert(!strcmp(p.rows[1],"READY      V:050"));
    turn(p);assert(levels.size()==1 && levels.back()==66 && actions.empty());
    click(p,REHA_LEVEL_PUSH);assert(p.menu.page==remote::MenuPage::Browse);
    assert(!strcmp(p.rows[0],"Restart take    "));
    turn(p);assert(p.menu.item==remote::MenuItem::StopRehearsal && levels.size()==1 && actions.empty());
    click(p,REHA_LEVEL_PUSH);assert(actions.empty() && !strcmp(p.rows[0],"are you sure?   ") && !strcmp(p.rows[1],"                "));
    click(p,REHA_LEVEL_PUSH);assert(actions.empty() && !strcmp(p.rows[1],"E013            "));
    click(p,REHA_PANEL_BACK_PIN);click(p,REHA_LEVEL_PUSH);
    p.menu.home();p.notice=remote::PanelError::None;
    session.cc(110,1,clockNow);
    p.catalog.valid=true;p.catalog.generation=7;p.catalog.songCount=2;p.catalog.takeCount=3;
    p.catalog.songIndex=p.catalog.takeIndex=0;
    strcpy(p.catalog.songs[0],"Test song");strcpy(p.catalog.songs[1],"Other song");
    for(int i=0;i<3;++i)p.catalog.takes[i]=i+1;
    Button::down[REHA_TAKE_PIN]=true;update(p);assert(!strcmp(p.rows[0],"select take     "));
    turn(p);assert(actions.empty() && !strcmp(p.rows[1],"take : 02       "));
    turn(p);assert(actions.empty() && !strcmp(p.rows[1],"take : 03       "));
    Button::down[REHA_SONG_PIN]=true;update(p);turn(p);assert(actions.empty());
    assert(!strcmp(p.rows[1],"E011            "));
    click(p,REHA_PANEL_BACK_PIN);turn(p);assert(actions.empty() && levels.size()==1);
    releaseHeld(p,REHA_TAKE_PIN);releaseHeld(p,REHA_SONG_PIN);
    Button::down[REHA_TAKE_PIN]=true;update(p);turn(p);assert(actions.empty());
    releaseHeld(p,REHA_TAKE_PIN);
    assert(actions.size()==1 && actions.back()==remote::Action::SelectTake && session.browseIndex==1);
    ack();strcpy(text.rows[0],"T2 Test song");update(p);
    p.catalog.takeIndex=1;
    Button::down[REHA_SONG_PIN]=true;update(p);
    turn(p);assert(actions.size()==1 && !strcmp(p.rows[1],"Other song      "));
    releaseHeld(p,REHA_SONG_PIN);
    assert(actions.size()==2 && actions.back()==remote::Action::SelectSong && session.browseIndex==1);ack();update(p);
    click(p,REHA_LEVEL_PUSH);const auto beforeRestart=actions.size();click(p,REHA_LEVEL_PUSH);
    assert(p.menu.page==remote::MenuPage::Confirm && actions.size()==beforeRestart && !strcmp(p.rows[1],"                "));
    click(p,REHA_PANEL_BACK_PIN);assert(actions.size()==beforeRestart && p.menu.page==remote::MenuPage::Browse);
    click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);assert(p.menu.page==remote::MenuPage::Browse);
    assert(actions.back()==remote::Action::Restart);ack();update(p);
    click(p,REHA_PANEL_BACK_PIN);assert(p.menu.page==remote::MenuPage::Home);
    session.cc(119,1,clockNow);update(p);assert(pins[REHA_PANEL_RECORD_LED]==HIGH);
    Button::down[REHA_TAKE_PIN]=true;update(p);turn(p);assert(actions.size()==3);
    releaseHeld(p,REHA_TAKE_PIN);
    session.cc(119,4,clockNow);update(p);assert(!strcmp(p.rows[1],"E004            "));
    session.connection(false);p.reset();update(p);assert(pins[REHA_PANEL_RECORD_LED]==LOW);
    assert(!strcmp(p.rows[0],"Bluetooth MIDI  ") && !strcmp(p.rows[1],"Disconnected    "));
    Button::down[REHA_TAKE_PIN]=false;session.connection(true);
    session.cc(117,1,clockNow);session.cc(112,31,clockNow);session.cc(119,0,clockNow);update(p);
    click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);
    session.cc(114,session.token,clockNow);session.cc(116,4,clockNow);update(p);
    assert(!strcmp(p.rows[1],"E009            "));
    click(p,REHA_PANEL_BACK_PIN);click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);
    clockNow+=1600;session.tick(clockNow);update(p);
    assert(!strcmp(p.rows[1],"E010            "));
    session.pending=false;session.cc(119,5,clockNow);p.menu.home();p.notice=remote::PanelError::None;
    update(p);assert(!strcmp(p.rows[1],"E005            "));
    click(p,REHA_LEVEL_PUSH);assert(p.menu.page==remote::MenuPage::Home);
    assert(!strcmp(p.rows[1],"E005            "));
    // Requested REC* is not confirmed recording: do not light the lamp early.
    session=remote::Session{};p.reset();memset(Button::down,0,sizeof Button::down);
    session.connection(true);session.cc(117,1,clockNow);session.cc(112,31,clockNow);
    session.cc(119,0,clockNow);p.feedback(20,64,clockNow);update(p);
    char title[17];strcpy(title,p.rows[0]);
    p.action(remote::Action::RecordStop,session,clockNow);update(p);
    assert(!strcmp(p.rows[0],title) && !strcmp(p.rows[1],"REC*       V:050"));
    assert(pins[REHA_PANEL_RECORD_LED]==LOW);
    const auto count=actions.size();const auto token=session.token;
    p.action(remote::Action::RecordStop,session,clockNow);update(p);
    assert(actions.size()==count && session.token==token && p.notice==remote::PanelError::None);
    assert(!strcmp(p.rows[1],"REC*       V:050"));
    ack();update(p);assert(session.awaitingState && !strcmp(p.rows[1],"REC*       V:050"));
    p.action(remote::Action::RecordStop,session,clockNow);update(p);assert(actions.size()==count);
    session.cc(119,1,clockNow);update(p);
    assert(!session.awaitingState && !strcmp(p.rows[1],"REC        V:050") && pins[REHA_PANEL_RECORD_LED]==HIGH);
    const auto recordingActions=actions.size();const auto recordingToken=session.token;
    p.action(remote::Action::PlayStop,session,clockNow);update(p);
    p.action(remote::Action::PlayStop,session,clockNow);update(p);
    assert(actions.size()==recordingActions && session.token==recordingToken && !session.pending);
    assert(p.notice==remote::PanelError::None && !strcmp(p.rows[1],"REC        V:050"));
    assert(pins[REHA_PANEL_RECORD_LED]==HIGH);
    p.action(remote::Action::RecordStop,session,clockNow);update(p);
    assert(!strcmp(p.rows[1],"SAVE*      V:050"));ack();update(p);
    assert(!strcmp(p.rows[1],"SAVE*      V:050"));
    session.cc(119,3,clockNow);update(p);assert(!strcmp(p.rows[1],"SAVE       V:050"));
    const auto savingActions=actions.size();const auto savingToken=session.token;
    p.action(remote::Action::PlayStop,session,clockNow);update(p);
    assert(actions.size()==savingActions && session.token==savingToken && !session.pending);
    assert(p.notice==remote::PanelError::None && !strcmp(p.rows[1],"SAVE       V:050"));
    session.cc(119,0,clockNow);update(p);assert(!strcmp(p.rows[1],"READY      V:050"));
    // Initial or lost recorder is a normal waiting screen; reopening recovers.
    session.connection(true);p.reset();update(p);
    assert(!strcmp(p.rows[0],"Bluetooth MIDI  ") && !strcmp(p.rows[1],"Waiting recorder"));
    session.cc(117,1,clockNow);session.cc(112,127,clockNow);session.cc(119,6,clockNow);update(p);
    assert(!strcmp(p.rows[0],"connected       ") && !strcmp(p.rows[1],"start rehersal? "));
    const auto beforeStart=actions.size();click(p,REHA_LEVEL_PUSH);
    assert(actions.size()==beforeStart+1 && actions.back()==remote::Action::StartRehearsal && session.awaitingState);
    assert(p.menu.page==remote::MenuPage::Home); // Start has no confirmation
    click(p,REHA_LEVEL_PUSH);assert(actions.size()==beforeStart+1); // no duplicate
    ack();update(p);assert(p.rows[0][15]=='*');
    strcpy(text.rows[0],"T0 Test song");session.cc(119,0,clockNow);update(p);
    assert(!strcmp(p.rows[0],"T00-Test song   "));
    click(p,REHA_LEVEL_PUSH);turn(p);turn(p);assert(p.menu.item==remote::MenuItem::Brightness); // no Remove 0
    turn(p);assert(p.menu.item==remote::MenuItem::LcdTest);turn(p);assert(p.menu.item==remote::MenuItem::Restart);
    session.selection.value=42;session.selection.valid=true;
    strcpy(text.rows[0],"T3 Test song");update(p);turn(p);turn(p);
    assert(!strcmp(p.rows[0],"Remove 03 take  "));
    click(p,REHA_LEVEL_PUSH);const auto beforeRemove=actions.size();
    session.selection.valid=false;update(p);assert(p.menu.page==remote::MenuPage::Home);
    session.selection.value=43;session.selection.valid=true;
    click(p,REHA_LEVEL_PUSH);turn(p);turn(p);click(p,REHA_LEVEL_PUSH);
    click(p,REHA_PANEL_BACK_PIN);assert(actions.size()==beforeRemove && p.menu.page==remote::MenuPage::Browse);
    click(p,REHA_LEVEL_PUSH);strcpy(text.rows[0],"T4 Test song");update(p);
    assert(p.menu.page==remote::MenuPage::Home && actions.size()==beforeRemove); // no retarget
    click(p,REHA_PANEL_BACK_PIN);click(p,REHA_LEVEL_PUSH);turn(p);turn(p);click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);
    assert(actions.back()==remote::Action::RemoveTake);ack();update(p);
    click(p,REHA_LEVEL_PUSH);turn(p);click(p,REHA_LEVEL_PUSH);click(p,REHA_LEVEL_PUSH);
    assert(actions.back()==remote::Action::StopRehearsal && session.awaitingState);
    ack();session.cc(119,6,clockNow);update(p);assert(!strcmp(p.rows[0],"connected       "));
    p.catalog.valid=true;const auto beforeClose=actions.size();
    clockNow+=4000;p.update(session,text,clockNow);
    assert(!strcmp(p.rows[0],"Bluetooth MIDI  ") && !strcmp(p.rows[1],"Waiting recorder"));
    assert(!p.catalog.valid && !p.preview.active && !p.level.known);
    assert(!session.canSend(remote::Action::RecordStop,clockNow) && actions.size()==beforeClose);
    session.cc(117,1,clockNow);session.cc(112,127,clockNow);session.cc(119,6,clockNow);
    update(p);assert(!strcmp(p.rows[0],"connected       ") && !strcmp(p.rows[1],"start rehersal? "));
    assert(actions.size()==beforeClose); // reopen never replays any command

    // Preview every detent locally; one absolute commit only on stable release.
    session=remote::Session{};memset(Button::down,0,sizeof Button::down);
    session.connection(true);session.cc(117,1,clockNow);session.cc(112,127,clockNow);
    session.cc(110,1,clockNow);session.cc(119,0,clockNow);strcpy(text.rows[0],"T3 Test song");
    DesktopPanel held;held.begin();held.feedback(20,64,clockNow);update(held);
    held.catalog.valid=true;held.catalog.generation=42;held.catalog.songCount=3;held.catalog.takeCount=3;
    held.catalog.songIndex=held.catalog.takeIndex=1;
    strcpy(held.catalog.songs[0],"One");strcpy(held.catalog.songs[1],"Two");strcpy(held.catalog.songs[2],"Three");
    held.catalog.takes[0]=1;held.catalog.takes[1]=3;held.catalog.takes[2]=10;
    for(int key: {REHA_TAKE_PIN,REHA_SONG_PIN}){
        Button::down[key]=true;update(held);const auto before=actions.size(),vol=levels.size();
        for(int i=0;i<12;++i){
            const int cycles[2][4]={{1,0,2,3},{2,0,1,3}};
            for(int ab:cycles[i%2]){pins[REHA_LEVEL_A]=(ab>>1)&1;pins[REHA_LEVEL_B]=ab&1;callbacks[REHA_LEVEL_A](contexts[REHA_LEVEL_A]);}
            update(held);assert(actions.size()==before && levels.size()==vol);
            assert(!strncmp(held.rows[0],"select ",7));
        }
        // Brief release cannot commit or switch to home/volume.
        Button::down[key]=false;update(held);Button::down[key]=true;update(held);
        assert(actions.size()==before && !strncmp(held.rows[0],"select ",7));
        turn(held);releaseHeld(held,key);
        assert(actions.size()==before+1 && session.browseIndex==2 && session.browseGeneration==42);
        ack();update(held);
    }
    // Back cancels; stale catalogs cancel; empty songs remain T00 with no request.
    auto before=actions.size();Button::down[REHA_TAKE_PIN]=true;update(held);turn(held);
    click(held,REHA_PANEL_BACK_PIN);releaseHeld(held,REHA_TAKE_PIN);assert(actions.size()==before);
    Button::down[REHA_TAKE_PIN]=true;update(held);turn(held);held.catalog.generation++;
    releaseHeld(held,REHA_TAKE_PIN);assert(actions.size()==before);
    held.catalog.takeCount=0;held.catalog.takeIndex=remote::BrowseCatalog::None;
    Button::down[REHA_TAKE_PIN]=true;update(held);turn(held);assert(!strcmp(held.rows[1],"take : 00       "));
    releaseHeld(held,REHA_TAKE_PIN);assert(actions.size()==before);
    // A partial background catalog disables browsing, without changing the
    // heading, sending commands, moving volume, or retaining an old preview.
    auto frame=[&](uint8_t kind,std::initializer_list<uint8_t> data){
        uint8_t sum=kind+data.size();held.feedback(90,kind,clockNow);held.feedback(91,data.size(),clockNow);
        for(auto v:data){held.feedback(92,v,clockNow);sum+=v;}
        held.feedback(93,sum&127,clockNow);
    };
    for(int key: {REHA_TAKE_PIN,REHA_SONG_PIN}) {
        held.catalog.valid=true;held.catalog.takeCount=3;held.catalog.takeIndex=0;
        Button::down[key]=true;update(held);turn(held);
        const auto count=actions.size(),volumeCount=levels.size();const auto requests=catalogRequests;
        frame(0,{60,0,0,0,0,1,0,3,0,0,0,0,0});
        held.tell(remote::PanelError::ActionUnavailable,clockNow);
        for(int i=0;i<4;++i) {
            turn(held);
            assert(!strcmp(held.rows[0],key==REHA_TAKE_PIN?"select take     ":"select song     "));
            assert(!strcmp(held.rows[1],"Updating list   "));
            assert(!held.preview.active && actions.size()==count && levels.size()==volumeCount);
        }
        assert(catalogRequests==requests); // no request on individual detents
        // Releasing an incomplete list must never submit the old choice.
        releaseHeld(held,key);assert(actions.size()==count);
        Button::down[key]=true;update(held);
        frame(1,{60,0,0,0,0,0,0,'N','e','w'});
        frame(2,{60,0,0,0,0,0,0,1,0,0,0,0});
        frame(2,{60,0,0,0,0,1,0,2,0,0,0,0});
        frame(2,{60,0,0,0,0,2,0,3,0,0,0,0});
        frame(3,{60,0,0,0,0});
        held.notice=remote::PanelError::None;update(held);
        assert(held.preview.active && held.preview.index==0);
        assert(!strcmp(held.rows[1],key==REHA_TAKE_PIN?"take : 01       ":"New             "));
        // Transient errors keep the heading; expiry only restores bottom text.
        held.tell(remote::PanelError::ActionUnavailable,clockNow);update(held);
        assert(!strcmp(held.rows[0],key==REHA_TAKE_PIN?"select take     ":"select song     "));
        assert(!strcmp(held.rows[1],"E007            "));
        held.notice=remote::PanelError::None;
        if(key==REHA_TAKE_PIN) {
            turn(held);releaseHeld(held,key);
            assert(actions.size()==count+1 && session.browseGeneration==60 && session.browseIndex==1);ack();
        } else {releaseHeld(held,key);assert(actions.size()==count);}
    }
    // Finishing a recording invalidates the old take list immediately, even
    // before the next background catalog header has arrived.
    const auto requests=catalogRequests;
    session.cc(119,1,clockNow);update(held);
    session.cc(119,3,clockNow);update(held);
    session.cc(119,0,clockNow);Button::down[REHA_TAKE_PIN]=true;update(held);
    assert(catalogRequests==requests+1);
    assert(!held.catalog.valid && !strcmp(held.rows[0],"select take     "));
    assert(!strcmp(held.rows[1],"Updating list   "));
    update(held);turn(held);assert(catalogRequests==requests+1);
    // A complete replacement between panel updates also resets the preview.
    held.catalog.valid=true;held.catalog.takeIndex=0;held.notice=remote::PanelError::None;update(held);turn(held);
    assert(held.preview.index==1);
    ++held.catalog.generation;held.catalog.takeIndex=2;turn(held);
    assert(held.preview.index==2 && !held.preview.changed && !strcmp(held.rows[1],"take : 03       "));

    // Brightness is local, previews live, saves on click, cancels on BACK,
    // survives controller reset, and never sends MIDI volume/transport events.
    memset(Button::down,0,sizeof Button::down);session=remote::Session{};
    session.connection(true);session.cc(117,1,clockNow);session.cc(112,127,clockNow);session.cc(119,0,clockNow);
    DesktopPanel light;light.begin();update(light);const auto actionCount=actions.size(),levelCount=levels.size();
    click(light,REHA_LEVEL_PUSH);turn(light);turn(light);turn(light);
    assert(light.menu.item==remote::MenuItem::Brightness && !strcmp(light.rows[1],"Press to adjust "));
    click(light,REHA_LEVEL_PUSH);assert(light.menu.page==remote::MenuPage::Brightness);
    turn(light);assert(light.brightnessEdit==15 && brightnessWrites==0);
    assert(!strcmp(light.rows[0],"Brightness      ") && !strcmp(light.rows[1],"15%  Press save "));
    click(light,REHA_PANEL_BACK_PIN);assert(light.brightness==10 && brightnessWrites==0);
    click(light,REHA_LEVEL_PUSH);turn(light);click(light,REHA_LEVEL_PUSH);
    assert(light.brightness==15 && storedBrightness==15 && brightnessWrites==1);
    click(light,REHA_LEVEL_PUSH);click(light,REHA_LEVEL_PUSH);assert(brightnessWrites==1); // unchanged: no flash write
    click(light,REHA_LEVEL_PUSH);for(int i=0;i<25;++i)turn(light);
    assert(light.brightnessEdit==100);
    brightnessSaveOk=false;click(light,REHA_LEVEL_PUSH);
    assert(light.menu.page==remote::MenuPage::Brightness && light.brightness==15);
    brightnessSaveOk=true;click(light,REHA_PANEL_BACK_PIN);
    light.reset();update(light);assert(light.brightness==15);

    assert(actions.size()==actionCount && levels.size()==levelCount);

    // Temporary timing mode: encoder clicks are local optical references,
    // RUN -> STOP -> ZERO. Rotation never leaks through to MIDI volume.
    click(light,REHA_LEVEL_PUSH);for(int i=0;i<4;++i)turn(light);
    assert(light.menu.item==remote::MenuItem::LcdTest);
    click(light,REHA_LEVEL_PUSH);
    assert(light.menu.page==remote::MenuPage::LcdTest && testLedStage==0);
    assert(!strcmp(light.rows[0],"LCD timer ZERO  ") && !strcmp(light.rows[1],"0000000000 ms   "));
    click(light,REHA_LEVEL_PUSH);assert(light.testStage==1 && testLedStage==1);
    clockNow+=300;update(light);assert(light.testElapsed>=300);
    click(light,REHA_LEVEL_PUSH);const auto frozen=light.testElapsed;
    assert(light.testStage==2 && testLedStage==2 && !strcmp(light.rows[0],"LCD timer STOP  "));
    for(int i=0;i<5;++i){turn(light);assert(light.testElapsed==frozen);}
    for(int i=0;i<30;++i)update(light);
    assert(!light.testWritePending && reportedStage==2 && reportedElapsed==frozen);
    const auto reports=testReports;update(light);assert(testReports==reports);
    click(light,REHA_LEVEL_PUSH);assert(light.testStage==0 && testLedStage==0 && light.testElapsed==0);
    click(light,REHA_LEVEL_PUSH);assert(light.testStage==1);
    click(light,REHA_PANEL_BACK_PIN);assert(light.menu.page==remote::MenuPage::Browse && !light.testWritePending);
    assert(actions.size()==actionCount && levels.size()==levelCount);
    DesktopPanel reboot;reboot.begin();assert(reboot.brightness==15);

}
