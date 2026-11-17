#include "audit_host.h"
#include "guess_the_door_module.cpp"

int main(int argc,char **argv){
    audit::Check(argc==2,"capture directory");audit::output=argv[1];audit::GL gl;
    auto host=audit::Host();
    static unsigned voice_calls=0;
    host.request_sound=[](uint32_t cue){
        if(cue==REVERIE_NATIVE_SOUND_GUESS_LEVER_VOICE)++voice_calls;
    };
    const auto open=[&](){
        auto *s=static_cast<State*>(Create(&host));
        audit::Check(s&&!Recovering(s)&&GlStart(s),"open playable maze");Resume(s);return s;
    };
    const auto close=[](State *s){
        Pause(s);audit::Check(PrepareExit(s)==1,"save maze");GlEnd(s);Destroy(s);
    };
    auto *s=open();
    auto in=audit::Input();
    const auto seed=s->game.seed;
    const auto pull=[&](bool match){
        in=audit::Input();Update(s,&in);
        const auto &room=s->game.current;
        // Choose from the displayed clue and physical door attributes.
        const bool leftMatches=reverie::doors::Value(room.doors[0],room.clueFeature)==room.clueValue;
        in.move_x=(leftMatches==match)?-1.f:1.f;
        in.primary_down=1;Update(s,&in);
        audit::Check(s->game.phase==Phase::Pulling,"lever activated");
        if(s->game.easterActive)audit::Capture(Render,s,"guess-lever-caption");
        in=audit::Input();
        for(int frame=0;frame<150&&s->game.phase!=Phase::Ready&&s->game.phase!=Phase::Victory;++frame)
            Update(s,&in);
    };
    pull(true);pull(true);pull(false);
    audit::Check(s->game.room==0&&s->game.falls==1&&s->game.seed==seed,"pit restores original maze");
    for(unsigned n=0;n<reverie::doors::kRooms;++n){
        audit::Check(s->game.room==n,"next numbered room");
        if(n==6){
            close(s);s=open();
            audit::Check(s->game.room==n&&s->game.seed==seed&&s->game.falls==1,"restore midpoint");
        }
        pull(true);
    }
    audit::Check(s->game.phase==Phase::Victory&&s->game.clearCount==12,"complete all rooms");
    close(s);s=open();audit::Check(s->game.phase==Phase::Victory,"restore victory");
    const auto eyes=audit::Capture(Render,s,"guess-the-door-victory");
    audit::Check(eyes[0]!=eyes[1],"distinct stereo views");
    in=audit::Input();Update(s,&in);in.primary_down=1;Update(s,&in);
    audit::Check(s->game.room==0&&s->game.phase==Phase::Ready&&s->game.seed!=seed,"new seed awaits first choice");
    const unsigned before=voice_calls;
    pull(true); // First door in the new seed.
    const auto &second=s->game.current;
    pull(reverie::doors::Value(second.doors[1],second.clueFeature)==second.clueValue);
    audit::Check(s->game.easterSeen && voice_calls==before+1,"right lever speaks once with caption");
    close(s);s=open();
    audit::Check(s->game.easterSeen && voice_calls==before+1,"reopening preserves voice history");
    close(s);
    std::cout<<"Guess the Door: pit, same seed replay, six-room reopen, all 12 rooms, victory reopen and fresh maze.\n";
}
