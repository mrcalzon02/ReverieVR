#ifndef REVERIE_CLASSIC_CHESS_WOOD_ATLAS_H
#define REVERIE_CLASSIC_CHESS_WOOD_ATLAS_H
// Native Chess board diffuse material from two owner-licensed PSX Mega Pack
// wood textures in shared Beckenburg Drive library (not Beckenburg game code).
// Only a color-adjusted, palettized 64x32 game-resolution derivative ships;
// paid original PNGs and their 256x256 material textures stay in private Drive.
// Light: wood_6.png (source SHA256 996de4048a84404c7990e9bd8f736ccc6d890ac0cee713e0b76370284bf1102e).
// Dark:  wood_1.png (source SHA256 0c4dad77f7e83eb846c49fbc05fb7f73c562d45d3ec1678150eb1dd5f4a01eb1).
// Two 32x32 panels side by side, nibble indices refer to independent palettes.
// 1 KiB texture indices and 64-byte RGB565 palette; no runtime file IO.
#include <cstdint>
#include <vector>
namespace reverie { namespace chesswood {
constexpr int kWidth=64,kHeight=32,kTile=32,kPixelCount=kWidth*kHeight;
static constexpr uint16_t kPalette[32]={0xe5cc,0xe58b,0xe56b,0xdd6b,0xdd6b,0xdd4a,0xdd4a,0xd54a,0xd50a,0xd509,0xcd0a,0xd4e9,0xcce9,0xccc8,0xc4c9,0xbc88,0x7288,0x7267,0x6a67,0x6a67,0x6267,0x6a67,0x6267,0x6267,0x6267,0x6227,0x6227,0x6226,0x6226,0x6226,0x5a26,0x5a06};
static constexpr char kNibbleHex[]=
    "3004475599ddddbbf5661195147a8bd83003466579fdddacf657009502478cd7bb88025dbeffdb9b9212334131145dc5ca77013dceffddaca103333200024dd7"
    "ffeaa445aefffceeee711447a400238dffea8425affffddefd700346a300037defeeea899deeddcaea8aa884334cb268efeeeaaaadeeddddda8aa8a4233da337"
    "866bffe78b543843444334aa36bdb335897dffe77c632843365324aa37ada33769ddfffca9554514786256bb8bdfb6337adefffaaa642623777357adaadfa733"
    "dddb644b53325522969b989bcfbdffddfffd733c33306500979ca9aadfafffddaffea9bb9dd635ba441bd921330037a7affdaaabadd737aa320cfa0032003888"
    "ada77abbbdfb4a8735bdd53401ac85eeada86adddffa3a7625add32302ad86debbbceeaa5dffda520299db78114ac89bccddeeaa6fffda50017ada67223ad7ad"
    "9dcccca88dfdb4554448bb4444334311adddddaa7dfda3362247dd244333230248bdbd8314310001777564015662136357addd82132200026776730037510353"
    "bde747548aaa7aefbdd86962386366ddcfd727438a686aefafd75a73377337dffff5317feecaeaefea835dd5552266bffff7308ffedaeaffda833fd7653377df"
    "7bd87affea55a55ab4446b877512626e6ad88dfff866a76da4227a867510517e474aeffffc539657aaaaa866eca531757848dffffd72a7778aadaa57ed833277"
    "9636dfd634ceacb1477aca69baa866bba736dfd733deaaa2468aaa7adaaa77aa9333ab634494144766847cb66213aedba333aa7333a3024677747ad77303aefa"
    "d978401dfbb81013635aaaaa7513cefffa78300dfaa80003336aaa8a7603dfffd8b304afe8ef7786697dffeaea5589dbd9a202afd6df6677777dfffada66aadd"
    "99934eecbdffa48569bddfa35401963697732eedddffa2a37aadffa16300a733595aceffebffc4355cacfd522638b8106a4addffeaffd2266dadfd313537da00"
    "0417effeaeffca7b7baa835285514b510204dffeaeffaa8a7aaa733376603d60eeeefecb8aefc889835a5113885228cedeeffedc8aefd7aa9267602378502ade"
    "ceecff8b553aea71158c85bd6210017ddeedffac7338da7203ad77dd3100006feeeabdbd7668dc8213ea31ac50144aaaeeeadddf777afd9313ea308a60034aaa"
    "738b8bdfea711030038353478d89dfda73adadffea61002003a26326adaadffa757aa7dfdea4744139b5685abaa5bd54666aa7dffd8684222ad3576aad86ad63"
    "eaa7103b87aaaa753377543345513511da86003da6aa8a633377733336302320ea20003ddca86532634311111332157aea10005dfda77633622222000330046a"
    "eca735675bbb9bbaa4777aaa351305dadda627774adaaccaa6886aaa370303da4114748b6bd9bfb888699bdd56333398220283ac7ad9cfaaa779aadd573333aa";
static_assert(sizeof(kPalette)/sizeof(kPalette[0])==32u,"Chess wood palettes");
static_assert(sizeof(kNibbleHex)-1==kPixelCount,"Chess wood pixel count");
inline int HexDigit(char c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    return -1;
}
inline bool Decode(std::vector<uint16_t> &pixels){
    std::vector<uint16_t> result(kPixelCount);
    for(int n=0;n<kPixelCount/2;++n){
        const int hi=HexDigit(kNibbleHex[n*2]);
        const int lo=HexDigit(kNibbleHex[n*2+1]);
        if(hi<0||lo<0)return false;
        const int combined=(hi<<4)|lo;
        for(int which=0;which<2;++which){
            const int p=n*2+which;
            const int index=(p%kWidth<kTile?0:16)+
                            (which?combined&15:combined>>4);
            result[p]=kPalette[index];
        }
    }
    pixels.swap(result);
    return true;
}
}} // namespace reverie::chesswood
#endif
