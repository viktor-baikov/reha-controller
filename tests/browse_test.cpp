#include "../rehearsal_remote/Browse.hpp"
#include <assert.h>
#include <initializer_list>
using namespace remote;
void send(BrowseCatalog &c,uint8_t kind,std::initializer_list<uint8_t> data,bool good=true){
    uint8_t sum=kind+data.size();c.cc(90,kind,0);c.cc(91,data.size(),0);
    for(auto v:data){c.cc(92,v,0);sum+=v;}c.cc(93,good?(sum&127):((sum+1)&127),0);
}
int main(){
    BrowseCatalog c;
    send(c,0,{1,0,0,0,0,2,0,2,0,0,0,1,0});
    send(c,1,{1,0,0,0,0,0,0,'O','n','e'});
    send(c,2,{1,0,0,0,0,0,0,3,0,0,0,0});
    send(c,3,{1,0,0,0,0});assert(!c.valid); // incomplete list cannot publish
    send(c,1,{1,0,0,0,0,1,0,'T','w','o'},false);
    send(c,2,{1,0,0,0,0,1,0,10,0,0,0,0});
    send(c,3,{1,0,0,0,0});assert(!c.valid);
    send(c,1,{2,0,0,0,0,1,0,'B','a','d'}); // stale generation
    send(c,3,{1,0,0,0,0});assert(!c.valid);
    send(c,1,{1,0,0,0,0,1,0,'T','w','o'});
    send(c,3,{1,0,0,0,0});assert(c.valid && !strcmp(c.songs[1],"Two") && c.takes[1]==10);
    BrowsePreview p;p.begin(false,c);assert(!strcmp(p.label,"take : 10"));
    p.turn(-1,c);assert(!strcmp(p.label,"take : 03") && p.committable(c));
    p.turn(-100,c);assert(p.index==0);p.turn(1,c);assert(!p.changed);
    p.begin(true,c);p.turn(1,c);assert(!strcmp(p.label,"Two") && p.committable(c));
    c.generation++;assert(!p.committable(c));p.cancel();assert(!p.active);
    send(c,0,{3,0,0,0,0,0,0,0,0,127,127,127,127});send(c,3,{3,0,0,0,0});
    assert(c.valid);p.begin(false,c);p.turn(100,c);assert(!strcmp(p.label,"take : 00") && !p.committable(c));
    send(c,0,{4,0,0,0,0,1,2,0,0,127,127,127,127});assert(!c.valid); // 257 songs: refuse, never truncate
    HoldFilter h;assert(h.update(true,0));assert(h.update(false,1));assert(h.update(true,50));
    assert(h.update(false,100));assert(h.update(false,159));assert(!h.update(false,160));
    assert(h.update(true,0xfffffff0));assert(h.update(false,0xfffffff8));assert(!h.update(false,0x40));
}
