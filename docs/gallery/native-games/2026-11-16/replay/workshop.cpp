#include "test_chamber_module.cpp"
#include "featured_module_test_host.h"
int main(int argc, char** argv) {
    audit::Check(argc==2,"capture directory"); audit::output=argv[1]; audit::GL gl;
    auto host=audit::Host(); const auto* game=reverie_native_module_entry_v1();
    auto* state=static_cast<ModuleState*>(game->create(&host)); audit::Check(state,"workshop boot");
    audit::Check(game->on_gl_context_created(state),"workshop real GL");
    auto in=audit::Input(); game->update(state,&in); in.secondary_down=1; game->update(state,&in);
    audit::Check(state->workshop.editing,"opened real editor"); in.secondary_down=0; game->update(state,&in);
    audit::Capture(game->render_eye,state,"test-chamber-edit");
    game->pause(state); audit::Check(game->prepare_exit(state),"workshop exit");
    game->release_gl_context(state);game->destroy(state);
}
