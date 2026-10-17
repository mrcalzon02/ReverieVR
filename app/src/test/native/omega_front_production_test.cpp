#include "omega_front_production.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using namespace reverie::omega;

static_assert(CivilProduction::Store(65534,5)==65535,"depot saturates");
static_assert(CivilProduction::Store(20,5)==25,"depot conserves additions");
static_assert(CivilProduction::Produce(8,0,45,0,0,0,18).food==0,"no settlers");
static_assert(CivilProduction::Produce(10,0,76,0,9,8,0).water==0,"broken pump");
static_assert(CivilProduction::Produce(10,0,76,0,9,8,18).water==6,"live pump");
int main() {
    for(int i=0;i<16;++i) {
        for(unsigned eco=0;eco<3;++eco) {
            const auto y=CivilProduction::Produce(i,eco,82,0,31,8,18);
            if(eco!=0)assert(y.food==0 && y.water==0);
            else if(i==10)assert(y.food==0 && y.water==6);
            else assert(y.food==1 && y.water==1);
        }
    }
    assert(CivilProduction::Produce(5,0,82,1,120,0,18).food==0);
    assert(CivilProduction::Produce(5,0,34,0,120,0,18).water==0);
    assert(CivilProduction::Produce(5,0,82,0,120,0,18).food==4);
    assert(CivilProduction::Produce(5,0,82,0,120,65,18).food==2);
    assert(CivilProduction::Produce(5,0,50,0,120,65,18).food==1);
    assert(CivilProduction::Produce(10,0,76,0,9,8,7).water==3);
    assert(CivilProduction::Produce(10,0,76,0,9,8,0).food==0);
    assert(CivilProduction::Produce(6,0,82,0,0,0,18).water==0);
    assert(CivilProduction::Produce(6,1,82,0,31,0,18).water==0);
    assert(CivilProduction::Store(65535,65535)==65535);
    const DailyYield one=CivilProduction::Produce(6,0,82,0,31,8,18);
    uint16_t food=44,water=34;
    for(int day=0;day<120;++day){
        food=CivilProduction::Store(food,one.food);
        water=CivilProduction::Store(water,one.water);
        // Simulated physical daily home consumption remains separate and
        // never wraps unsigned inventory negative.
        food=food>=4?static_cast<uint16_t>(food-4):0;
        water=water>=4?static_cast<uint16_t>(water-4):0;
    }
    assert(food==0 && water==0);
    std::cout<<"Omega Front bounded Eden production and pump dependence passed\n";
}
