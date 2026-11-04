#include "../../main/jni/native/reverie_vr_workshop.h"
#include "../../main/jni/native/reverie_native_spatial_sound_queue.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace reverie::authoring;
struct Memory {
  std::array<std::string,3> bytes{};
  std::array<bool,3> has{};
  bool reject=false;
  static int Read(void* context,size_t i,std::string& out){
    auto& m=*static_cast<Memory*>(context);
    if(m.reject)return -1;
    if(!m.has.at(i))return 0;
    out=m.bytes.at(i);return 1;
  }
  static bool Write(void* context,size_t i,const std::string& bytes){
    auto& m=*static_cast<Memory*>(context);
    if(m.reject)return false;
    m.bytes.at(i)=bytes;m.has.at(i)=true;return true;
  }
  VrStore store(){return {this,Read,Write};}
};
static bool bad(const std::string& x){
  try{(void)DecodeVrBundle(x);return false;}
  catch(const std::exception&){return true;}
}
int main(){
  const Scene seed{"white_room",{
    Piece{"speaker","box","-",1.,-1.5,-3.,0.,.5},
    Piece{"other","cylinder","-",-1.,-1.5,-2.,0.,.25}
  }};
  Memory m;VrWorkshop w;
  assert(w.Open(seed,m.store()));
  assert(!w.AttachSound());
  w.editing=true;w.selected=0;
  assert(w.AttachSound()&&w.sound.enabled);
  assert(w.sound.anchor_id=="speaker"&&w.sound.cue==11u);
  assert(w.TuneSound(0,true)&&w.sound.cue==12u);
  assert(w.TuneSound(1,false)&&w.sound.interval==2.5);
  assert(w.TuneSound(2,false)&&w.sound.radius==5.);
  assert(w.Undo()&&w.sound.radius==6.);
  uint32_t cue=0;VrStereoCue stereo;
  assert(!w.TickSound(.05,2.,0.,0.,0.,1.,0.,cue,stereo)); // editing silent
  w.editing=false;
  for(int i=0;i<49;++i)
      assert(!w.TickSound(.05,2.,0.,0.,0.,1.,0.,cue,stereo));
  assert(w.TickSound(.05,2.,0.,0.,0.,1.,0.,cue,stereo));
  assert(cue==12u&&stereo.audible&&stereo.right>stereo.left);
  assert(!w.TickSound(.05,2.,0.,0.,0.,1.,0.,cue,stereo)); // bounded rate
  w.editing=true;
  assert(w.Save(m.store())&&!w.dirty);
  assert(m.bytes[0].compare(0,12,"RVPROJECT\t3\n")==0);
  assert(DecodeVrBundle(m.bytes[0]).sound.cue==12u);
  VrWorkshop restored;assert(restored.Open(seed,m.store()));
  assert(restored.sound.enabled&&restored.sound.anchor_id=="speaker");
  restored.editing=true;restored.selected=0;
  assert(restored.Remove()&&!restored.sound.enabled);
  assert(restored.Undo()&&restored.sound.enabled);
  assert(restored.ClearSound()&&!restored.sound.enabled);
  assert(restored.Undo()&&restored.sound.enabled);
  assert(!ValidVrSound(seed,VrSound{true,"absent",11u,2.,6.}));
  assert(!ValidVrSound(seed,VrSound{true,"speaker",22u,2.,6.}));
  assert(!ValidVrSound(seed,VrSound{true,"speaker",11u,0.,6.}));
  assert(!ValidVrSound(seed,VrSound{true,"speaker",11u,2.,20.}));
  assert(bad(EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
                           EncodeVrLight(seed,{}),
                           "RVSOUND\t1\nsound\tmissing\t11\t3\t6\n")));
  assert(bad(EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
                           EncodeVrLight(seed,{}),
                           "RVSOUND\t1\nsound\tspeaker\t99\t3\t6\n")));
  assert(bad(m.bytes[0]+"X"));
  // V2 light projects must remain loadable, then be written in V3.
  m.bytes[1]=EncodeVrBundle(EncodeVrScene(seed),EncodeMotions(seed,{}),
                            EncodeVrLight(seed,VrLight{true,"speaker",1.,5.,.65,.35}));
  m.has[1]=true;
  assert(restored.NextProject(m.store())&&restored.slot==1);
  assert(!restored.sound.enabled&&restored.light.enabled);
  assert(restored.Save(m.store())&&m.bytes[1].compare(0,12,"RVPROJECT\t3\n")==0);
  // Storage failures keep project, anchor and changes in memory.
  restored.selected=0;assert(restored.AttachSound());
  m.reject=true;assert(!restored.NextProject(m.store())&&restored.slot==1);
  assert(restored.sound.enabled&&restored.dirty);
  m.reject=false;assert(restored.Save(m.store()));
  // Pure panning and distance at a known listener/right vector.
  auto right=SpatializeVrSound(2.,0.,0.,0.,0.,0.,1.,0.,6.);
  auto left=SpatializeVrSound(-2.,0.,0.,0.,0.,0.,1.,0.,6.);
  assert(right.audible&&left.audible&&right.right>right.left);
  assert(left.left>left.right);
  assert(!SpatializeVrSound(20.,0.,0.,0.,0.,0.,1.,0.,6.).audible);
  assert(!SpatializeVrSound(2.,0.,0.,0.,0.,0.,0.,0.,6.).audible);
  ReverieNativeSpatialSoundQueue queue;
  assert(queue.Push(12u,right.left,right.right));
  assert(queue.Push(11u,left.left,left.right));
  assert(!queue.Push(11u,1.f,1.f)); // 2-event rate cap
  const uint64_t packet=queue.Drain();
  assert((packet&255u)==12u&&((packet>>24u)&255u)==11u);
  assert(queue.Drain()==0u);
  assert(!queue.Push(25u,1.f,1.f));
  assert(!queue.Push(11u,-1.f,1.f));
  std::cout<<"PASS VR sound cue binding, V3 save/migration, stereo distance, rate cap and undo\n";
}
