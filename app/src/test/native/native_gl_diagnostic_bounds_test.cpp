#include "reverie_native_gl_utils.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){
    char full[64]{};
    ReverieNativeGlDiagnosticWriteV1(full,sizeof(full),"fragment shader","compile failed");
    assert(std::strcmp(full,"fragment shader: compile failed")==0);
    ReverieNativeGlDiagnosticWriteV1(full,sizeof(full),nullptr,nullptr);
    assert(std::strcmp(full,"GLES2: no driver detail")==0);
    char guarded[9]={'L','?','?','?','?','?','?','?','R'};
    for(size_t capacity=0;capacity<=7;++capacity){
        std::memset(guarded+1,'?',7);
        ReverieNativeGlDiagnosticWriteV1(guarded+1,capacity,"very long phase","long driver info");
        assert(guarded[0]=='L'&&guarded[8]=='R');
        if(capacity)assert(guarded[capacity]=='\0');
        for(size_t i=capacity+1;i<8;++i)assert(guarded[i]=='?');
    }
    ReverieNativeGlDiagnosticWriteV1(nullptr,80,nullptr,nullptr);
    char large[256];char message[1024];std::memset(message,'X',sizeof(message));message[1023]='\0';
    ReverieNativeGlDiagnosticWriteV1(large,sizeof(large),"vertex shader",message);
    assert(std::strlen(large)==255&&std::strncmp(large,"vertex shader: ",15)==0);
    std::puts("PASS GL diagnostic bounds, defaults and truncation without format warnings");
}
