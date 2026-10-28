#ifndef REVERIE_SCENE_DOCUMENT_H
#define REVERIE_SCENE_DOCUMENT_H
// Portable, untrusted-input-safe REVSCENE\t1 reader shared by native VR and desktop hosts.
// Matches tools/desktop/SceneDocument.java; deliberately does not load arbitrary OBJ files.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace reverie { namespace authoring {
constexpr size_t kSceneMaxBytes=150000u;
constexpr size_t kSceneMaxPieces=512u;
struct Piece {
    std::string id, kind, asset;
    double x=0, y=0, z=0, yaw=0, scale=1;
};
struct Scene {
    std::string environment;
    std::vector<Piece> pieces;
};
inline bool ValidId(const std::string &v) {
    if(v.empty()||v.size()>48) return false;
    for(unsigned char c:v) if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
        (c>='0'&&c<='9')||c=='-'||c=='_')) return false;
    return true;
}
inline bool SafeAsset(const std::string &name) {
    if(name.empty()||name.size()>256||name[0]=='/'||name[0]=='\\'||
       name.find('\\')!=std::string::npos||name.find(':')!=std::string::npos||
       name.find('\r')!=std::string::npos||name.find('\n')!=std::string::npos||
       name.find('\t')!=std::string::npos) return false;
    // Same case-insensitive extension check as Java. Native assets are not loaded here.
    if(name.size()<4) return false;
    auto ext=name.substr(name.size()-4);
    for(char &c:ext) if(c>='A'&&c<='Z') c=char(c-'A'+'a');
    if(ext!=".obj") return false;
    size_t start=0;
    for(size_t i=0;i<=name.size();++i){
        if(i==name.size()||name[i]=='/') {
            const auto seg=name.substr(start,i-start);
            if(seg==".."||seg=="."||seg.empty()) return false;
            start=i+1;
        }
    }
    return true;
}
inline std::vector<std::string> Fields(const std::string &line) {
    std::vector<std::string> result;
    size_t pos=0;
    for(;;){
        auto e=line.find('\t',pos);
        if(e==std::string::npos) {result.push_back(line.substr(pos));return result;}
        result.push_back(line.substr(pos,e-pos));pos=e+1;
    }
}
inline double Number(const std::string &token) {
    if(token.empty())throw std::invalid_argument("empty numeric field");
    // Java Double.parseDouble accepts whitespace; reject instead of silently accepting
    // a forged or noncanonical scene record. The Java writer never emits whitespace.
    for(unsigned char c:token) if(c==' '||c=='\t'||c=='\r'||c=='\n')
        throw std::invalid_argument("whitespace in number");
    errno=0; char *end=nullptr;
    const double value=std::strtod(token.c_str(),&end);
    if(errno==ERANGE||end==token.c_str()||*end!='\0'||!std::isfinite(value)||
       std::abs(value)>100000.) throw std::invalid_argument("invalid transform");
    return value;
}
inline Scene ParseScene(const char *data,size_t bytes) {
    if((data==nullptr&&bytes!=0)||bytes>kSceneMaxBytes) throw std::invalid_argument("scene too large or null");
    if(data!=nullptr&&std::memchr(data,'\0',bytes)!=nullptr) throw std::invalid_argument("NUL byte in scene");
    const std::string source(data!=nullptr?data:"",bytes);
    size_t start=0;size_t line_number=0;
    Scene scene;std::unordered_set<std::string> ids;
    while(start<source.size()){
        const auto end=source.find('\n',start);
        const std::string line=source.substr(start,end==std::string::npos?end:end-start);
        ++line_number;
        if(line_number==1) {
            if(line!="REVSCENE\t1")throw std::invalid_argument("scene magic/version");
        } else if(line_number==2) {
            const auto env=Fields(line);
            if(env.size()!=2||env[0]!="environment"||
                !(env[1]=="white_room"||env[1]=="forest_glade"||env[1]=="dune_beach"))
                throw std::invalid_argument("scene environment");
            scene.environment=env[1];
        } else {
            const auto f=Fields(line);
            if(f.size()!=9||f[0]!="piece"||scene.pieces.size()>=kSceneMaxPieces)
                throw std::invalid_argument("invalid scene piece");
            Piece p; p.id=f[1];p.kind=f[2];p.asset=f[3];
            if(!ValidId(p.id)||!ids.insert(p.id).second||
               !(p.kind=="box"||p.kind=="cylinder"||p.kind=="obj")||
               (p.kind=="obj"?!SafeAsset(p.asset):p.asset!="-"))
                throw std::invalid_argument("unsafe scene piece");
            p.x=Number(f[4]);p.y=Number(f[5]);p.z=Number(f[6]);
            p.yaw=Number(f[7]);p.scale=Number(f[8]);
            if(p.scale<=0.||p.scale>500.)throw std::invalid_argument("invalid scale");
            scene.pieces.push_back(std::move(p));
        }
        if(end==std::string::npos)break;
        start=end+1;
    }
    if(line_number<2||scene.environment.empty()||source.empty()||source.back()!='\n')
        throw std::invalid_argument("incomplete scene");
    return scene;
}
inline Scene ParseScene(const std::string &bytes) {return ParseScene(bytes.data(),bytes.size());}

// GLES2-ready interleaved XYZUV triangles, generated ONCE during GL context creation.
// OBJ authoring references are deliberately unsupported until a licensed mesh importer exists.
// Primitive shapes exactly match the Java desktop workbench proportions and yaw transform.
inline std::vector<float> PrimitiveTriangles(const Scene &scene) {
    std::vector<float> output;
    output.reserve(scene.pieces.size()*16u*12u*3u*5u);
    auto quad=[&](const Piece &p,const std::array<double,3> &a,const std::array<double,3> &b,
                  const std::array<double,3> &c,const std::array<double,3> &d){
        const std::array<std::array<double,3>,6> v={a,b,c,a,c,d};
        const float uvs[][2]={{0,0},{1,0},{1,1},{0,0},{1,1},{0,1}};
        const double angle=p.yaw*3.14159265358979323846/180.,co=std::cos(angle),si=std::sin(angle);
        for(size_t i=0;i<v.size();++i){
            const double x=p.x+p.scale*(v[i][0]*co-v[i][2]*si);
            const double y=p.y+p.scale*v[i][1];
            const double z=p.z+p.scale*(v[i][0]*si+v[i][2]*co);
            if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||
               std::abs(x)>1.e8||std::abs(y)>1.e8||std::abs(z)>1.e8)
                throw std::invalid_argument("geometry exceeds coordinate limits");
            output.insert(output.end(),{float(x),float(y),float(z),uvs[i][0],uvs[i][1]});
        }
    };
    auto triangle=[&](const Piece &p,const std::array<double,3> &a,
                      const std::array<double,3> &b,const std::array<double,3> &c) {
        // Reuse quad mapping but do not generate degenerate cap triangles.
        const std::array<std::array<double,3>,3> points={a,b,c};
        const float uv[][2]={{.5f,.5f},{0.f,0.f},{1.f,0.f}};
        const double angle=p.yaw*3.14159265358979323846/180.,co=std::cos(angle),si=std::sin(angle);
        for(size_t i=0;i<points.size();++i){
            const double x=p.x+p.scale*(points[i][0]*co-points[i][2]*si);
            const double y=p.y+p.scale*points[i][1];
            const double z=p.z+p.scale*(points[i][0]*si+points[i][2]*co);
            if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))
                throw std::invalid_argument("geometry nonfinite");
            output.insert(output.end(),{float(x),float(y),float(z),uv[i][0],uv[i][1]});
        }
    };
    for(const auto &p:scene.pieces){
        if(p.kind=="obj")throw std::invalid_argument("OBJ preview needs an explicit licensed runtime mesh importer: "+p.id);
        if(p.kind=="box"){
            const std::array<std::array<double,3>,8> pt={{{-1,0,-1},{1,0,-1},{1,0,1},{-1,0,1},
                {-1,2,-1},{1,2,-1},{1,2,1},{-1,2,1}}};
            const int faces[][4]={{0,3,2,1},{4,5,6,7},{0,1,5,4},
                {1,2,6,5},{2,3,7,6},{3,0,4,7}};
            for(auto &face:faces)quad(p,pt[face[0]],pt[face[1]],pt[face[2]],pt[face[3]]);
        } else if(p.kind=="cylinder") {
            for(int i=0;i<16;++i){
                const double a=2.*3.14159265358979323846*i/16.,b=2.*3.14159265358979323846*(i+1)/16.;
                const std::array<double,3> l={std::cos(a),0,std::sin(a)},r={std::cos(b),0,std::sin(b)};
                quad(p,l,r,{r[0],2,r[2]},{l[0],2,l[2]});
                triangle(p,{0,0,0},r,l);
                triangle(p,{0,2,0},{l[0],2,l[2]},{r[0],2,r[2]});
            }
        } else throw std::invalid_argument("unknown primitive kind");
    }
    return output;
}
}} // namespace reverie::authoring
#endif