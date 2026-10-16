#include "omega_front_obligations.h"
#include <cassert>
#include <iostream>
using namespace reverie::omega;
int main(){
    Obligations q;
    assert(q.status(3)==Obligations::None);
    assert(q.status(2)==Obligations::None);
    assert(!q.Start(2,2));assert(!q.Start(3,0));
    assert(q.Start(3,5));assert(!q.Start(3,5));
    assert(q.due_day(3)==8);
    int rations=10,salvage=9,trust=55;
    int8_t relation=-14;uint16_t wreck=130,food=35;uint8_t claims=11;
    assert(!q.Settle(3,7,rations,salvage,trust,relation,wreck,food,claims));
    assert(salvage==9 && wreck==130);
    assert(q.Settle(3,8,rations,salvage,trust,relation,wreck,food,claims));
    assert(q.status(3)==Obligations::Honored && salvage==7 && wreck==132);
    assert(relation==-11 && trust==55 && claims==8);
    assert(!q.Settle(3,9,rations,salvage,trust,relation,wreck,food,claims));
    assert(salvage==7 && wreck==132); // never pays twice
    assert(q.Start(4,9));
    salvage=1;int8_t meridian=-95;uint16_t gate_wreck=9;uint8_t gate_claims=93;
    assert(q.Settle(4,12,rations,salvage,trust,meridian,gate_wreck,food,gate_claims));
    assert(q.status(4)==Obligations::Broken && salvage==1 && gate_wreck==9);
    assert(meridian==-100 && trust==51 && gate_claims==100);
    assert(q.Start(5,13));
    int8_t ninth=98;uint16_t ford_food=23,ford_wreck=8;uint8_t ford_claims=6;
    const int stock=rations+ford_food;
    assert(q.Settle(5,16,rations,salvage,trust,ninth,ford_wreck,ford_food,ford_claims));
    assert(q.status(5)==Obligations::Honored && rations+ford_food==stock);
    assert(ninth==100 && ford_claims==3);
    auto stages=q.stages();auto days=q.days();
    Obligations restored;
    assert(restored.Restore(stages,days,0x38,16));
    assert(restored.status(3)==Obligations::Honored);
    assert(restored.status(4)==Obligations::Broken);
    assert(restored.status(5)==Obligations::Honored);
    auto damaged=stages;damaged[1]=99;
    assert(!restored.Restore(damaged,days,0x38,16));
    assert(restored.stages()==stages); // transactional
    assert(!restored.Restore(stages,days,0x30,16)); // mask disagreement
    auto bad_day=days;bad_day[0]=15;
    assert(!restored.Restore(stages,bad_day,0x38,16)); // premature honor
    Obligations pending;
    assert(pending.Start(3,9));
    assert(!restored.Restore(pending.stages(),pending.days(),0x08,12)); // overdue
    Obligations legacy;
    assert(legacy.Migrate(0x18));
    assert(legacy.status(3)==Obligations::Legacy);
    assert(legacy.status(4)==Obligations::Legacy);
    assert(legacy.status(5)==Obligations::None);
    assert(legacy.Restore(legacy.stages(),legacy.days(),0x18,480));
    assert(!legacy.Settle(3,480,rations,salvage,trust,relation,wreck,food,claims));
    std::cout << "Omega Front treaty obligation/settlement regressions passed\n";
}
