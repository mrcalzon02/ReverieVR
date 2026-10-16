#include "omega_front_border.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
int main(){
    BorderPatrol patrol;
    int rations=5,trust=51;
    int8_t relation=-90;
    uint16_t store=10;
    uint8_t claims=10;
    assert(!patrol.Enforce(4,6,7,Obligations::Honored,rations,trust,relation,store,claims));
    assert(!patrol.Enforce(4,6,7,Obligations::Due,rations,trust,relation,store,claims));
    assert(!patrol.Enforce(4,5,7,Obligations::Broken,rations,trust,relation,store,claims));
    assert(!patrol.Enforce(4,6,9,Obligations::Broken,rations,trust,relation,store,claims));
    assert(patrol.Enforce(4,6,7,Obligations::Broken,rations,trust,relation,store,claims));
    assert(rations==3 && store==12 && relation==-94 && trust==49 && claims==18);
    assert(patrol.inspected(4) && patrol.last_operation()==4);
    assert(!patrol.Enforce(4,6,7,Obligations::Broken,rations,trust,relation,store,claims));
    assert(rations==3 && store==12);
    // A short-supplied commander can still retreat and the patrol cannot
    // collect imaginary rations from an empty pack.
    int scant=0;int8_t ninth=-99;uint16_t ford=32;uint8_t pressure=98;
    assert(patrol.Enforce(5,5,4,Obligations::Broken,scant,trust,ninth,ford,pressure));
    assert(scant==0 && ford==32 && ninth==-100 && pressure==100);
    // Capacity is honored even if the commander could have paid more.
    int fresh=9;int8_t scrap=-20;uint16_t full=UINT16_MAX-1;uint8_t other=0;
    assert(patrol.Enforce(3,5,9,Obligations::Broken,fresh,trust,scrap,full,other));
    assert(fresh==8 && full==UINT16_MAX);
    Obligations obligations;
    assert(obligations.Start(3,1) && obligations.Start(4,2) && obligations.Start(5,3));
    int supplies=0;uint16_t stock=0;
    int8_t r=-3;uint8_t pressure2=5;
    assert(obligations.Settle(3,4,scant,supplies,trust,r,stock,ford,pressure2));
    assert(obligations.Settle(4,5,scant,supplies,trust,r,stock,ford,pressure2));
    assert(obligations.Settle(5,6,scant,supplies,trust,r,stock,ford,pressure2));
    BorderPatrol saved;
    assert(saved.Restore(patrol.charged_mask(),patrol.grandfathered_mask(),
                         patrol.last_operation(),obligations));
    assert(saved.charged_mask()==0x38u);
    auto before=saved.charged_mask();
    assert(!saved.Restore(0x80,0,-1,obligations));
    assert(!saved.Restore(0x08,0x08,3,obligations));
    assert(!saved.Restore(0x08,0,-1,obligations));
    assert(saved.charged_mask()==before);
    BorderPatrol old;
    old.Migrate(obligations);
    assert(old.grandfathered_mask()==0x38u && old.charged_mask()==0);
    assert(!old.Enforce(3,5,9,Obligations::Broken,fresh,trust,scrap,full,other));
    assert(old.Restore(0,0x38,-1,obligations));
    // Early relations are not considered a broken treaty.
    Obligations clean;
    assert(!old.Restore(0,0x08,-1,clean));
    std::cout << "Omega Front finite border retaliation regressions passed\n";
}
