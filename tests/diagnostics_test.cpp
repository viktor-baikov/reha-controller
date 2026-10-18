#include <assert.h>
#include <stdarg.h>
#include <string>
struct UART {
    int capacity=256;std::string output;
    int availableForWrite() const {return capacity;}
    void println(const char *s) {output+=s;output+='\n';}
    void printf(const char *format,...) {char b[180];va_list a;va_start(a,format);vsnprintf(b,sizeof b,format,a);va_end(a);output+=b;}
} Serial;
#include "../rehearsal_remote/Diagnostics.hpp"
int main() {
    ControllerDiagnostics d;
    d.event(10,"input","pin=%d",6);assert(Serial.output.find("LOG t=10 event=input pin=6")!=std::string::npos);
    Serial.capacity=0;d.event(20,"dropped","still retained");assert(d.liveDropped==1);
    d.live=false;for(int i=0;i<40;++i)d.event(i,"counter","%d",i);
    assert(d.count==32);Serial.output.clear();d.dump();
    assert(Serial.output.find("event=counter 7\n")==std::string::npos);
    assert(Serial.output.find("event=counter 8\n")!=std::string::npos);
    assert(Serial.output.find("event=counter 39\n")!=std::string::npos);
    d.event(99,"long","%s",std::string(1000,'x').c_str());
    for(const auto &r:d.records)assert(r[ControllerDiagnostics::Width-1]==0);
}
