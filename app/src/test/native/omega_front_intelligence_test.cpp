#include "omega_front_intelligence.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
int main(){
    Intelligence a,b;
    assert(a.level(6)==Intelligence::Surveyed);
    assert(a.level(7)==Intelligence::Terrain);
    assert(a.level(2)==Intelligence::Terrain);
    assert(a.level(10)==Intelligence::Terrain);
    assert(a.level(0)==Intelligence::Unknown);
    assert(!Intelligence::Adjacent(3,4));
    assert(Intelligence::Adjacent(6,7));
    assert(!Intelligence::Adjacent(6,6));
    int food=5;
    assert(!a.Survey(6,11,food) && food==5);
    assert(a.Survey(6,7,food) && food==3);
    assert(!a.Survey(6,7,food) && food==3);
    int salvage=10,trust=55;int8_t standing=-30;
    uint16_t yard=7;uint8_t claims=16;
    assert(a.Parley(6,7,2,food,salvage,trust,standing,yard,claims));
    assert(food==2 && salvage==6 && trust==54 && standing==-22);
    assert(yard==11 && claims==10 && a.parleyed(7));
    assert(!a.Parley(6,7,2,food,salvage,trust,standing,yard,claims));
    assert(food==2 && salvage==6 && yard==11);
    b.ObserveArrival(7);
    assert(b.level(11)==Intelligence::Terrain);
    int none=0;assert(!b.Survey(7,11,none));
    auto levels=a.levels(),talks=a.talks();
    Intelligence c;
    assert(c.Restore(levels,talks,6));
    assert(c.level(7)==Intelligence::Surveyed && c.parleyed(7));
    auto bad=levels;bad[7]=3;
    assert(!c.Restore(bad,talks,6));
    assert(c.level(7)==Intelligence::Surveyed);
    bad=levels;bad[6]=0;
    assert(!c.Restore(bad,talks,6));
    bad=levels;bad[7]=1;
    assert(!c.Restore(bad,talks,6));
    int rr=10,ss=10,tt=55;int8_t hostile=-80;
    uint16_t wreck=100;uint8_t pressure=10;
    assert(!a.Parley(6,5,1,rr,ss,tt,hostile,wreck,pressure));
    assert(rr==10 && ss==10 && wreck==100);
    std::cout<<"Omega Front reconnaissance/parley resource and restore tests passed\n";
}
