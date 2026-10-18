#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
namespace remote {
// CC90=kind, CC91=length, CC92=data, CC93=checksum(kind+length+data).
struct BrowseFrame {
    uint8_t kind=255,length=0,index=0,sum=0,data[48]={};
    uint32_t at=0;bool sized=false;
    void reset(){kind=255;sized=false;index=0;}
    bool cc(uint8_t cc,uint8_t value,uint32_t now){
        if(kind!=255 && uint32_t(now-at)>=1500)reset();
        if(cc==90){kind=value;index=0;sized=false;sum=value;at=now;}
        else if(cc==91 && kind!=255){if(value>sizeof data){reset();return false;}length=value;sum=(sum+value)&127;sized=true;}
        else if(cc==92 && kind!=255 && sized){if(index>=length){reset();return false;}data[index++]=value;sum=(sum+value)&127;}
        else if(cc==93 && kind!=255){bool ok=sized && index==length && sum==value; if(!ok)reset();return ok;}
        return false;
    }
};
inline uint32_t browseNumber(const uint8_t *p){uint32_t n=0;for(uint8_t i=0;i<5;++i)n|=uint32_t(p[i])<<(7*i);return n;}
inline uint16_t browseIndex(const uint8_t *p){return p[0]|(uint16_t(p[1])<<7);}
struct BrowseCatalog {
    static constexpr uint16_t MaxSongs=128,MaxTakes=512,None=16383;
    BrowseFrame frame;
    char songs[MaxSongs][33]={};uint32_t takes[MaxTakes]={};
    bool songSeen[MaxSongs]={},takeSeen[MaxTakes]={},valid=false,staging=false;
    uint16_t songCount=0,takeCount=0,songIndex=None,takeIndex=None;
    uint32_t generation=0,activity=0;
    void reset(){valid=staging=false;generation=0;frame.reset();}
    bool cc(uint8_t cc,uint8_t value,uint32_t now){
        if(cc>=90 && cc<=93)activity=now;
        if(!frame.cc(cc,value,now))return false;
        const uint8_t kind=frame.kind,n=frame.length;frame.reset();
        const auto *p=frame.data;
        if(n<5 || p[4]>15)return false;
        uint32_t tag=browseNumber(p);if(!tag)return false;
        if(kind==0 && n==13){
            valid=staging=false;generation=tag;
            songCount=browseIndex(p+5);takeCount=browseIndex(p+7);
            songIndex=browseIndex(p+9);takeIndex=browseIndex(p+11);
            if(songCount>MaxSongs || takeCount>MaxTakes ||
               (songIndex!=None && songIndex>=songCount) || (takeIndex!=None && takeIndex>=takeCount))return false;
            memset(songSeen,0,sizeof songSeen);memset(takeSeen,0,sizeof takeSeen);staging=true;
        }else if(staging && tag==generation && kind==1 && n>=7 && n<=39){
            uint16_t i=browseIndex(p+5);if(i>=songCount)return false;
            memcpy(songs[i],p+7,n-7);songs[i][n-7]=0;songSeen[i]=true;
        }else if(staging && tag==generation && kind==2 && n==12 && p[11]<=15){
            uint16_t i=browseIndex(p+5);if(i>=takeCount)return false;
            takes[i]=browseNumber(p+7);if(!takes[i] || takes[i]>1000000)return false;takeSeen[i]=true;
        }else if(staging && tag==generation && kind==3 && n==5){
            for(uint16_t i=0;i<songCount;++i)if(!songSeen[i])return false;
            for(uint16_t i=0;i<takeCount;++i)if(!takeSeen[i])return false;
            valid=true;staging=false;return true;
        }
        return false;
    }
};
// Delay only release; keep short contact dropouts from ending a held gesture.
struct HoldFilter {
    bool held=false,releasing=false;uint32_t releasedAt=0;
    void reset(){held=releasing=false;}
    bool update(bool down,uint32_t now){
        if(down){held=true;releasing=false;}
        else if(held){if(!releasing){releasing=true;releasedAt=now;}
            else if(uint32_t(now-releasedAt)>=60)held=releasing=false;}
        return held;
    }
};
struct BrowsePreview {
    bool active=false,changed=false;bool song=false;uint16_t index=BrowseCatalog::None,start=BrowseCatalog::None;
    uint32_t generation=0;char label[65]={};
    void cancel(){active=changed=false;}
    void render(const BrowseCatalog &c){
        if(song){if(index<c.songCount)strcpy(label,c.songs[index]);else strcpy(label,"No songs");}
        else snprintf(label,sizeof label,"take : %02lu",index<c.takeCount?(unsigned long)c.takes[index]:0ul);
    }
    void begin(bool isSong,const BrowseCatalog &c){
        cancel();if(!c.valid)return;active=true;song=isSong;generation=c.generation;
        index=start=song?c.songIndex:c.takeIndex;render(c);
    }
    void turn(int steps,const BrowseCatalog &c){
        if(!active || !c.valid || c.generation!=generation)return;
        int count=song?c.songCount:c.takeCount;if(!count)return;
        int next=index==BrowseCatalog::None?(steps>0?0:count-1):int(index)+steps;
        index=next<0?0:next>=count?count-1:next;changed=index!=start;render(c);
    }
    bool committable(const BrowseCatalog &c)const{return active && changed && c.valid && generation==c.generation;}
};
}
