// CPU-only contract: real Sitka desktop LightingAuthoringPanel schema v1.
// No gameplay/geometry or Android device acceptance is inferred.
#include "../../main/jni/native/sitka_lighting.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using reverie::sitka::ParseLighting;
static void Check(bool condition,const char* why){
    if(!condition)throw std::runtime_error(why);
}
static std::string Profile(int ambient,const std::string& preset="CUSTOM"){
    return "#ReverieVR lighting authoring; not native gameplay\n"
        "schema=1\npreset="+preset+"\nbeam=OMNIDIRECTIONAL\n"
        "ambient="+std::to_string(ambient)+"\nsource=100\nreach=100\n"
        "diffusion=100\nflicker=100\ncache=100\ncell=4\n"
        "outdoor=true\nchanged=true\nreuse=true\n";
}
static void Reject(const std::string& text){
    try{(void)ParseLighting(text);}
    catch(const std::invalid_argument&){return;}
    throw std::runtime_error("invalid authoring lighting accepted");
}
static std::string Swap(std::string text,const std::string& from,
                        const std::string& to){
    const auto pos=text.find(from);
    Check(pos!=std::string::npos,"fixture replacement missing");
    text.replace(pos,from.size(),to);
    return text;
}
int main(){
    try{
        Check(std::abs(ParseLighting(Profile(50)).ambient_scale-.5f)<1.e-6f,
              "minimum authored ambient lost");
        Check(std::abs(ParseLighting(Profile(100)).ambient_scale-1.f)<1.e-6f,
              "balanced authored ambient lost");
        Check(std::abs(ParseLighting(Profile(150)).ambient_scale-1.5f)<1.e-6f,
              "maximum authored ambient lost");
        Check(std::abs(ParseLighting(Profile(100,"BALANCED")).ambient_scale-1.f)<1.e-6f,
              "canonical named preset rejected");
        Reject(Profile(49));Reject(Profile(151));Reject(Profile(-1));
        Reject(Swap(Profile(100),"ambient=100","ambient=nan"));
        Reject(Swap(Profile(100),"ambient=100","ambient=100.5"));
        Reject(Swap(Profile(100),"schema=1","schema=2"));
        Reject(Swap(Profile(100),"source=100","source=999"));
        Reject(Swap(Profile(100),"cell=4","cell=3"));
        Reject(Swap(Profile(100),"outdoor=true","outdoor=perhaps"));
        Reject(Swap(Profile(100),"beam=OMNIDIRECTIONAL","beam=UNKNOWN"));
        Reject(Swap(Profile(100),"ambient=100\n",""));
        Reject(Profile(100)+"ambient=100\n");
        Reject(Profile(100)+"unexpected=1\n");
        Reject(Profile(150,"BALANCED"));
        std::cout<<"PASS: Java lighting schema v1 ambient intensity bridge and strict rejection\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FAIL: Sitka lighting parser: "<<e.what()<<"\n";
        return 1;
    }
}
