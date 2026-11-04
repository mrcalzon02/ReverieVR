#ifndef REVERIE_OMEGA_FRONT_PORTRAIT_CATALOG_H
#define REVERIE_OMEGA_FRONT_PORTRAIT_CATALOG_H
// Menu-only portrait catalog; no new battle factions or save schema.
namespace reverie { namespace omega_art {
struct PortraitEntry {const char *role; const char *path;};
constexpr int kPortraitFactionCount=5,kPortraitsPerFaction=20;
static const char *const kPortraitFactionIds[5]={"F06","F07","F08","F09","F10"};
static const char *const kPortraitFactionNames[5]={"THE LOST NINTH","ASHEN CROWN","COPPER STEPPE CONFEDERATION","BRASSLINE REPUBLIC","THE FARWAKE"};
static const PortraitEntry kPortraits[5][20]={
{ // F06
{"legionary_shield","native/omega-front/ui/icons/f06/f06_01_legionary_shield.png"},
{"shield_wall_legionaries","native/omega-front/ui/icons/f06/f06_02_shield_wall_legionaries.png"},
{"velite_javelin","native/omega-front/ui/icons/f06/f06_03_velite_javelin.png"},
{"light_skirmisher","native/omega-front/ui/icons/f06/f06_04_light_skirmisher.png"},
{"slinger","native/omega-front/ui/icons/f06/f06_05_slinger.png"},
{"auxiliary_archer","native/omega-front/ui/icons/f06/f06_06_auxiliary_archer.png"},
{"optio_officer","native/omega-front/ui/icons/f06/f06_07_optio_officer.png"},
{"centurion","native/omega-front/ui/icons/f06/f06_08_centurion.png"},
{"aquilifer_eagle_standard","native/omega-front/ui/icons/f06/f06_09_aquilifer_eagle_standard.png"},
{"signifer_legion_standard","native/omega-front/ui/icons/f06/f06_10_signifer_legion_standard.png"},
{"cavalry_scout","native/omega-front/ui/icons/f06/f06_11_cavalry_scout.png"},
{"mounted_lancer","native/omega-front/ui/icons/f06/f06_12_mounted_lancer.png"},
{"scorpio_bolt_engine","native/omega-front/ui/icons/f06/f06_13_scorpio_bolt_engine.png"},
{"siege_engine_crew","native/omega-front/ui/icons/f06/f06_14_siege_engine_crew.png"},
{"castrum_engineer","native/omega-front/ui/icons/f06/f06_15_castrum_engineer.png"},
{"road_pioneer","native/omega-front/ui/icons/f06/f06_16_road_pioneer.png"},
{"barricade_bearer","native/omega-front/ui/icons/f06/f06_17_barricade_bearer.png"},
{"field_medic","native/omega-front/ui/icons/f06/f06_18_field_medic.png"},
{"logistics_pack_train","native/omega-front/ui/icons/f06/f06_19_logistics_pack_train.png"},
{"legion_commander","native/omega-front/ui/icons/f06/f06_20_legion_commander.png"}
},
{ // F07
{"levy_spearman","native/omega-front/ui/icons/f07/f07_01_levy_spearman.png"},
{"hearth_knight","native/omega-front/ui/icons/f07/f07_02_hearth_knight.png"},
{"banner_marshal","native/omega-front/ui/icons/f07/f07_03_banner_marshal.png"},
{"trebuchet_cart","native/omega-front/ui/icons/f07/f07_04_trebuchet_cart.png"},
{"wall_mason","native/omega-front/ui/icons/f07/f07_05_wall_mason.png"},
{"bow_levy","native/omega-front/ui/icons/f07/f07_06_bow_levy.png"},
{"crossbow_retainer","native/omega-front/ui/icons/f07/f07_07_crossbow_retainer.png"},
{"halberd_guard","native/omega-front/ui/icons/f07/f07_08_halberd_guard.png"},
{"hearth_swordsman","native/omega-front/ui/icons/f07/f07_09_hearth_swordsman.png"},
{"shield_sergeant","native/omega-front/ui/icons/f07/f07_10_shield_sergeant.png"},
{"mounted_scout","native/omega-front/ui/icons/f07/f07_11_mounted_scout.png"},
{"mounted_knight","native/omega-front/ui/icons/f07/f07_12_mounted_knight.png"},
{"crowned_warlord","native/omega-front/ui/icons/f07/f07_13_crowned_warlord.png"},
{"court_castellan","native/omega-front/ui/icons/f07/f07_14_court_castellan.png"},
{"court_chaplain","native/omega-front/ui/icons/f07/f07_15_court_chaplain.png"},
{"castle_observer","native/omega-front/ui/icons/f07/f07_16_castle_observer.png"},
{"fortification_planner","native/omega-front/ui/icons/f07/f07_17_fortification_planner.png"},
{"stone_hauling_crew","native/omega-front/ui/icons/f07/f07_18_stone_hauling_crew.png"},
{"ox_supply_wagon","native/omega-front/ui/icons/f07/f07_19_ox_supply_wagon.png"},
{"fortress_gatehouse","native/omega-front/ui/icons/f07/f07_20_fortress_gatehouse.png"}
},
{ // F08
{"steppe_runner","native/omega-front/ui/icons/f08/f08_01_steppe_runner.png"},
{"chariot_harrier","native/omega-front/ui/icons/f08/f08_02_chariot_harrier.png"},
{"hearth_guard","native/omega-front/ui/icons/f08/f08_03_hearth_guard.png"},
{"sling_skirmisher","native/omega-front/ui/icons/f08/f08_04_sling_skirmisher.png"},
{"herd_warden","native/omega-front/ui/icons/f08/f08_05_herd_warden.png"},
{"bronze_shield_spearmen","native/omega-front/ui/icons/f08/f08_06_bronze_shield_spearmen.png"},
{"mounted_bow_scout","native/omega-front/ui/icons/f08/f08_07_mounted_bow_scout.png"},
{"copper_axe_warrior","native/omega-front/ui/icons/f08/f08_08_copper_axe_warrior.png"},
{"roundshield_veteran","native/omega-front/ui/icons/f08/f08_09_roundshield_veteran.png"},
{"clan_standard","native/omega-front/ui/icons/f08/f08_10_clan_standard.png"},
{"council_chieftain","native/omega-front/ui/icons/f08/f08_11_council_chieftain.png"},
{"riding_scout","native/omega-front/ui/icons/f08/f08_12_riding_scout.png"},
{"chariot_archer","native/omega-front/ui/icons/f08/f08_13_chariot_archer.png"},
{"caravan_supply_wagon","native/omega-front/ui/icons/f08/f08_14_caravan_supply_wagon.png"},
{"herd_train","native/omega-front/ui/icons/f08/f08_15_herd_train.png"},
{"ritual_elder","native/omega-front/ui/icons/f08/f08_16_ritual_elder.png"},
{"horse_handler","native/omega-front/ui/icons/f08/f08_17_horse_handler.png"},
{"water_finder","native/omega-front/ui/icons/f08/f08_18_water_finder.png"},
{"pasture_camp","native/omega-front/ui/icons/f08/f08_19_pasture_camp.png"},
{"war_chariot_guard","native/omega-front/ui/icons/f08/f08_20_war_chariot_guard.png"}
},
{ // F09
{"trench_riflemen","native/omega-front/ui/icons/f09/f09_01_trench_riflemen.png"},
{"barrage_crew","native/omega-front/ui/icons/f09/f09_02_barrage_crew.png"},
{"armored_railcar","native/omega-front/ui/icons/f09/f09_03_armored_railcar.png"},
{"sapper_platoon","native/omega-front/ui/icons/f09/f09_04_sapper_platoon.png"},
{"gas_mask_corps","native/omega-front/ui/icons/f09/f09_05_gas_mask_corps.png"},
{"line_rifleman","native/omega-front/ui/icons/f09/f09_06_line_rifleman.png"},
{"machine_gun_section","native/omega-front/ui/icons/f09/f09_07_machine_gun_section.png"},
{"trench_mortar","native/omega-front/ui/icons/f09/f09_08_trench_mortar.png"},
{"signal_officer","native/omega-front/ui/icons/f09/f09_09_signal_officer.png"},
{"rail_guard","native/omega-front/ui/icons/f09/f09_10_rail_guard.png"},
{"rail_engineer","native/omega-front/ui/icons/f09/f09_11_rail_engineer.png"},
{"assault_pioneer","native/omega-front/ui/icons/f09/f09_12_assault_pioneer.png"},
{"railway_gun","native/omega-front/ui/icons/f09/f09_13_railway_gun.png"},
{"supply_locomotive","native/omega-front/ui/icons/f09/f09_14_supply_locomotive.png"},
{"field_hospital","native/omega-front/ui/icons/f09/f09_15_field_hospital.png"},
{"trench_blockhouse","native/omega-front/ui/icons/f09/f09_16_trench_blockhouse.png"},
{"rail_junction","native/omega-front/ui/icons/f09/f09_17_rail_junction.png"},
{"coal_foundry","native/omega-front/ui/icons/f09/f09_18_coal_foundry.png"},
{"command_carriage","native/omega-front/ui/icons/f09/f09_19_command_carriage.png"},
{"munition_wagon","native/omega-front/ui/icons/f09/f09_20_munition_wagon.png"}
},
{ // F10
{"ghostline_sharpshooter","native/omega-front/ui/icons/f10/f10_01_ghostline_sharpshooter.png"},
{"mote_drone","native/omega-front/ui/icons/f10/f10_02_mote_drone.png"},
{"vector_rifle_team","native/omega-front/ui/icons/f10/f10_03_vector_rifle_team.png"},
{"quiet_carrier","native/omega-front/ui/icons/f10/f10_04_quiet_carrier.png"},
{"archive_medic","native/omega-front/ui/icons/f10/f10_05_archive_medic.png"},
{"ghostline_observer","native/omega-front/ui/icons/f10/f10_06_ghostline_observer.png"},
{"nanoweave_shield","native/omega-front/ui/icons/f10/f10_07_nanoweave_shield.png"},
{"silence_infiltrator","native/omega-front/ui/icons/f10/f10_08_silence_infiltrator.png"},
{"adaptive_lancer","native/omega-front/ui/icons/f10/f10_09_adaptive_lancer.png"},
{"signal_cartographer","native/omega-front/ui/icons/f10/f10_10_signal_cartographer.png"},
{"prism_sentry","native/omega-front/ui/icons/f10/f10_11_prism_sentry.png"},
{"field_calibrator","native/omega-front/ui/icons/f10/f10_12_field_calibrator.png"},
{"power_cell_custodian","native/omega-front/ui/icons/f10/f10_13_power_cell_custodian.png"},
{"nanorepair_unit","native/omega-front/ui/icons/f10/f10_14_nanorepair_unit.png"},
{"whisper_relay","native/omega-front/ui/icons/f10/f10_15_whisper_relay.png"},
{"extraction_pilot","native/omega-front/ui/icons/f10/f10_16_extraction_pilot.png"},
{"phase_waypoint","native/omega-front/ui/icons/f10/f10_17_phase_waypoint.png"},
{"power_cell_cache","native/omega-front/ui/icons/f10/f10_18_power_cell_cache.png"},
{"refuge_barrier","native/omega-front/ui/icons/f10/f10_19_refuge_barrier.png"},
{"farwake_command","native/omega-front/ui/icons/f10/f10_20_farwake_command.png"}
}
};
inline const PortraitEntry *PortraitAt(int f,int slot){
    return f>=0 && f<kPortraitFactionCount &&
           slot>=0 && slot<kPortraitsPerFaction ? &kPortraits[f][slot]:nullptr;
}
}} // namespace reverie::omega_art
#endif
