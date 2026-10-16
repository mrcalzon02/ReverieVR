#include "omega_front_accord.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
int main(){
    Accord accord;
    int rations=22,salvage=12,trust=55;
    int8_t relation=-2;uint16_t wreck=130;uint8_t claims=11;
    assert(Accord::Sector(3)==9 && Accord::Faction(3)==4);
    assert(!accord.Conclude(3,10,9,false,4,relation,rations,salvage,trust,wreck,claims));
    assert(rations==22 && salvage==12 && !accord.mask());
    assert(!accord.Conclude(3,6,9,true,4,relation,rations,salvage,trust,wreck,claims));
    assert(!accord.Conclude(3,10,9,true,2,relation,rations,salvage,trust,wreck,claims));
    assert(accord.Conclude(3,10,9,true,4,relation,rations,salvage,trust,wreck,claims));
    assert(accord.mask()==8 && accord.last_operation()==3 && accord.completed(3));
    assert(rations==20 && salvage==18 && wreck==124 && claims==4 && relation==2);
    assert(!accord.Conclude(3,10,9,true,4,relation,rations,salvage,trust,wreck,claims));
    assert(rations==20 && salvage==18 && wreck==124);
    Accord meridian;
    int8_t gateRelation=-22;uint16_t gateWreck=7;uint8_t gateClaims=16;
    rations=19;salvage=14;trust=50;
    assert(meridian.Conclude(4,6,7,true,2,gateRelation,rations,salvage,trust,gateWreck,gateClaims));
    assert(meridian.completed(4) && !meridian.completed(3));
    assert(rations==17 && salvage==8 && gateWreck==13 && gateRelation==-17 && gateClaims==9);
    Accord legion;
    int8_t ninthRelation=3;uint16_t fordWreck=6;uint8_t fordClaims=6;
    rations=9;salvage=5;trust=50;
    assert(legion.Conclude(5,5,4,true,0,ninthRelation,rations,salvage,trust,fordWreck,fordClaims));
    assert(rations==5 && salvage==2 && fordWreck==9 && ninthRelation==7 && fordClaims==0);
    Accord reject;
    assert(!reject.Restore(0x80,3,4));
    assert(!reject.Restore(8,3,3));
    assert(!reject.Restore(8,-1,4));
    assert(!reject.Restore(0,3,4));
    assert(!reject.Restore(0x18,3,6)); // later marked treaty, old last marker
    assert(reject.Restore(0x18,4,5));
    assert(reject.completed(3)&&reject.completed(4)&&reject.last_operation()==4);
    assert(reject.Restore(0,-1,0));
    std::cout<<"Omega Front physical concession and alternate resolution rules passed\n";
}
