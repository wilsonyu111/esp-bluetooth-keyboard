#include "helldiver.h"
#include "messageQueue.h"
#include <HijelHID_BLEKeyboard.h>
#include <string.h>

static const uint8_t DIR_U = 0;
static const uint8_t DIR_D = 1;
static const uint8_t DIR_L = 2;
static const uint8_t DIR_R = 3;
static const uint8_t HELLDIVER_DIRS_MAX = 16;

struct HelldiverStratagem {
    const char* name;
    const char* category;
    const char* icon;
    uint8_t count;
    uint8_t dirs[HELLDIVER_DIRS_MAX];
};

static const HelldiverStratagem kStratagems[] = {
    {"Orbital Precision Strike", "Offensive", "orbital_precision_icon.png", 3, {DIR_R, DIR_R, DIR_U}},
    {"Orbital Gatling Barrage", "Offensive", "orbital_gatling_icon.png", 5, {DIR_R, DIR_D, DIR_L, DIR_U, DIR_U}},
    {"Orbital Airburst Strike", "Offensive", "orbital_airburst_icon.png", 3, {DIR_R, DIR_R, DIR_R}},
    {"Orbital Napalm Barrage", "Offensive", "orbital_napalm_icon.png", 6, {DIR_R, DIR_R, DIR_D, DIR_L, DIR_R, DIR_U}},
    {"Orbital 120MM HE Barrage", "Offensive", "orbital_120mm_icon.png", 6, {DIR_R, DIR_R, DIR_D, DIR_L, DIR_R, DIR_D}},
    {"Orbital Walking Barrage", "Offensive", "orbital_walking_icon.png", 6, {DIR_R, DIR_D, DIR_R, DIR_D, DIR_R, DIR_D}},
    {"Orbital 380MM HE Barrage", "Offensive", "orbital_380mm_icon.png", 7, {DIR_R, DIR_D, DIR_U, DIR_U, DIR_L, DIR_D, DIR_D}},
    {"Orbital Railcannon Strike", "Offensive", "orbital_railcannon_icon.png", 5, {DIR_R, DIR_U, DIR_D, DIR_D, DIR_R}},
    {"Orbital Laser", "Offensive", "orbital_laser_icon.png", 5, {DIR_R, DIR_D, DIR_U, DIR_R, DIR_D}},
    {"Orbital EMS Strike", "Offensive", "orbital_ems_icon.png", 4, {DIR_R, DIR_R, DIR_L, DIR_D}},
    {"Orbital Gas Strike", "Offensive", "orbital_gas_icon.png", 4, {DIR_R, DIR_R, DIR_D, DIR_R}},
    {"Orbital Smoke Strike", "Offensive", "orbital_smoke_icon.png", 4, {DIR_R, DIR_R, DIR_D, DIR_U}},
    {"Eagle 500kg Bomb", "Offensive", "eagle_500kg_icon.png", 5, {DIR_U, DIR_R, DIR_D, DIR_D, DIR_D}},
    {"Eagle Strafing Run", "Offensive", "eagle_strafing_icon.png", 3, {DIR_U, DIR_R, DIR_R}},
    {"Eagle 110MM Rocket Pods", "Offensive", "eagle_110mm_icon.png", 4, {DIR_U, DIR_R, DIR_U, DIR_L}},
    {"Eagle Airstrike", "Offensive", "eagle_strike_icon.png", 4, {DIR_U, DIR_R, DIR_D, DIR_R}},
    {"Eagle Cluster Bomb", "Offensive", "eagle_cluster_icon.png", 5, {DIR_U, DIR_R, DIR_D, DIR_D, DIR_R}},
    {"Eagle Napalm Airstrike", "Offensive", "eagle_napalm_icon.png", 4, {DIR_U, DIR_R, DIR_D, DIR_U}},
    {"Eagle Smoke Strike", "Offensive", "eagle_smoke_icon.png", 4, {DIR_U, DIR_R, DIR_U, DIR_D}},
    {"CQC-1 One True Flag", "Supply", "otf_icon.png", 5, {DIR_D, DIR_L, DIR_R, DIR_R, DIR_U}},
    {"MG-43 Machine Gun", "Supply", "machine_gun_icon.png", 5, {DIR_D, DIR_L, DIR_D, DIR_U, DIR_R}},
    {"M-105 Stalwart", "Supply", "stalwart_icon.png", 6, {DIR_D, DIR_L, DIR_D, DIR_U, DIR_U, DIR_L}},
    {"MG-206 Heavy Machine Gun", "Supply", "hmg_icon.png", 5, {DIR_D, DIR_L, DIR_U, DIR_D, DIR_D}},
    {"RS-422 Railgun", "Supply", "railgun_icon.png", 6, {DIR_D, DIR_R, DIR_D, DIR_U, DIR_L, DIR_R}},
    {"APW-1 Anti-Materiel Rifle", "Supply", "amr_icon.png", 5, {DIR_D, DIR_L, DIR_R, DIR_U, DIR_D}},
    {"GL-21 Grenade Launcher", "Supply", "grenade_icon.png", 5, {DIR_D, DIR_L, DIR_U, DIR_L, DIR_D}},
    {"GL-52 De-Escalator", "Supply", "deescalator_icon.png", 5, {DIR_D, DIR_R, DIR_U, DIR_L, DIR_R}},
    {"TX-41 Sterilizer", "Supply", "sterilizer_icon.png", 5, {DIR_D, DIR_L, DIR_U, DIR_D, DIR_L}},
    {"FLAM-40 Flamethrower", "Supply", "flamethrower_icon.png", 5, {DIR_D, DIR_L, DIR_U, DIR_D, DIR_U}},
    {"LAS-98 Laser Cannon", "Supply", "laser_cannon_icon.png", 5, {DIR_D, DIR_L, DIR_D, DIR_U, DIR_L}},
    {"LAS-99 Quasar Cannon", "Supply", "quasar_cannon_icon.png", 5, {DIR_D, DIR_D, DIR_U, DIR_L, DIR_R}},
    {"ARC-3 Arc Thrower", "Supply", "arc_thrower_icon.png", 6, {DIR_D, DIR_R, DIR_D, DIR_U, DIR_L, DIR_L}},
    {"MLS-4X Commando", "Supply", "commando_icon.png", 5, {DIR_D, DIR_L, DIR_U, DIR_D, DIR_R}},
    {"EAT-17 Expendable Anti-tank", "Supply", "eat_icon.png", 5, {DIR_D, DIR_D, DIR_L, DIR_U, DIR_R}},
    {"AC-8 Autocannon", "Supply", "autocannon_icon.png", 6, {DIR_D, DIR_L, DIR_D, DIR_U, DIR_U, DIR_R}},
    {"RL-77 Airburst Rocket Launcher", "Supply", "airburst_rocket_icon.png", 5, {DIR_D, DIR_U, DIR_U, DIR_L, DIR_R}},
    {"FAF-14 Spear Launcher", "Supply", "spear_icon.png", 5, {DIR_D, DIR_D, DIR_U, DIR_D, DIR_D}},
    {"StA-X3 W.A.S.P. Launcher", "Supply", "wasp_launcher_icon.png", 5, {DIR_D, DIR_D, DIR_U, DIR_D, DIR_R}},
    {"GR-8 Recoilless Rifle", "Supply", "recoilless_icon.png", 5, {DIR_D, DIR_L, DIR_R, DIR_R, DIR_L}},
    {"B-1 Supply Pack", "Supply", "supply_pack_icon.png", 6, {DIR_D, DIR_L, DIR_D, DIR_U, DIR_U, DIR_D}},
    {"B-100 Portable Hellbomb", "Supply", "portable_hellbomb.png", 5, {DIR_D, DIR_R, DIR_U, DIR_U, DIR_U}},
    {"LIFT-860 Hover Pack", "Supply", "hover_pack_icon.png", 6, {DIR_D, DIR_U, DIR_U, DIR_D, DIR_L, DIR_R}},
    {"LIFT-850 Jump Pack", "Supply", "jump_pack_icon.png", 5, {DIR_D, DIR_U, DIR_U, DIR_D, DIR_U}},
    {"SH-32 Shield Generator Pack", "Supply", "shield_pack_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_R, DIR_L, DIR_R}},
    {"SH-51 Directional Shield Backpack", "Supply", "dir_shield_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_R, DIR_U, DIR_U}},
    {"SH-20 Ballistic Shield Backpack", "Supply", "ballistic_shield_icon.png", 6, {DIR_D, DIR_L, DIR_D, DIR_D, DIR_U, DIR_L}},
    {"AX/ARC-3 \"Guard Dog\" K-9", "Supply", "guard_dog_k-9_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_U, DIR_R, DIR_L}},
    {"AX/AR-23 \"Guard Dog\"", "Supply", "guard_dog_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_U, DIR_R, DIR_D}},
    {"AX/LAS-5 \"Guard Dog\" Rover", "Supply", "guard_dog_rover_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_U, DIR_R, DIR_R}},
    {"AX/TX-13 \"Guard Dog\" Dog Breath", "Supply", "guard_dog_breath_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_U, DIR_R, DIR_U}},
    {"M-102 Fast Recon Vehicle", "Supply", "frv_icon.png", 7, {DIR_L, DIR_D, DIR_R, DIR_D, DIR_R, DIR_D, DIR_U}},
    {"EXO-49 Emancipator Exosuit", "Supply", "emancipator_icon.png", 7, {DIR_L, DIR_D, DIR_R, DIR_U, DIR_L, DIR_D, DIR_U}},
    {"EXO-45 Patriot Exosuit", "Supply", "patriot_icon.png", 7, {DIR_L, DIR_D, DIR_R, DIR_U, DIR_L, DIR_D, DIR_D}},
    {"TD-220 Bastion (Tank)", "Supply", "bastion_icon.png", 9, {DIR_L, DIR_D, DIR_R, DIR_D, DIR_L, DIR_D, DIR_U, DIR_D, DIR_U}},
    {"A/G-16 Gatling Sentry", "Defensive", "gatling_sentry_icon.png", 4, {DIR_D, DIR_U, DIR_R, DIR_L}},
    {"A/MG-43 Machine Gun Sentry", "Defensive", "mg_sentry_icon.png", 5, {DIR_D, DIR_U, DIR_R, DIR_R, DIR_U}},
    {"E/FLAM-40 Flame Sentry", "Defensive", "flame_sentry_icon.png", 6, {DIR_D, DIR_U, DIR_R, DIR_D, DIR_U, DIR_U}},
    {"A/MLS-4X Rocket Sentry", "Defensive", "rocket_sentry_icon.png", 5, {DIR_D, DIR_U, DIR_R, DIR_R, DIR_L}},
    {"A/AC-8 Autocannon Sentry", "Defensive", "ac_sentry_icon.png", 6, {DIR_D, DIR_U, DIR_R, DIR_U, DIR_L, DIR_U}},
    {"A/M-23 EMS Mortar Sentry", "Defensive", "ems_mortar_icon.png", 5, {DIR_D, DIR_U, DIR_R, DIR_D, DIR_R}},
    {"A/M-12 Mortar Sentry", "Defensive", "mortar_sentry_icon.png", 5, {DIR_D, DIR_U, DIR_R, DIR_R, DIR_D}},
    {"FX-12 Shield Generator Relay", "Defensive", "shield_relay_icon.png", 6, {DIR_D, DIR_D, DIR_L, DIR_R, DIR_L, DIR_R}},
    {"E/GL-21 Grenadier Battlement", "Defensive", "grenadier_icon.png", 5, {DIR_D, DIR_R, DIR_D, DIR_L, DIR_R}},
    {"E/AT-12 Anti-Tank Emplacement", "Defensive", "at_emplacement_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_R, DIR_R, DIR_R}},
    {"E/MG-101 HMG Emplacement", "Defensive", "hmg_empl_icon.png", 6, {DIR_D, DIR_U, DIR_L, DIR_R, DIR_R, DIR_L}},
    {"A/ARC-3 Tesla Tower", "Defensive", "tesla_tower_icon.png", 6, {DIR_D, DIR_U, DIR_R, DIR_U, DIR_L, DIR_R}},
    {"MD-17 Anti-Tank Mines", "Defensive", "anti-tank_mines_icon.png", 4, {DIR_D, DIR_L, DIR_U, DIR_U}},
    {"MD-8 Gas Mines", "Defensive", "gas_mines_icon.png", 4, {DIR_D, DIR_L, DIR_L, DIR_R}},
    {"MD-6 Anti-Personnel Minefield", "Defensive", "ap_minefield_icon.png", 4, {DIR_D, DIR_L, DIR_U, DIR_R}},
    {"MD-I4 Incendiary Mines", "Defensive", "incendiary_icon.png", 4, {DIR_D, DIR_L, DIR_L, DIR_D}},
    {"Reinforce", "Mission", "reinforce_icon.png", 5, {DIR_U, DIR_D, DIR_R, DIR_L, DIR_U}},
    {"SOS Beacon", "Mission", "sos_beacon_icon.png", 4, {DIR_U, DIR_D, DIR_R, DIR_U}},
    {"Resupply", "Mission", "resupply_icon.png", 4, {DIR_D, DIR_D, DIR_U, DIR_R}},
    {"NUX-223 Hellbomb", "Mission", "hellbomb_icon.png", 8, {DIR_D, DIR_U, DIR_L, DIR_D, DIR_U, DIR_R, DIR_D, DIR_U}},
    {"SSSD Delivery", "Mission", "sssd_delivery_icon.png", 5, {DIR_D, DIR_D, DIR_D, DIR_U, DIR_U}},
    {"Seismic Probe", "Mission", "seismic_probe_icon.png", 6, {DIR_U, DIR_U, DIR_L, DIR_R, DIR_D, DIR_D}},
    {"Upload Data", "Mission", "upload_data_icon.png", 5, {DIR_L, DIR_R, DIR_U, DIR_U, DIR_U}},
    {"Eagle Rearm", "Mission", "eagle_rearm_icon.png", 5, {DIR_U, DIR_U, DIR_L, DIR_U, DIR_R}},
    {"SEAF Artillery", "Mission", "seaf_artillery_icon.png", 4, {DIR_R, DIR_U, DIR_U, DIR_D}},
    {"Super Earth Flag", "Mission", "se_flag_icon.png", 4, {DIR_D, DIR_U, DIR_D, DIR_U}},
    {"Hive Breaker Drill", "Mission", "hive_drill_icon.png", 6, {DIR_L, DIR_U, DIR_D, DIR_R, DIR_D, DIR_D}},
};

static const uint8_t kStratagemCount = 81;


static uint16_t dirToCode(uint8_t dir) {
    switch (dir) {
        case DIR_U: return KEY_UP;
        case DIR_D: return KEY_DOWN;
        case DIR_L: return KEY_LEFT;
        case DIR_R: return KEY_RIGHT;
        default: return 0;
    }
}

static const char* dirToKeyId(uint8_t dir) {
    switch (dir) {
        case DIR_U: return "KEY_UP";
        case DIR_D: return "KEY_DOWN";
        case DIR_L: return "KEY_LEFT";
        case DIR_R: return "KEY_RIGHT";
        default: return "KEY_UP";
    }
}

static String jsonEscape(const char* text) {
    String out;
    if (text == nullptr) {
        return out;
    }
    out.reserve(strlen(text) + 8);
    for (const char* p = text; *p; ++p) {
        char c = *p;
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out += c;
        }
    }
    return out;
}

uint8_t helldiverCount() {
    return kStratagemCount;
}

String helldiverListJson() {
    String json;
    json.reserve(96 + (size_t)kStratagemCount * 180);
    json += "{\"combos\":[";
    for (uint8_t i = 0; i < kStratagemCount; ++i) {
        if (i > 0) {
            json += ',';
        }
        const HelldiverStratagem& s = kStratagems[i];
        json += "{\"index\":";
        json += i;
        json += ",\"name\":\"";
        json += jsonEscape(s.name);
        json += "\",\"category\":\"";
        json += jsonEscape(s.category);
        json += "\",\"icon\":\"/hd-icons/";
        json += jsonEscape(s.icon);
        json += "\",\"keys\":[";
        for (uint8_t d = 0; d < s.count; ++d) {
            if (d > 0) {
                json += ',';
            }
            json += "{\"id\":\"";
            json += dirToKeyId(s.dirs[d]);
            json += "\"}";
        }
        json += "]}";
    }
    json += "]}";
    return json;
}

bool helldiverFire(uint8_t index, uint16_t holdTime, uint16_t waitAfterTap) {
    if (index >= kStratagemCount) {
        return false;
    }
    const HelldiverStratagem& s = kStratagems[index];
    if (s.count == 0 || !queueHasRoom(s.count)) {
        return false;
    }
    uint16_t codes[HELLDIVER_DIRS_MAX];
    for (uint8_t i = 0; i < s.count; ++i) {
        codes[i] = dirToCode(s.dirs[i]);
        if (codes[i] == 0) {
            return false;
        }
    }
    return queuePushKeys(codes, s.count, KeyKind::Keyboard, holdTime, waitAfterTap, false);
}
