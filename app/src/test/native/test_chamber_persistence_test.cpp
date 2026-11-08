// Production module lifecycle and input with an in-memory faulting host.
#include "../../main/jni/native/test_chamber_module.cpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
namespace {
std::map<std::string,std::string> disk;
bool failWrite=false,applyThenFail=false;
unsigned writes=0u;
void TestLog(int32_t,const char*,const char*) {}
int32_t TestRead(const char* name,void* data,uint32_t capacity,uint32_t* size) {
    const auto found=disk.find(name);if(found==disk.end())return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(found->second.size());
    if(!data)return REVERIE_NATIVE_SAVE_OK;
    if(capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    if(*size)std::memcpy(data,found->second.data(),*size);
    return REVERIE_NATIVE_SAVE_OK;
}
int32_t TestWrite(const char* name,const void* data,uint32_t size) {
    ++writes;if(failWrite&&!applyThenFail)return REVERIE_NATIVE_SAVE_ERROR;
    disk[name]=std::string(static_cast<const char*>(data),size);
    return failWrite?REVERIE_NATIVE_SAVE_ERROR:REVERIE_NATIVE_SAVE_OK;
}
int32_t TestAsset(const char* name,void* data,uint32_t capacity,uint32_t* size) {
    std::ifstream file(std::string("app/src/main/assets/")+name,std::ios::binary|std::ios::ate);
    if(!file)return REVERIE_NATIVE_SAVE_NOT_FOUND;
    *size=static_cast<uint32_t>(file.tellg());
    if(!data||capacity<*size)return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    file.seekg(0);file.read(static_cast<char*>(data),*size);
    return file?REVERIE_NATIVE_SAVE_OK:REVERIE_NATIVE_SAVE_ERROR;
}
std::string Name(unsigned slot) {return "vr-workshop-project-"+std::to_string(slot)+".rvproj";}
ModuleState* New(const ReverieNativeHostV1& host) {
    auto* s=static_cast<ModuleState*>(Create(&host));assert(s);return s;
}
ReverieNativeInputV1 Input() {
    ReverieNativeInputV1 input{};input.struct_size=sizeof(input);input.delta_seconds=.05f;
    return input;
}
void InForwardView(ModuleState* s) {
    UpdateWorkshopHud(s);assert(!s->font.vertices.empty());
    const float tangent=std::tan(40.f*3.14159265358979323846f/180.f);
    for(size_t index=0;index<s->font.vertices.size();index+=8u) {
        const auto& vertices=s->font.vertices;
        const float extent=-vertices[index+2u]*tangent-.05f;
        assert(std::fabs(vertices[index])+.032f<extent);
        assert(std::fabs(vertices[index+1u])<extent);
    }
}
}
int main() {
    using namespace reverie::authoring;
    ReverieNativeHostV1 host{};host.struct_size=sizeof(host);host.abi_version=1u;
    host.log=TestLog;host.read_save=TestRead;host.write_save=TestWrite;host.read_asset=TestAsset;
    const Scene authored{"white_room",{Piece{"original-box","box","-",0,-1,-3,0,.5}}};
    disk[Name(0)]="ORIGINAL INVALID ZERO";disk[Name(2)]=EncodeVrScene(authored);
    auto* s=New(host);assert(s->workshop.slot==2u&&!s->workshop.recovering&&writes==0u);
    InForwardView(s);
    assert(kApi.prepare_exit(s)==1);
    auto input=Input();input.secondary_down=1u;Update(s,&input);assert(s->workshop.editing);
    InForwardView(s);
    s->workshop.motion_page=true;InForwardView(s);
    s->workshop.lighting_page=true;InForwardView(s);
    s->workshop.audio_page=true;InForwardView(s);
    s->workshop.motion_page=s->workshop.lighting_page=s->workshop.audio_page=false;
    input.secondary_down=0u;Update(s,&input);
    input.pointer_kind=REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    const float x=static_cast<float>((kVrMenuLeft+kVrMenuRight)*.5),y=.99f;
    const float length=std::sqrt(x*x+y*y+2.62f*2.62f);
    input.pointer_direction[0]=x/length;input.pointer_direction[1]=y/length;
    input.pointer_direction[2]=-2.62f/length;input.primary_down=1u;Update(s,&input);
    assert(s->workshop.dirty&&s->workshop.scene.pieces.size()==2u);
    failWrite=true;kApi.pause(s);assert(s->workshop.dirty&&s->save_failed&&kApi.prepare_exit(s)==0);
    assert(DecodeVrBundle(disk.at(Name(2))).scene.pieces.size()==1u);
    applyThenFail=true;assert(kApi.prepare_exit(s)==0&&s->workshop.dirty);
    assert(DecodeVrBundle(disk.at(Name(2))).scene.pieces.size()==2u);
    kApi.resume(s);Update(s,&input);Update(s,&input);
    assert(s->workshop.scene.pieces.size()==2u); // Held action is not replayed.
    failWrite=applyThenFail=false;input.primary_down=0u;
    for(unsigned i=0;i<45u;++i)Update(s,&input);
    assert(!s->workshop.dirty&&!s->save_failed&&kApi.prepare_exit(s)==1);
    assert(disk.at(Name(0))=="ORIGINAL INVALID ZERO");Destroy(s);
    s=New(host);assert(s->workshop.slot==2u&&s->workshop.scene.pieces.size()==2u);Destroy(s);

    // Actual Create returns a live retry screen even when every save is corrupt.
    disk[Name(0)]="bad-zero";disk[Name(1)]="bad-one";disk[Name(2)]="bad-two";
    const auto originals=disk;const auto previousWrites=writes;s=New(host);
    assert(s->workshop.recovering&&kApi.prepare_exit(s)==0&&writes==previousWrites);
    InForwardView(s);
    input=Input();input.primary_down=1u;Update(s,&input);
    assert(s->workshop.recovering&&disk==originals&&s->demo_mode==0);
    input.primary_down=0u;Update(s,&input);disk[Name(1)]=EncodeVrScene(authored);
    input.primary_down=1u;Update(s,&input);
    assert(!s->workshop.recovering&&s->workshop.slot==1u&&kApi.prepare_exit(s)==1);
    assert(s->demo_mode==0&&disk.at(Name(0))==originals.at(Name(0))&&
           disk.at(Name(2))==originals.at(Name(2))&&writes==previousWrites);
    Destroy(s);
    std::cout<<"Test Chamber module: valid fallback, dirty pause/exit retry, applied-error writes, held input, recreation and all-bad recovery PASS\n";
}
