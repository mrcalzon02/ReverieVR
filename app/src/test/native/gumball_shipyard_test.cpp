#include "gumball_shipyard.h"
#include <cassert>
#include <iostream>
#include <string>
using namespace reverie::gumball;
int main(){
 Shipyard s;assert(s.ready());assert(s.count(Part::Core)==1);assert(s.stock(Part::Armor)==3);
 assert(s.profile().mounts==1);
 assert(s.place(2,2,Part::Mount));assert(s.profile().mounts==2);
 assert(s.profile().muzzle_y[0]!=s.profile().muzzle_y[1]);
 assert(s.remove(2,2));
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
 // Real editor click semantics: selecting a new stocked part must replace
 // an occupied module in ONE click, with returned old stock, not erase it.
 Shipyard editor;
 assert(editor.at(3,2)==Part::Frame);
 const auto original=editor.snapshot();
 assert(editor.edit(3,2,Part::Armor)==ShipEdit::Replaced);
 assert(editor.at(3,2)==Part::Armor);
 assert(editor.stock(Part::Frame)==original.stock[static_cast<unsigned>(Part::Frame)]+1u);
 assert(editor.stock(Part::Armor)==original.stock[static_cast<unsigned>(Part::Armor)]-1u);
 assert(editor.connected());
 assert(editor.edit(3,2,Part::Frame)==ShipEdit::Replaced);
 assert(editor.at(3,2)==Part::Frame);
 assert(editor.stock(Part::Frame)==original.stock[static_cast<unsigned>(Part::Frame)]);
 assert(editor.stock(Part::Armor)==original.stock[static_cast<unsigned>(Part::Armor)]);
 assert(editor.edit(3,3,Part::Armor)==ShipEdit::CoreLocked);
 assert(editor.edit(0,0,Part::Armor)==ShipEdit::NotConnected);
 assert(editor.edit(-1,0,Part::Armor)==ShipEdit::Invalid);
 // An occupied bridge cannot be deleted: the thruster above depends on it.
 assert(editor.edit(3,2,Part::Frame)==ShipEdit::NotConnected);
 // A non-bridge newly attached cell can be toggled off with the same brush.
 assert(editor.edit(4,2,Part::Armor)==ShipEdit::Placed);
 assert(editor.edit(4,2,Part::Armor)==ShipEdit::Removed);
 assert(editor.snapshot().checksum==original.checksum);
 assert(editor.ready());
 // Exhausted stock rejects without touching part ownership or layout.
 Shipyard full;
 assert(full.edit(4,3,Part::Armor)==ShipEdit::Replaced);
 assert(full.edit(4,2,Part::Armor)==ShipEdit::Replaced);
 assert(full.edit(2,2,Part::Armor)==ShipEdit::Placed);
 assert(full.stock(Part::Armor)==0u);
 const auto fullySaved=full.snapshot();
 assert(full.edit(3,2,Part::Armor)==ShipEdit::NoStock);
 assert(full.snapshot().checksum==fullySaved.checksum);
 assert(full.restore(fullySaved));
 assert(full.connected());
 assert(std::string(ShipEditMessage(ShipEdit::CoreLocked))=="CORE IS FIXED");
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
 // Deterministic layout property stress: no disconnected or duplicating ship can survive.
 Shipyard stress;
 uint32_t rng=0xF00DBA11u;
 for(int tick=0;tick<30000;++tick){
     rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
     const int x=static_cast<int>((rng>>3)%7u),y=static_cast<int>((rng>>9)%7u);
     const Part part=static_cast<Part>(2u+((rng>>18)%9u));
     if(rng&1u)stress.place(x,y,part);else stress.remove(x,y);
     assert(stress.connected());
     const auto snapshot=stress.snapshot();
     Shipyard copy;
     assert(copy.restore(snapshot));
     for(int gy=0;gy<7;++gy)for(int gx=0;gx<7;++gx)assert(stress.at(gx,gy)==copy.at(gx,gy));
     if(stress.ready()){
         const auto profile=stress.profile();
         assert(profile.power>=profile.demand);
         assert(profile.mass<=18*stress.count(Part::Thruster));
         assert(profile.mounts>=1&&profile.mounts<=2);
     }
 }
 std::cout<<"shipyard structural and save tests passed\n";
}
