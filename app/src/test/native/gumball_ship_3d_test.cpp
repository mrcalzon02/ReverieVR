#include "gumball_ship_3d.h"
#include "gumball_space_3d.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace reverie::gumball;
int main(){
    Shipyard legacy;
    ShipAssembly ship;
    assert(ship.ready());
    assert(ship.connected());
    assert(ship.count(Part::Core)==1);
    assert(ship.count(Part::Mount)==1);
    assert(ship.at(3,3,2)==Part::Core);
    for(int z=0;z<kShipDepth;++z)
        for(int y=0;y<7;++y)for(int x=0;x<7;++x)
            assert(ship.at(x,y,z)==
                   (z==2?legacy.at(x,y):Part::Empty));
    assert(ship.profile().mass==legacy.profile().mass);
    assert(ship.profile().power==legacy.profile().power);
    assert(ship.profile().mounts==legacy.profile().mounts);
    assert(ship.edit(3,3,2,Part::Armor)==ShipEdit::CoreLocked);
    assert(ship.edit(0,0,0,Part::Armor)==ShipEdit::NotConnected);
    assert(ship.stock(Part::Armor)==3u);
    // Connected placement in Z changes the actually saved volume.
    assert(ship.edit(3,3,3,Part::Armor)==ShipEdit::Placed);
    assert(ship.at(3,3,3)==Part::Armor);
    assert(ship.stock(Part::Armor)==2u);
    assert(ship.edit(3,3,4,Part::Frame)==ShipEdit::Placed);
    assert(ship.edit(3,3,3,Part::Armor)==ShipEdit::NotConnected);
    assert(ship.edit(3,3,4,Part::Frame)==ShipEdit::Removed);
    assert(ship.edit(3,3,3,Part::Armor)==ShipEdit::Removed);
    assert(ship.stock(Part::Armor)==3u);
    assert(ship.ready());
    // Atomic replacement refunds stock and leaves no intermediate break.
    assert(ship.edit(3,4,2,Part::Wing)==ShipEdit::Replaced);
    assert(!ship.ready());  // deleted last thruster
    assert(ship.edit(3,4,2,Part::Thruster)==ShipEdit::Replaced);
    assert(ship.ready());
    const Ship3DSave good=ship.snapshot();
    assert(good.version==2u&&good.magic==0x33424853u);
    assert(good.checksum==Ship3DChecksum(good));
    ShipAssembly restored;
    assert(restored.restore(good));
    assert(restored.ready());
    Ship3DSave damaged=good;
    damaged.cells[3+3*7+2*49]=0;
    damaged.checksum=Ship3DChecksum(damaged);
    assert(!restored.restore(damaged));
    damaged=good;
    damaged.stock[static_cast<unsigned>(Part::Armor)]++;
    damaged.checksum=Ship3DChecksum(damaged);
    assert(!restored.restore(damaged));
    damaged=good;
    damaged.cells[3+3*7+4*49]=static_cast<uint8_t>(Part::Armor);
    damaged.checksum=Ship3DChecksum(damaged);
    assert(!restored.restore(damaged));
    damaged=good;
    damaged.checksum^=0x1234u;
    assert(!restored.restore(damaged));
    // Former 72-byte format remains readable without changing its checksum.
    Shipyard modifiedLegacy;
    assert(modifiedLegacy.edit(4,3,Part::Armor)==ShipEdit::Replaced);
    Shipyard checkLegacy;
    assert(checkLegacy.restore(modifiedLegacy.snapshot()));
    ShipAssembly migrated;
    migrated.import_legacy(checkLegacy);
    assert(migrated.at(4,3,2)==Part::Armor);
    assert(migrated.stock(Part::Reactor)==checkLegacy.stock(Part::Reactor));
    assert(migrated.restore(migrated.snapshot()));
    // Every Sour enemy kind uses the same 3D connected ship components.
    for(unsigned kind=0;kind<13u;++kind){
        for(uint32_t variant=0;variant<3u;++variant){
            auto enemy=ShipAssembly::enemy(kind,variant);
            assert(enemy.connected());
            assert(enemy.ready());
            assert(enemy.profile().power>=enemy.profile().demand);
            assert(enemy.count(Part::Core)==1);
            assert(enemy.count(Part::Thruster)>=1);
            assert(enemy.count(Part::Mount)>=1);
            assert(enemy.count(Part::Reactor)>=1);
            std::vector<float> a,b;
            AppendAssemblyMesh(a,enemy,0.f,0.f,-4.f,.1f,0.f);
            AppendAssemblyMesh(b,enemy,0.f,0.f,-4.f,.1f,0.f);
            assert(!a.empty()&&a==b&&a.size()%18u==0u);
            for(float v:a)assert(std::isfinite(v));
        }
    }
    // A perspective lane moves its world-space Z positions as time passes.
    std::vector<float> scene0,scene1;
    AppendSpaceLane(scene0,0.f,42u);
    AppendSpaceLane(scene1,2.f,42u);
    assert(!scene0.empty()&&scene0.size()==scene1.size());
    assert(scene0!=scene1&&scene0.size()%18u==0u);
    for(float v:scene1)assert(std::isfinite(v));
    std::cout<<"Gumball 3D assembly, voxel edits, migration, enemy meshes and scrolling passed\n";
}
