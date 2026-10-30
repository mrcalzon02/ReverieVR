#include "../../main/jni/native/reverie_scene_document.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace reverie::authoring;
static void rejects(const std::string& input) {bool failed=false;try{ParseScene(input);}catch(const std::invalid_argument&){failed=true;}assert(failed);}
int main(int argc,char**argv){
    const std::string base="REVSCENE\t1\nenvironment\tforest_glade\npiece\trock-1\tbox\t-\t1.0\t2.0\t3.0\t45.0\t1.5\npiece\tcolumn\tcylinder\t-\t-3.0\t1.0\t2.0\t0.0\t2.0\n";
    const Scene s=ParseScene(base);
    assert(s.environment=="forest_glade"&&s.pieces.size()==2);
    const auto mesh=PrimitiveTriangles(s);
    assert(mesh.size()==(36u+16u*12u)*5u);
    for(float v:mesh)assert(std::isfinite(v));
    assert(std::abs(mesh[0]-1.0f)<.01f);
    rejects("");rejects("REVSCENE\t2\nenvironment\twhite_room\n");
    rejects("REVSCENE\t1\nenvironment\twrong\n");
    rejects("REVSCENE\t1\nenvironment\twhite_room\npiece\tbad\tbox\t-\tNaN\t0\t0\t0\t1\n");
    rejects("REVSCENE\t1\nenvironment\twhite_room\npiece\tbad\tobj\t../escape.obj\t0\t0\t0\t0\t1\n");
    rejects("REVSCENE\t1\nenvironment\twhite_room\npiece\tbad\tbox\t-\t0\t0\t0\t0\t0\n");
    rejects("REVSCENE\t1\nenvironment\twhite_room\npiece\tbad\tbox\t-\t0\t0\t0\t0\t1\npiece\tbad\tbox\t-\t0\t0\t0\t0\t1\n");
    rejects(base+"garbage\n");rejects(base.substr(0,base.size()-1));
    assert(!SafeAsset("../x.obj")&&!SafeAsset("C:/foo.obj")&&SafeAsset("app/src/main/assets/models/foo.OBJ"));
    const auto sceneWithObj=ParseScene("REVSCENE\t1\nenvironment\twhite_room\npiece\tart\tobj\tmodels/test.obj\t0\t0\t0\t0\t1\n");
    bool objBlocked=false;try{PrimitiveTriangles(sceneWithObj);}catch(const std::invalid_argument&){objBlocked=true;}assert(objBlocked);
    std::string many="REVSCENE\t1\nenvironment\twhite_room\n";
    for(int i=0;i<513;i++)many+="piece\tr"+std::to_string(i)+"\tbox\t-\t0\t0\t0\t0\t1\n";
    rejects(many);rejects(std::string(150001,'x'));
    if(argc==2){
      // Parse ACTUAL desktop-Java-produced scene file (cross-language fixture).
      FILE *f=std::fopen(argv[1],"rb");assert(f);char buf[150002];size_t n=std::fread(buf,1,sizeof(buf),f);std::fclose(f);
      auto fromJava=ParseScene(buf,n);assert(fromJava.pieces.size()==2);
      assert(PrimitiveTriangles(fromJava).size()==mesh.size());
    }
    if(argc==3){
      FILE *f=std::fopen(argv[2],"rb");assert(f);char buf[150002];size_t n=std::fread(buf,1,sizeof(buf),f);std::fclose(f);
      auto installed=ParseScene(buf,n);assert(installed.environment=="white_room");
      assert(installed.pieces.size()==30);
      bool hasChair=false,hasBeacon=false,hasRotor=false,hasOrb=false;
      bool hasMaterial=false,hasDoor=false;
      size_t expected=0;
      for(const auto &piece:installed.pieces){
          assert(piece.kind=="box"||piece.kind=="cylinder");
          expected+=(piece.kind=="box"?36u:16u*12u)*5u;
          hasChair|=piece.id=="chair-seat";
          hasBeacon|=piece.id=="beacon-light";
          hasRotor|=piece.id=="spinner-arm";
          hasOrb|=piece.id=="moving-orb";
          hasMaterial|=piece.id=="sample-glass";
          hasDoor|=piece.id=="door-crossbar";
      }
      assert(hasChair&&hasBeacon&&hasRotor&&hasOrb&&hasMaterial&&hasDoor);
      const auto installedMesh=PrimitiveTriangles(installed);
      assert(installedMesh.size()==expected);
      for(float value:installedMesh)assert(std::isfinite(value));
    }
    std::cout<<"REVSCENE native: PASS strict parse, safety, GLES2 mesh, native/Java fixture\n";
}