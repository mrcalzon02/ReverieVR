#include "sitka_helm.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
using namespace reverie::sitka;
static bool near(double a,double b){return std::abs(a-b)<1.e-6;}
int main(){
    HelmProgress progress;
    auto layout=AssembleHelm(progress);
    assert(layout.count==3u&&layout.deck.Contains(0.,-.5));
    uint32_t money=20000u;
    assert(!progress.Purchase(HelmItem::Sonar,money));
    assert(progress.Purchase(HelmItem::Battery,money));
    assert(progress.Purchase(HelmItem::Sonar,money));
    assert(progress.Purchase(HelmItem::Gps,money));
    assert(!progress.Purchase(HelmItem::Sonar,money));
    assert(!progress.Purchase(HelmItem::Roof,money));
    assert(progress.UpgradeHull(money));
    assert(progress.Purchase(HelmItem::Roof,money));
    assert(progress.Purchase(HelmItem::Radar,money));
    assert(progress.Purchase(HelmItem::Overhead,money));
    assert(progress.Purchase(HelmItem::MastCamera,money));
    assert(progress.Purchase(HelmItem::RodRacks,money));
    layout=AssembleHelm(progress);
    assert(layout.count==11u&&std::string(layout.mounts[1].model)=="steering-column-and-wheel");
    DeckPassenger person;
    assert(person.Walk(.5,-1.1,layout.deck));
    assert(!person.Walk(5.,0.,layout.deck));
    BoatPose boat;boat.east=1000.;boat.north=2000.;
    boat.heading=1.5707963267948966;
    auto p=person.World(boat);
    assert(near(p[0],998.9)&&near(p[1],1999.5));
    assert(progress.UpgradeHull(money));
    assert(progress.Purchase(HelmItem::Outriggers,money));
    assert(!progress.Purchase(HelmItem::Winch,money));
    assert(progress.UpgradeHull(money));
    assert(progress.Purchase(HelmItem::Winch,money));
    assert(!progress.UpgradeHull(money));
    FishingSession save;
    save.fishing.journal.credits=10000u;
    assert(!PurchaseHelm(save,HelmItem::Sonar));
    assert(PurchaseHelm(save,HelmItem::Battery));
    assert(PurchaseHelm(save,HelmItem::Sonar));
    assert(PurchaseHull(save));
    assert(save.dirty&&save.fishing.journal.hull_tier==1u);
    const std::string version2=SaveJournal(save.fishing.journal);
    Journal recovered;
    assert(LoadJournal(version2,recovered));
    assert(recovered.helm_upgrades==save.fishing.journal.helm_upgrades);
    assert(recovered.credits==save.fishing.journal.credits);
    assert(recovered.hull_tier==1u);
    Journal legacy;
    assert(LoadJournal("SITKA-FISH-1\n123\n900\n1\n1\n1\n0\n0\n0\n",legacy));
    assert(legacy.credits==900u&&legacy.gear==1u);
    assert(legacy.hull_tier==0u&&legacy.helm_upgrades==0u);
    assert(!LoadJournal(version2+"corrupt",recovered));
    assert(recovered.helm_upgrades==save.fishing.journal.helm_upgrades);
    HelmWheel wheel;
    WheelHand left{.2,0.,0.,true},right{-.2,0.,0.,false};
    wheel.Update(left,right);left={0.,.2,0.,true};
    auto fb=wheel.Update(left,right);
    assert(fb.ticks>0u&&wheel.RudderTarget()>0.);
    const double angle=wheel.angle;
    right.grip=true;wheel.Update(left,right);
    assert(near(wheel.angle,angle));
    right.x=20.;assert(wheel.Update(left,right).detach);
    HelmRudder a,b;
    for(int i=0;i<30;++i)a.Advance(.1,1.);
    for(int i=0;i<60;++i)b.Advance(.05,1.);
    assert(near(a.degrees,b.degrees)&&near(a.degrees,35.));
    a.Advance(.1,NAN);assert(near(a.degrees,35.));
    Weather weather;weather.wind=5.f;boat.launched=true;
    const DepthReading depth{DepthStatus::Safe,12.,11.5,BedQuality::Procedural};
    const auto readout=HelmReadouts(progress,boat,weather,depth,18.,.5,.73);
    assert(readout.sonar&&readout.gps&&readout.weather_radar&&readout.overhead);
    assert(readout.sounder[0]=="DEPTH 12.0 m");
    assert(readout.sounder[1].find("PROCEDURAL")!=std::string::npos);
    assert(near(readout.rpm,1150.));
    const auto bare=HelmReadouts(HelmProgress{},boat,weather,depth,0.,0.,1.);
    assert(!bare.sonar&&!bare.overhead&&bare.sounder[0].empty());
    std::cout<<"PASS Sitka helm progression, wheel, deck and upgrade-gated readouts\n";
}
