#include "../../main/jni/native/sitka_environment.h"
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
using namespace reverie::sitka;
static bool rejects(std::function<void()> fn){try{fn();return false;}catch(const std::invalid_argument&){return true;}}
int main(){
    const std::string text="version=1\nlevel=sitka\ncount=1\n"
        "water.0.id=sea\nwater.0.shape=OBLONG\nwater.0.x=0\n"
        "water.0.y=0\nwater.0.z=0\nwater.0.width=100\n"
        "water.0.depth=90\nwater.0.yaw=0\n"
        "water.0.setting.wave.height=0.16\nwater.0.setting.wave.grid=0.75\n"
        "water.0.setting.layer1.speed=0.05\nwater.0.setting.layer2.speed=0.12\n";
    auto water=ParseWater(text);
    assert(water.size()==1&&water[0].id=="sea");
    assert(FootprintContains(water[0],1,3)&&!FootprintContains(water[0],55,55));
    assert(std::abs(WaveHeight(water[0],2,3,12)-WaveHeight(water[0],2,3,12))<1e-6f);
    assert(rejects([&]{ParseWater("version=1\nlevel=other\ncount=0\n");}));
    assert(rejects([&]{ParseWater(text+"water.0.width=8\n");}));
    assert(rejects([&]{ParseWater("version=1\nlevel=sitka\ncount=65\n");}));
    auto weather=ParseWeather("version=1\ncondition=RAIN\nhour=13\nwind_mps=1\nwind_heading_deg=90\n");
    assert(weather.kind=="RAIN"&&std::abs(weather.rain-.52f)<.001f);
    assert(std::abs(weather.heading-kPi*.5f)<.001f);
    assert(rejects([&]{ParseWeather("version=1\ncondition=DRAGONS\nhour=12\nwind_mps=1\nwind_heading_deg=0\n");}));
    assert(rejects([&]{ParseWeather("version=1\ncondition=RAIN\nhour=12\nwind_mps=nan\nwind_heading_deg=0\n");}));
    assert(FootprintContains(Water{"test","PILL",0,0,0,16,4,0},5,0));
    std::cout<<"sitka environment data adapter OK\n";
}
