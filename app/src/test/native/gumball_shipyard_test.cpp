#include "gumball_shipyard.h"
#include <cassert>
#include <iostream>
using namespace reverie::gumball;
int main(){
 Shipyard s;assert(s.ready());assert(s.count(Part::Core)==1);assert(s.stock(Part::Armor)==3);
 assert(!s.place(0,0,Part::Armor));assert(!s.remove(3,3));
 assert(s.place(4,2,Part::Armor));assert(s.count(Part::Armor)==1);assert(s.profile().hull>=14);
 assert(s.remove(4,2));assert(s.stock(Part::Armor)==3);assert(!s.remove(4,2));
 assert(s.place(3,0,Part::Frame));assert(!s.remove(3,1));
 assert(s.place(4,1,Part::Thruster));assert(s.profile().speed>0.f);assert(s.ready());
 auto saved=s.snapshot();Shipyard restored;assert(restored.restore(saved));assert(restored.ready());
 assert(restored.count(Part::Thruster)==2);
 auto bad=saved;bad.stock[2]=99;bad.checksum=ShipyardChecksum(bad);assert(!restored.restore(bad));
 bad=saved;bad.cells[3*7+3]=0;bad.checksum=ShipyardChecksum(bad);assert(!restored.restore(bad));
 bad=saved;bad.cells[0]=static_cast<uint8_t>(Part::Armor);bad.checksum=ShipyardChecksum(bad);assert(!restored.restore(bad));
 assert(restored.remove(4,1));assert(restored.remove(3,0));
 Shipyard invalid;assert(invalid.remove(2,3));assert(!invalid.ready());
 assert(invalid.place(2,3,Part::Mount)&&invalid.ready());
 // Overloading a power bus keeps the editor functional, but blocks launch.
 assert(invalid.place(2,2,Part::Shield));
 assert(invalid.place(4,2,Part::Cargo));
 assert(invalid.place(4,4,Part::Cargo));
 assert(!invalid.ready());
 assert(invalid.remove(4,4)&&invalid.ready());
 auto damaged=restored.snapshot();damaged.checksum^=1u;
 assert(!restored.restore(damaged));
 std::cout<<"shipyard structural and save tests passed\n";
}
