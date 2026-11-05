#ifndef REVERIE_SITKA_WALK_GRAPH_H
#define REVERIE_SITKA_WALK_GRAPH_H
// Sitka source-only pedestrian corridor contract. Nothing is generated
// from names, tourist maps, proximity or shore polygons. Every edge and
// fishing stance requires approved georeferenced authoring. Not a promise
// that real sidewalks are public or wheelchair-accessible.
#include "sitka_terrain_mesh.h"
#include "sitka_fishing_spots.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace reverie { namespace sitka {
constexpr size_t kMaxWalkBytes=262144u;
constexpr size_t kMaxWalkNodes=4096u;
constexpr size_t kMaxWalkEdges=8192u;
constexpr size_t kMaxWalkStances=128u;
struct WalkNode {std::string id; float x=0,y=0,z=0;};
struct WalkEdge {std::string id; size_t a=0,b=0;float width=0;std::string kind;};
struct WalkStance {std::string id, spot_id;size_t node=0;float radius=0;};
struct WalkSurface {bool valid=false;float y=0;size_t edge=0;float distance=0;};

inline std::vector<std::string> WalkFields(const std::string& line){
    std::vector<std::string> f; size_t pos=0;
    while(true){const size_t end=line.find('|',pos);
        f.push_back(line.substr(pos,end==std::string::npos?end:end-pos));
        if(end==std::string::npos)break;
        pos=end+1;
    }
    return f;
}
inline bool WalkId(const std::string& id){
    if(id.empty()||id.size()>64u)return false;
    for(const char c:id)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return true;
}
inline float WalkNumber(const std::string& s,float lo,float hi){
    size_t used=0u;float n=0;
    try{n=std::stof(s,&used);}
    catch(const std::exception&){throw std::invalid_argument("Sitka walk numeric field");}
    if(used!=s.size()||!std::isfinite(n)||n<lo||n>hi)
        throw std::invalid_argument("Sitka walk out-of-range numeric field");
    return n;
}
inline double WalkOrigin(const std::string& s){
    size_t used=0u; double d=0.;
    try{d=std::stod(s,&used);}
    catch(const std::exception&){throw std::invalid_argument("Invalid Sitka projected origin");}
    if(used!=s.size()||!std::isfinite(d)||std::abs(d)>1.e8)
        throw std::invalid_argument("Invalid Sitka projected origin");
    return d;
}
inline bool WalkKind(const std::string& s){
    return s=="sidewalk"||s=="seawalk"||s=="trail"||
           s=="shore_path"||s=="crossing"||s=="bridge_walk"||
           s=="steps"||s=="public_pier";
}

class WalkGraph {
public:
    std::vector<WalkNode> nodes;
    std::vector<WalkEdge> edges;
    std::vector<WalkStance> stances;
    size_t spawn=0;
    std::vector<uint8_t> reachable;

    static WalkGraph Parse(const std::string& source,double terrain_east,double terrain_north){
        if(!std::isfinite(terrain_east)||!std::isfinite(terrain_north)||
           source.size()>kMaxWalkBytes)
            throw std::invalid_argument("Sitka walk file too large");
        std::istringstream input(source);std::string line;
        if(!std::getline(input,line)||line!="SITKA-WALK-1")
            throw std::invalid_argument("Sitka walk version");
        WalkGraph graph;std::unordered_map<std::string,size_t> ids;
        std::set<std::string> used_edge,used_stance;
        struct PendingEdge {std::string id,a,b,kind;float width;};
        struct PendingStance {std::string id,node,spot;float radius;};
        std::vector<PendingEdge> pending_edges;
        std::vector<PendingStance> pending_stances;
        std::string origin;
        bool approved=false,projected=false;
        while(std::getline(input,line)){
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            const auto f=WalkFields(line);
            if(f.empty())throw std::invalid_argument("Sitka walk empty row");
            if(f[0]=="N"){
                if(f.size()!=5u||graph.nodes.size()>=kMaxWalkNodes||
                   !WalkId(f[1])||ids.count(f[1]))
                    throw std::invalid_argument("Sitka walk node id/count");
                const WalkNode n{f[1],WalkNumber(f[2],-100000.f,100000.f),
                    WalkNumber(f[3],-2000.f,2000.f),
                    WalkNumber(f[4],-100000.f,100000.f)};
                ids.emplace(n.id,graph.nodes.size());
                graph.nodes.push_back(n);
            }else if(f[0]=="E"){
                if(f.size()!=6u||pending_edges.size()>=kMaxWalkEdges||
                   !WalkId(f[1])||!used_edge.insert(f[1]).second||
                   !WalkId(f[2])||!WalkId(f[3])||!WalkKind(f[4]))
                    throw std::invalid_argument("Sitka walk edge type/id/count");
                pending_edges.push_back({f[1],f[2],f[3],f[4],
                    WalkNumber(f[5],.4f,15.f)});
            }else if(f[0]=="F"){
                if(f.size()!=5u||pending_stances.size()>=kMaxWalkStances||
                   !WalkId(f[1])||!used_stance.insert(f[1]).second||
                   !WalkId(f[2])||!WalkId(f[3]))
                    throw std::invalid_argument("Sitka walk fishing stance id/count");
                pending_stances.push_back({f[1],f[2],f[3],
                    WalkNumber(f[4],.3f,2.5f)});
            }else if(f[0]=="P"){
                if(f.size()!=5u||approved || !WalkId(f[1]) ||
                   f[2].size()!=64u || f[3]!="EPSG32608" || f[4]!="reviewed")
                    throw std::invalid_argument("Sitka pedestrian provenance missing");
                for(char c:f[2])if(!((c>='0'&&c<='9')||
                                  (c>='a'&&c<='f')))
                    throw std::invalid_argument("Bad pedestrian source SHA-256");
                approved=true;
            }else if(f[0]=="O"){
                if(f.size()!=3u || projected ||
                   std::abs(WalkOrigin(f[1])-terrain_east)>.001 ||
                   std::abs(WalkOrigin(f[2])-terrain_north)>.001)
                    throw std::invalid_argument("Sitka walk origin does not match terrain");
                projected=true;
            }else if(f[0]=="S"){
                if(f.size()!=3u||!WalkId(f[1])||!WalkId(f[2])||
                   !origin.empty())
                    throw std::invalid_argument("Sitka walk spawn entry");
                origin=f[2];
            }else throw std::invalid_argument("Sitka walk unsupported record");
        }
        if(!approved||!projected||origin.empty()||ids.count(origin)==0u||pending_edges.empty())
            throw std::invalid_argument("Sitka walk requires sourced spawn and edges");
        graph.spawn=ids.at(origin);
        std::set<std::pair<size_t,size_t>> unique_paths;
        std::vector<std::vector<size_t>> adj(graph.nodes.size());
        for(const auto& p:pending_edges){
            if(ids.count(p.a)==0u||ids.count(p.b)==0u)
                throw std::invalid_argument("Sitka walk dangling edge");
            const size_t a=ids.at(p.a),b=ids.at(p.b);
            if(a==b)throw std::invalid_argument("Sitka walk zero edge");
            const auto& na=graph.nodes[a];const auto& nb=graph.nodes[b];
            const float dx=nb.x-na.x,dz=nb.z-na.z,dy=nb.y-na.y;
            const float len=std::hypot(dx,dz);
            if(len<.1f||len>500.f||
               (p.kind!="steps" && std::abs(dy)>len*.35f)||
               (p.kind=="steps" && std::abs(dy)>len*1.5f))
                throw std::invalid_argument("Sitka walk span/grade invalid");
            const auto key=std::minmax(a,b);
            if(!unique_paths.insert(key).second)
                throw std::invalid_argument("Sitka walk duplicate corridor");
            graph.edges.push_back({p.id,a,b,p.width,p.kind});
            adj[a].push_back(b);adj[b].push_back(a);
        }
        graph.reachable.assign(graph.nodes.size(),0u);
        std::queue<size_t> q;q.push(graph.spawn);
        graph.reachable[graph.spawn]=1u;
        while(!q.empty()){
            const auto a=q.front();q.pop();
            for(size_t b:adj[a])if(!graph.reachable[b]){
                graph.reachable[b]=1u;q.push(b);
            }
        }
        for(const auto& p:pending_stances){
            if(ids.count(p.node)==0u)
                throw std::invalid_argument("Sitka fishing stance dangling node");
            const size_t n=ids.at(p.node);
            if(!graph.reachable[n])
                throw std::invalid_argument("Sitka fishing stance disconnected");
            bool public_edge=false;
            for(const auto& e:graph.edges)
                if(e.a==n||e.b==n){public_edge=true;break;}
            if(!public_edge)
                throw std::invalid_argument("Sitka fishing stance not on corridor");
            graph.stances.push_back({p.id,p.spot,n,p.radius});
        }
        return graph;
    }

    WalkSurface Surface(double x,double z) const{
        WalkSurface best;double dist2=std::numeric_limits<double>::infinity();
        if(!std::isfinite(x)||!std::isfinite(z))return best;
        for(size_t i=0;i<edges.size();++i){
            const auto& e=edges[i];
            if(!reachable[e.a]||!reachable[e.b])continue;
            const auto& a=nodes[e.a];const auto& b=nodes[e.b];
            const double vx=double(b.x)-a.x,vz=double(b.z)-a.z;
            const double d=vx*vx+vz*vz;
            if(d<.01)continue;
            const double t=std::max(0.,std::min(1.,
                ((x-a.x)*vx+(z-a.z)*vz)/d));
            const double dx=x-a.x-vx*t,dz=z-a.z-vz*t;
            const double sq=dx*dx+dz*dz;
            const double limit=e.width*.5+.25;
            if(sq<dist2 && sq<=limit*limit){
                best={true,static_cast<float>(a.y+t*(b.y-a.y)),i,
                      static_cast<float>(std::sqrt(sq))};
                dist2=sq;
            }
        }
        return best;
    }

    // Dijkstra over ONLY authored connections; a visual crossing of two
    // paths never creates a graph junction without the same surveyed node.
    std::vector<size_t> Route(const std::string& from,
                               const std::string& to)const{
        size_t source=nodes.size(),destination=nodes.size();
        for(size_t i=0u;i<nodes.size();++i){
            if(nodes[i].id==from)source=i;
            if(nodes[i].id==to)destination=i;
        }
        if(source==nodes.size()||destination==nodes.size()||
           !reachable[source]||!reachable[destination])return {};
        std::vector<std::vector<std::pair<size_t,double>>> adj(nodes.size());
        for(const auto& e:edges){
            const auto& a=nodes[e.a];const auto& b=nodes[e.b];
            const double d=std::hypot(std::hypot(double(a.x)-b.x,
                                                 double(a.z)-b.z),
                                       double(a.y)-b.y);
            adj[e.a].push_back({e.b,d});
            adj[e.b].push_back({e.a,d});
        }
        std::vector<double> distances(nodes.size(),
                                     std::numeric_limits<double>::infinity());
        std::vector<size_t> parent(nodes.size(),nodes.size());
        using Work=std::pair<double,size_t>;
        std::priority_queue<Work,std::vector<Work>,std::greater<Work>> queue;
        distances[source]=0.;queue.push({0.,source});
        while(!queue.empty()){
            const auto item=queue.top();queue.pop();
            if(item.first>distances[item.second])continue;
            if(item.second==destination)break;
            for(const auto& next:adj[item.second]){
                const double candidate=item.first+next.second;
                if(candidate+1.e-8<distances[next.first]){
                    distances[next.first]=candidate;
                    parent[next.first]=item.second;
                    queue.push({candidate,next.first});
                }
            }
        }
        if(!std::isfinite(distances[destination]))return {};
        std::vector<size_t> out;
        for(size_t n=destination;n!=source;n=parent[n]){
            if(n>=nodes.size()||out.size()>=nodes.size())return {};
            out.push_back(n);
        }
        out.push_back(source);
        std::reverse(out.begin(),out.end());
        return out;
    }

    // No automatic access based on water proximity. A cast requires a
    // sourced walkway, a reachable legal bank/pier node AND the specific
    // approved fishing site. The existing ray/water check remains mandatory.
    bool CanCastFrom(double x,double z,float target_e,float target_n,
                     const std::vector<FishingSpot>& spots)const{
        if(!std::isfinite(target_e)||!std::isfinite(target_n)||
           !Surface(x,z).valid)return false;
        for(const auto& stance:stances){
            const auto& n=nodes[stance.node];
            const double dx=x-n.x,dz=z-n.z;
            if(dx*dx+dz*dz>double(stance.radius)*stance.radius)continue;
            for(const auto& site:spots){
                if(site.id!=stance.spot_id)continue;
                const double a=target_e-site.east,b=target_n-site.north;
                if(a*a+b*b>double(site.radius)*site.radius)continue;
                const double cx=target_e-x,cz=target_n-z;
                if(cx*cx+cz*cz<=35.*35.)return true;
            }
        }
        return false;
    }

    // One flat strip per actual authored segment; no fake city grid.
    TerrainMesh BuildPathMesh()const{
        TerrainMesh mesh;
        for(const auto& e:edges){
            if(!reachable[e.a]||!reachable[e.b])continue;
            const auto& a=nodes[e.a];const auto& b=nodes[e.b];
            const double dx=b.x-a.x,dz=b.z-a.z;
            const double inv=1./std::hypot(dx,dz);
            const float nx=static_cast<float>(-dz*inv*e.width*.5),
                        nz=static_cast<float>(dx*inv*e.width*.5);
            const std::array<uint8_t,3> color=
                e.kind=="shore_path"||e.kind=="trail"?
                    std::array<uint8_t,3>{106,100,83}:
                e.kind=="seawalk"||e.kind=="public_pier"?
                    std::array<uint8_t,3>{112,102,89}:
                e.kind=="crossing"?
                    std::array<uint8_t,3>{171,172,158}:
                    std::array<uint8_t,3>{119,125,119};
            const uint32_t k=static_cast<uint32_t>(mesh.vertices.size());
            // 3 cm lift prevents terrain z-fighting without implying
            // full collision or verified sidewalk construction.
            mesh.vertices.push_back({a.x-nx,a.y+.03f,a.z-nz,color[0],color[1],color[2]});
            mesh.vertices.push_back({a.x+nx,a.y+.03f,a.z+nz,color[0],color[1],color[2]});
            mesh.vertices.push_back({b.x+nx,b.y+.03f,b.z+nz,color[0],color[1],color[2]});
            mesh.vertices.push_back({b.x-nx,b.y+.03f,b.z-nz,color[0],color[1],color[2]});
            mesh.indices.insert(mesh.indices.end(),{k,k+1u,k+2u,k,k+2u,k+3u});
        }
        return mesh;
    }
};
}} // namespace reverie::sitka
#endif
