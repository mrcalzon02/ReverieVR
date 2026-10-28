#include "../../main/jni/native/omega_front_continent.h"
#include <cassert>
#include <iostream>
#include <queue>
using namespace reverie::omega;
int main(){
  Continent a(414),b(414),c(99);
  assert(kContinentCells==600);
  int land=0,sea=0,river=0,high=0,trees=0,roads=0,variant=0;
  for(int y=0;y<kContinentHeight;++y)for(int x=0;x<kContinentWidth;++x){
    auto t=a.At(x,y).terrain;
    assert(t==b.At(x,y).terrain);
    if(t!=c.At(x,y).terrain)++variant;
    if(t==ContinentTerrain::Sea)++sea; else ++land;
    if(t==ContinentTerrain::River)++river;
    if(t==ContinentTerrain::Highland||t==ContinentTerrain::Snow)++high;
    if(t==ContinentTerrain::Forest)++trees;
    if(t==ContinentTerrain::Road)++roads;
  }
   std::cerr<<"terrain counts land="<<land<<" sea="<<sea<<" river="<<river<<" high="<<high<<" forest="<<trees<<" roads="<<roads<<" variant="<<variant<<"\n"; assert(land>260&&sea>60&&river>4&&high>12&&trees>12&&roads>10&&variant>15);
  assert(a.MarchRations(0)==2); // Chalkwatch barren escarpment
  assert(a.MarchRations(2)==2); // Splinter Ridge
  assert(a.MarchRations(5)==1); // White Orchard supply road
  assert(a.MarchRations(6)==1); // Hearthwell
  assert(a.MarchRations(12)==2); // Spoor Nest badlands
  assert(a.MarchRations(16)==0);
  for(int s=0;s<16;++s){
    assert(Continent::SectorAt(Continent::SectorX(s),Continent::SectorY(s))==s);
    assert(a.At(Continent::SectorX(s),Continent::SectorY(s)).terrain!=ContinentTerrain::Sea);
    assert(!Continent::AdjacentHearthfall(s,s));
  }
  std::array<bool,16> seen{};std::queue<int> q;q.push(6);seen[6]=true;
  while(!q.empty()){
    int v=q.front();q.pop();
    for(int n=0;n<16;++n)if(!seen[n]&&Continent::AdjacentHearthfall(v,n)){
      seen[n]=true;q.push(n);
    }
  }
  for(bool known:seen)assert(known);
  std::cout<<"Omega continent 600 cells / 16 stable Hearthfall sectors: PASS\n";
}