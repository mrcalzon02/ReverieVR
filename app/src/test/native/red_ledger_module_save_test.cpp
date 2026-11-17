#include "red_ledger_module.cpp"
#include "featured_module_test_host.h"
using Checkpoint=reverie::redledger::save::Checkpoint;
static void Tap(ModuleState* state,WorkTarget target,const char *plaque=nullptr){
    const TargetBounds* bounds=nullptr;for(const auto& b:kWorkTargets)if(b.target==target)bounds=&b;
    audit::Check(bounds!=nullptr,"target exists");
    auto input=audit::Pointer(bounds->center[0],bounds->center[1],bounds->center[2]);
    if(plaque){
        bool found=false;
        for(const auto &sign:signs::kSigns)if(sign.room==static_cast<int>(state->market.room())&&
                                                std::strcmp(sign.text,plaque)==0){
            input=audit::Pointer(sign.x,sign.y,sign.z);found=true;break;
        }
        audit::Check(found,"room has the visible target plaque");
    }
    Update(state,&input);
    if(state->hovered_target!=target)std::cerr<<"Tried "<<WorkTargetName(target)<<" but focused "<<WorkTargetName(state->hovered_target)<<"\n";
    audit::Check(state->hovered_target==target,"actual ray focuses intended target");
    input.primary_down=1u;Update(state,&input);input.primary_down=0u;Update(state,&input);
}
static void Click(ModuleState* state){auto in=audit::Input();Update(state,&in);in.primary_down=1;Update(state,&in);in.primary_down=0;Update(state,&in);}
static void Service(ModuleState* state){Tap(state,WorkTarget::CupStack);Tap(state,WorkTarget::Tap);Tap(state,WorkTarget::Patron);}
// Native module regression: a served premium drink/snack and its unpaid
// receipt persist together across Create/Destroy; collection stays exactly once.
static void PlayPremiumPendingReceipt(const ReverieNativeHostV1 &host) {
    audit::saves.clear();
    auto *state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&OnGlContextCreated(state)==1,"premium receipt: initial GLES2 module");
    const int cash=state->simulation.cash_cents(), beer=state->simulation.beer_units();
    const int drink=state->market.stock_drink(1), snack=state->market.stock_snack(0);
    audit::Check(drink>0&&snack>0,"premium receipt: real stocked inventory");
    Tap(state,WorkTarget::DrinkChoice);Tap(state,WorkTarget::SnackChoice);
    audit::Check(state->market.drink()==1&&state->market.snack()==0,
                 "premium receipt: pointer selects branded products");
    Service(state);
    const int owed=state->simulation.pending_payment_cents();
    audit::Check(owed>0&&state->simulation.cash_cents()==cash&&
                 state->market.stock_drink(1)==drink-1&&state->market.stock_snack(0)==snack-1&&
                 state->simulation.beer_units()==beer&&state->simulation.patrons_remaining()==4,
                 "premium receipt: exactly one drink and snack consumed without early payment");
    audit::Check(!IsTargetAvailable(state,WorkTarget::DoorStock)&&
                 !IsTargetAvailable(state,WorkTarget::Patron)&&
                 IsTargetAvailable(state,WorkTarget::Payment),
                 "premium receipt: no second service or stockroom while payment owed");
    const auto unpaid=audit::saves.at(kSaveSlot);
    audit::Check(PrepareExit(state)==1,"premium receipt: unpaid checkpoint durable");
    ReleaseGlContext(state);Destroy(state);

    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&OnGlContextCreated(state)==1&&!state->load_blocked&&
                 state->simulation.cash_cents()==cash&&
                 state->simulation.pending_payment_cents()==owed&&
                 state->market.stock_drink(1)==drink-1&&state->market.stock_snack(0)==snack-1&&
                 state->simulation.beer_units()==beer&&state->simulation.patrons_remaining()==4&&
                 audit::saves.at(kSaveSlot)==unpaid,
                 "premium receipt: reload retains exactly the unpaid stock and balance");
    audit::Capture(RenderEye,state,"red-ledger-premium-owed",1.2f,-.12f);
    Tap(state,WorkTarget::Payment);
    audit::Check(state->simulation.cash_cents()==cash+owed&&
                 state->simulation.pending_payment_cents()==0&&
                 state->simulation.patrons_remaining()==3,
                 "premium receipt: first collection credits one receipt and advances patron");
    const auto paid=audit::saves.at(kSaveSlot);
    const auto target=std::find_if(std::begin(kWorkTargets),std::end(kWorkTargets),
        [](const TargetBounds& b){return b.target==WorkTarget::Payment;});
    audit::Check(target!=std::end(kWorkTargets),"premium receipt: physical till target");
    auto input=audit::Pointer(target->center[0],target->center[1],target->center[2]);
    Update(state,&input);input.primary_down=1;Update(state,&input);
    input.primary_down=0;Update(state,&input);
    audit::Check(!IsTargetAvailable(state,WorkTarget::Payment)&&
                 state->simulation.cash_cents()==cash+owed&&
                 state->simulation.pending_payment_cents()==0&&audit::saves.at(kSaveSlot)==paid,
                 "premium receipt: second physical till press cannot pay again");
    Tap(state,WorkTarget::DoorStock,"STOCK");Tap(state,WorkTarget::StockNext);
    audit::Check(state->market.offer()==1&&state->market.stock_drink(1)==drink-1&&
                 state->market.stock_snack(0)==snack-1,
                 "premium receipt: stockroom displays depleted branded SKU");
    audit::Capture(RenderEye,state,"red-ledger-premium-stock",1.2f,-.12f);
    audit::Check(PrepareExit(state)==1,"premium receipt: paid stockroom save durable");
    ReleaseGlContext(state);Destroy(state);

    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&OnGlContextCreated(state)==1&&!state->load_blocked&&
                 state->market.room()==trade::Room::Stockroom&&state->market.offer()==1&&
                 state->simulation.cash_cents()==cash+owed&&state->simulation.pending_payment_cents()==0&&
                 state->simulation.patrons_remaining()==3&&
                 state->market.stock_drink(1)==drink-1&&state->market.stock_snack(0)==snack-1&&
                 state->simulation.beer_units()==beer,
                 "premium receipt: paid amount, depleted stock and room survive second recreate");
    std::printf("PASS_RED_PREMIUM_PENDING paid=%d cash=%d stock=%d snack=%d patrons=3\n",
                owed,state->simulation.cash_cents(),state->market.stock_drink(1),
                state->market.stock_snack(0));
    ReleaseGlContext(state);Destroy(state);audit::saves.clear();
}
static void PlayOpeningShift(const ReverieNativeHostV1 &host){
    auto *state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&OnGlContextCreated(state)==1,"fresh opening shift");
    const int opening=state->simulation.cash_cents();
    Tap(state,WorkTarget::DoorStock,"STOCK");
    Tap(state,WorkTarget::StockBuy);
    const int cost=opening-state->simulation.cash_cents();
    audit::Check(cost>0,"purchase wholesale stock");
    Tap(state,WorkTarget::ToBar,"TO BAR");
    int sales=0;
    for(int n=0;n<4;++n){
        if(!state->simulation.clean_cups())Tap(state,WorkTarget::WashStation);
        Service(state);sales+=state->simulation.pending_payment_cents();
        Tap(state,WorkTarget::Payment);
    }
    Tap(state,WorkTarget::DoorLiving,"ROOM");
    Tap(state,WorkTarget::Bed,"SLEEP");
    const auto &book=state->simulation.last_ledger();
    audit::Check(state->ledger_page==LedgerPage::Accounts&&state->simulation.day()==2&&book.day==1&&
                book.patrons_served==4&&book.supply_spend_cents==cost&&book.sales_cents==sales&&
                book.closing_cash_cents==opening+sales-cost-book.debt_service_cents,"reconcile day one");
    const int closing=book.closing_cash_cents;
    audit::Check(PrepareExit(state)==1,"save closed shift");ReleaseGlContext(state);Destroy(state);
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&OnGlContextCreated(state)==1&&state->market.room()==trade::Room::Living&&
                state->simulation.cash_cents()==closing,"reopen in living quarters with exact balance");
    Tap(state,WorkTarget::Ledger);
    audit::Check(state->ledger_page==LedgerPage::Accounts&&state->simulation.last_ledger().sales_cents==sales,
                "last closed accounts accessible after restart");
    audit::Capture(RenderEye,state,"red-ledger-closed-shift",1.2f,-.12f);
    Tap(state,WorkTarget::Ledger);audit::Check(state->ledger_page==LedgerPage::Factions,"read faction accounts");
    Tap(state,WorkTarget::Ledger);audit::Check(state->ledger_page==LedgerPage::District,"read district record");
    Tap(state,WorkTarget::Ledger);audit::Check(state->ledger_page==LedgerPage::Renovations,"return to renovations");
    std::printf("PASS_RED_SHIFT open=%d supplies=%d sales=%d close=%d served=4\n",opening,cost,sales,closing);
    ReleaseGlContext(state);Destroy(state);audit::saves.clear();
}
static void GroceryPage(ModuleState *state) {
    for (int i=0; i<4 && state->ledger_page!=LedgerPage::District; ++i)
        Tap(state,WorkTarget::Ledger);
    audit::Check(state->ledger_page==LedgerPage::District,"open district book");
}
static void Shift(ModuleState *state, bool pay) {
    int day=state->simulation.day();
    if (state->market.room()==trade::Room::Living) Tap(state,WorkTarget::ToBar,"TO BAR");
    if (day<=7 && pay && state->simulation.current_event()==EventType::ProtectionDemand)
        Tap(state,WorkTarget::ProtectionEnvelope);
    if (state->simulation.current_event()!=EventType::SupplyInterruption &&
        state->simulation.beer_units()<8) {
        Tap(state,WorkTarget::DoorStock,"STOCK");
        Tap(state,WorkTarget::BeerOrder);
        Tap(state,WorkTarget::ToBar,"TO BAR");
    }
    for (int i=0;i<4;++i) {
        if (!state->simulation.clean_cups()) Tap(state,WorkTarget::WashStation);
        Service(state); Tap(state,WorkTarget::Payment);
    }
    Tap(state,WorkTarget::DoorLiving,"ROOM");
    Tap(state,WorkTarget::Bed,"SLEEP");
    audit::Check(state->simulation.day()==day+1,"close a full worked shift");
}
static void PlayDistrictAccount(const ReverieNativeHostV1 &host) {
    auto *state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && OnGlContextCreated(state)==1,"fresh district campaign");
    while (state->simulation.day()<=6) Shift(state,true);
    const auto beforeSeventhDay=audit::saves;
    Shift(state,true);
    audit::Check(state->ledger_page==LedgerPage::District && state->grocery.envelopes_paid==3 &&
                 state->grocery.first_fee()==0,"Grigory honors all three first-week envelopes");
    audit::Capture(RenderEye,state,"red-ledger-grigory",1.2f,-.12f);
    ReleaseGlContext(state); Destroy(state); audit::saves=beforeSeventhDay;
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && OnGlContextCreated(state)==1 && state->simulation.day()==7,"reopen earlier week");
    Shift(state,false);
    audit::Check(state->grocery.envelopes_paid==2 && state->grocery.first_fee()==100,
                 "missed protection receipt changes first district settlement");
    while (state->simulation.day()<GroceryAccount::kOpeningDay) Shift(state,false);
    audit::Check(state->ledger_page==LedgerPage::District && state->clientele.services()==240,
                 "sixty staffed shifts lead to Mininska");
    const int faction=2;
    Tap(state,WorkTarget::ToStock,"TO STOCK");
    for (int i=0; i<trade::kOffers && state->market.offer()!=trade::kDrinks+faction; ++i)
        Tap(state,WorkTarget::StockNext);
    audit::Check(state->market.offer()==trade::kDrinks+faction,"selected Union snack crate");
    Tap(state,WorkTarget::StockBuy); Tap(state,WorkTarget::ToBar,"TO BAR");
    Tap(state,WorkTarget::DoorLiving,"ROOM"); GroceryPage(state);
    for (int i=0;i<faction;++i) Tap(state,WorkTarget::UpgradeNext);
    const int before=state->simulation.cash_cents();
    const int stock=state->market.stock_snack(faction);
    Tap(state,WorkTarget::UpgradeBuy);
    audit::Check(!state->grocery.deliveries && state->simulation.cash_cents()==before &&
                 state->market.stock_snack(faction)==stock,"unimproved bar cannot dispatch a grocery invoice");
    Tap(state,WorkTarget::Ledger); Tap(state,WorkTarget::UpgradeBuy);
    audit::Check(state->renovations.tier(property::Walls)==1,"purchase real interior improvement");
    GroceryPage(state);
    int cash=state->simulation.cash_cents(),favor=state->market.favor(faction);
    int cents=state->grocery.proceeds(faction,true);
    const auto beforeDispatch=audit::saves;
    audit::reject_writes=true;
    Tap(state,WorkTarget::UpgradeBuy);
    audit::Check(state->grocery.deliveries==1 && state->simulation.cash_cents()==cash+cents &&
                 state->market.stock_snack(faction)==stock-3 && state->market.favor(faction)==favor+1 &&
                 !IsTargetAvailable(state,WorkTarget::UpgradeBuy),"invoice pays once from physical stock");
    audit::Check(PrepareExit(state)==0 && audit::saves==beforeDispatch,"failed dispatch write retains last good books");
    audit::reject_writes=false; Pause(state); audit::Check(PrepareExit(state)==1,"retry saves invoice");
    ReleaseGlContext(state); Destroy(state);
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && OnGlContextCreated(state)==1 && state->ledger_page==LedgerPage::District &&
                 state->grocery.last_dispatch==61 && state->grocery.last_supplier==faction &&
                 state->market.stock_snack(faction)==stock-3 && state->simulation.cash_cents()==cash+cents &&
                 !IsTargetAvailable(state,WorkTarget::UpgradeBuy),"reopen with one receipt and next order dated");
    audit::Capture(RenderEye,state,"red-ledger-mininska-grocery",1.2f,-.12f);
    Shift(state,false);
    const auto &book=state->simulation.last_ledger();
    audit::Check(book.closing_cash_cents==book.opening_cash_cents+book.sales_cents-book.supply_spend_cents-
                book.protection_spend_cents-book.inspection_fines_cents-book.debt_service_cents,
                "wholesale and grocery proceeds reconcile in shift accounts");
    while (state->simulation.day()<68) Shift(state,false);
    GroceryPage(state);
    int day68Cash=state->simulation.cash_cents();
    Tap(state,WorkTarget::UpgradeBuy);
    int nextPayment=state->simulation.cash_cents()-day68Cash;
    audit::Check(state->grocery.deliveries==2 && nextPayment==cents+100 && state->grocery.next_day()==75,
                 "week two order restores full price; first deduction never recurs");
    std::printf("PASS_RED_DISTRICT day=68 full_shifts=67 services=%d week_one_paid=2/3 supplier=%d orders=2 first=%d next=%d remaining_stock=%d next_day=75\n",
                state->clientele.services(),faction,cents,nextPayment,state->market.stock_snack(faction));
    ReleaseGlContext(state); Destroy(state); audit::saves.clear();
}
static void ReadPreviousBooks(const ReverieNativeHostV1 &host) {
    // Written by Phone Test #2028's native module after 60 staffed shifts and a wall purchase.
    std::ifstream file("app/src/test/native/fixtures/red_ledger_v5_day61.bin",std::ios::binary);
    std::vector<uint8_t> old{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    audit::Check(old.size()==396,"original checked v5 day 61 checkpoint");
    audit::saves[kSaveSlot]=old;
    audit::saves[kSaveSlot].back()^=1;
    auto *state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && state->load_blocked && PrepareExit(state)==0,
                 "damaged prior envelope never imports");
    Destroy(state);
    audit::saves[kSaveSlot]=old;
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && !state->load_blocked && state->save_dirty &&
                 state->simulation.day()==61 && state->simulation.cash_cents()==23800 &&
                 state->clientele.services()==240 && state->renovations.tier(property::Walls)==1 &&
                 state->market.room()==trade::Room::Living && state->grocery.incomplete_books &&
                 state->grocery.first_fee()==0 && state->ledger_page==LedgerPage::District,
                 "old checked dates, cash, service count, building and room survive import");
    audit::reject_writes=true; Pause(state);
    audit::Check(audit::saves[kSaveSlot]==old && PrepareExit(state)==0,
                 "failed upgrade keeps the old file byte for byte");
    audit::reject_writes=false; Pause(state); Destroy(state);
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state && !state->save_dirty && state->grocery.incomplete_books &&
                 state->simulation.cash_cents()==23800,"new checked books remember historical uncertainty");
    Destroy(state); audit::saves.clear();
}
int main(int argc,char** argv){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    audit::Check(argc==2,"capture directory argument");audit::output=argv[1];audit::GL context;auto host=audit::Host();
    ReadPreviousBooks(host);
    PlayPremiumPendingReceipt(host);
    PlayOpeningShift(host);
    PlayDistrictAccount(host);
    auto* state=static_cast<ModuleState*>(Create(&host));audit::Check(state&&!state->load_blocked,"fresh game");
    audit::Check(OnGlContextCreated(state)==1,"Red Ledger real GL");
    audit::Capture(RenderEye,state,"red-ledger-after-bar",1.2f,-.12f);
    const auto cup=std::find_if(std::begin(kWorkTargets),std::end(kWorkTargets),
        [](const TargetBounds& target){return target.target==WorkTarget::CupStack;});
    audit::Check(cup!=std::end(kWorkTargets),"cup stack exists");
    auto held=audit::Pointer(cup->center[0],cup->center[1],cup->center[2]);
    const int cups=state->simulation.clean_cups();
    Update(state,&held);held.primary_down=1;Update(state,&held);
    audit::Check(state->simulation.clean_cups()==cups-1,"first press takes one cup");
    Pause(state);Resume(state);Update(state,&held);
    audit::Check(state->simulation.clean_cups()==cups-1,"held Select across Pause/Resume cannot trigger a second bar action");
    held.primary_down=0;Update(state,&held);
    Tap(state,WorkTarget::CupStack);
    audit::Check(state->simulation.clean_cups()==cups,"fresh press can return the held cup");
    const int cash=state->simulation.cash_cents();Service(state);
    const int payment=state->simulation.pending_payment_cents();const auto prior=audit::saves;
    audit::reject_writes=true;Tap(state,WorkTarget::Payment);
    audit::Check(state->simulation.cash_cents()==cash+payment&&state->save_failed&&state->save_dirty,"failed payment is explicit and retained");
    audit::Check(audit::saves==prior&&PrepareExit(state)==0,"failed save cannot destroy live instance or overwrite prior bytes");
    audit::Capture(RenderEye,state,"red-ledger-after-save-warning",1.2f,-.12f);
    const auto attempts=audit::writes;audit::reject_writes=false;
    auto input=audit::Input();for(int i=0;i<40;++i)Update(state,&input);
    audit::Check(!state->save_dirty&&!state->save_failed&&audit::writes==attempts+1,"one bounded recovery retry commits current state");
    audit::Check(PrepareExit(state)==1,"clean ordinary exit permitted");
    ReleaseGlContext(state);Destroy(state);state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state&&!state->load_blocked&&state->simulation.cash_cents()==cash+payment
        &&state->simulation.pending_payment_cents()==0,"recreate preserves recovered collection exactly once");
    std::printf("PASS_RED_IDLE_RECOVERY cash=%d pending=%d bytes=%zu\n",state->simulation.cash_cents(),state->simulation.pending_payment_cents(),audit::saves[kSaveSlot].size());
    Service(state);audit::reject_writes=true;Tap(state,WorkTarget::Payment);Pause(state);
    audit::Check(state->save_dirty&&PrepareExit(state)==0,"pause failure retains live changes");
    audit::reject_writes=false;Pause(state);audit::Check(!state->save_dirty&&PrepareExit(state)==1,"Pause retries recovered storage");
    const int latestCash=state->simulation.cash_cents();const auto valid=audit::saves;
    Destroy(state);
    // Corrupt the former unchecked base currency, not only metadata.
    audit::saves[kSaveSlot][Checkpoint::kHeaderSize+12]^=1u;const auto damaged=audit::saves;
    const auto beforeReadFailureWrites=audit::writes;
    state=static_cast<ModuleState*>(Create(&host));audit::Check(state&&state->load_blocked,"corrupt economic checkpoint blocked");
    audit::Check(OnGlContextCreated(state)==1,"recovery screen GL");
    Click(state);for(int i=0;i<40;++i)Update(state,&input);Pause(state);
    audit::Check(PrepareExit(state)==0&&audit::saves==damaged&&audit::writes==beforeReadFailureWrites,"corrupt save never silently overwritten");
    audit::Capture(RenderEye,state,"red-ledger-save-recovery",1.2f,-.12f);
    audit::saves=valid;Click(state);
    audit::Check(!state->load_blocked&&state->simulation.cash_cents()==latestCash,"actual Retry reloads externally recovered original");
    const auto frame=audit::Capture(RenderEye,state,"red-ledger-recovered",1.2f,-.12f);
    ReleaseGlContext(state);context.Recreate();audit::Check(OnGlContextCreated(state)==1,"context recreation");
    audit::Check(frame==audit::Capture(RenderEye,state,"red-ledger-recovered-context",1.2f,-.12f),"both eye frames exact after GL recreation");
    ReleaseGlContext(state);Destroy(state);
    audit::read_error_slots.insert(kSaveSlot);state=static_cast<ModuleState*>(Create(&host));
    audit::Check(state->load_blocked&&PrepareExit(state)==0,"I/O read fault protected");
    audit::read_error_slots.clear();Resume(state);
    audit::Check(!state->load_blocked&&state->simulation.cash_cents()==latestCash,"Resume retries failed read without default overwrite");Destroy(state);
    const auto& checked=valid.at(kSaveSlot);
    audit::saves[kSaveSlot]=std::vector<uint8_t>(checked.begin()+Checkpoint::kHeaderSize,
        checked.begin()+Checkpoint::kHeaderSize+Checkpoint::kPreviousPayloadSize);
    state=static_cast<ModuleState*>(Create(&host));
    audit::Check(!state->load_blocked&&state->save_dirty&&state->simulation.cash_cents()==latestCash,"supported old composite imported");
    Pause(state);audit::Check(audit::saves[kSaveSlot].size()==Checkpoint::kBytes&&!state->save_dirty,"legacy upgrade committed only after validation");
    Service(state);audit::ambiguous_writes=true;const int pending=state->simulation.pending_payment_cents();
    const int beforeCash=state->simulation.cash_cents();Tap(state,WorkTarget::Payment);
    const auto unacknowledged=audit::saves;
    audit::Check(state->simulation.cash_cents()==beforeCash+pending&&state->save_dirty&&PrepareExit(state)==0,
        "visible but unacknowledged write retains current live state and exit guard");
    audit::ambiguous_writes=false;Pause(state);
    audit::Check(!state->save_dirty&&audit::saves==unacknowledged&&state->simulation.pending_payment_cents()==0,
        "durability retry writes identical economic state without duplicate payment");Destroy(state);
    auto unavailable=host;unavailable.read_save=nullptr;
    state=static_cast<ModuleState*>(Create(&unavailable));
    audit::Check(state==nullptr,"missing save service cannot create unprotected progress");Destroy(state);
    std::puts("PASS actual Red Ledger payment failure/retry, Pause, prepare_exit, corruption/read protection, legacy import and real stereo recovery");
}
