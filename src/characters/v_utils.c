#include "g_local.h"
#include "../gamemodes/ctf.h"
#include "../gamemodes/v_hw.h"
#include "class_limits.h"

qboolean IsAllowedPregameSkills();

//************************************************************************************************
//			Indexing functions
//************************************************************************************************

//Takes the class string and returns the index
int getClassNum(char *newclass) {
    if (Q_strcasecmp(newclass, "Soldier") == 0)
        return CLASS_SOLDIER;
    else if (Q_strcasecmp(newclass, "Mage") == 0)
        return CLASS_MAGE;
    else if (Q_strcasecmp(newclass, "Necromancer") == 0)
        return CLASS_NECROMANCER;
    else if (Q_strcasecmp(newclass, "Vampire") == 0)
        return CLASS_VAMPIRE;
    else if (Q_strcasecmp(newclass, "Engineer") == 0)
        return CLASS_ENGINEER;
    else if (Q_strcasecmp(newclass, "Poltergeist") == 0)
        return CLASS_POLTERGEIST;
    else if (Q_strcasecmp(newclass, "Knight") == 0)
        return CLASS_KNIGHT;
    else if (Q_strcasecmp(newclass, "Cleric") == 0)
        return CLASS_CLERIC;
    else if (Q_strcasecmp(newclass, "Shaman") == 0)
        return CLASS_SHAMAN;
    else if (Q_strcasecmp(newclass, "Alien") == 0)
        return CLASS_ALIEN;
    else if ((Q_strcasecmp(newclass, "Weapon Master") == 0) || (Q_strcasecmp(newclass, "WeaponMaster") == 0))
        return CLASS_WEAPONMASTER;
    return 0;
}

//************************************************************************************************
//			String functions
//************************************************************************************************

void padRight(char *String, int numChars) {
    //Pads a string with spaces at the end, until the length of the string is 'numChars'
    int i;
    for (i = strlen(String); i < numChars; ++i)
        String[i] = ' ';
    String[numChars] = 0;
}

//************************************************************************************************

char GetRandomChar(void) {
    switch (GetRandom(1, 3)) {
        case 1:
            return (GetRandom(48, 57));
        case 2:
            return (GetRandom(65, 90));
        case 3:
            return (GetRandom(97, 113));
    }
    return ' ';
}

//************************************************************************************************

char *GetRandomString(int len) {
    int i;
    char *s;

    s = (char *) V_Malloc(len * sizeof(char), TAG_GAME);
    for (i = 0; i < len - 1; i++) {
        s[i] = GetRandomChar();
    }
    s[i] = '\0'; // terminating char
    return s;
}

//************************************************************************************************
//	Command list (help menu)
//************************************************************************************************

void PrintCommands(edict_t *ent) {
    //Spam the user with a huge amount of text :)
    safe_cprintf(ent, PRINT_HIGH, "Vortex Command List:\n\n");
    safe_cprintf(ent, PRINT_HIGH, va("Version %s\n\n", VRX_VERSION));

    safe_cprintf(ent, PRINT_HIGH,
                 "== Character abilities ==\n\n    acid\n    airtotem [protect]\n    ammosteal\n    amnesia\n    ampdamage\n    antigrav\n    armorbomb\n    aura_holyfreeze\n    aura_salvation\n    autocannon\n    balancespirit\n    beam_off\n    beam_on\n    berserker\n    bless\n    blessedhammer\n    bombspell\n    boomerang\n    boost\n    brain\n    cacodemon\n    caltrop\n    chainlightning\n    cocoon\n    convert\n    ");
    safe_cprintf(ent, PRINT_HIGH,
                 "cripple\n    curse\n    darknesstotem\n    deathray\n    deflect\n    detector\n    detonatebody\n    detpipe\n    earthtotem\n    emp\n    explode\n    fireball\n    firetotem\n    flyer\n    forcewall\n    forcewall_off\n    frostnova\n    gasser\n    heal\n    healer\n    healray\n    hellspawn\n    holyfreeze\n    holyground\n    icebolt\n    laser\n    laser\n    laserplatform\n    lasertrap\n    lifedrain\n    lightningstorm\n    lifetap\n    magicbolt\n    magmine\n    manashield\n    medic\n    meditate\n    meteor\n    minisentry\n    mirv\n    monster\n    mutant\n    napalm\n    naturetotem\n    nosspeed\n    nova\n    obstacle\n    overload\n    parasite\n    plasmabolt\n    proxy\n    purge\n    salvation\n    scanner\n    sentry\n    shieldoff\n    shieldon\n    spell_bomb\n    spell_boost\n    spell_corpse\n    spell_stealammo\n    spike\n    spikegrenade\n    spiker\n    spore\n    sprintoff\n    sprinton\n    sspeed\n    supplystation\n    tank\n    teleport_fwd\n    totem [remove]\n    unholyground\n    watertotem\n    weaken\n    wormhole\n    yang\n    yin\n");
    safe_cprintf(ent, PRINT_HIGH,
                 "\n\n== General commands ==\n\n    abilityindex\n    ally\n    allyinfo\n    combat\n    flashlight\n    gravjump \n    lasersight\n    masterpw\n    mute\n    owner\n    rune\n    speech\n    team\n    togglesecondary\n    trade\n    transfercredits\n    update\n    vrxhelp\n    vrxid\n    vrxmenu\n    whois\n    yell\n");

    if (ent->myskills.administrator)
        safe_cprintf(ent, PRINT_HIGH,
                     "\n\n== Admin commands ==\n\n    addnode\n    admincmd\n    bbox\n    checkclientsettings\n    computenode\n    deleteallnode\n    deletenode\n    loadnode\n    lockon_crosshair\n    lockon_off\n    lockon_on\n    navipos\n    savenode\n    showgrid\n    spawnpos\n    teleport_rnd\n    writepos\n");
}

//************************************************************************************************
//	Armoury Strings (stuff you can buy at the armory)
//************************************************************************************************

char *GetArmoryItemString(int purchase_number) {
    switch (purchase_number) {
        case 1:
            return "Shotgun";
        case 2:
            return "Super Shotgun";
        case 3:
            return "Machinegun";
        case 4:
            return "Chaingun";
        case 5:
            return "Grenade Launcher";
        case 6:
            return "Rocket Launcher";
        case 7:
            return "Hyperblaster";
        case 8:
            return "Railgun";
        case 9:
            return "BFG10K";
        case 10:
            return "20mm Cannon";
        case 11:
            return "Bullets";
        case 12:
            return "Shells";
        case 13:
            return "Cells";
        case 14:
            return "Grenades";
        case 15:
            return "Rockets";
        case 16:
            return "Slugs";
        case 17:
            return "T-Balls";
        case 18:
            return "Health";
        case 19:
            return "Power Cubes";
        case 20:
            return "Body Armor";
        case 21:
            return "Health Potions";
        case 22:
            return "Holy Water";
        case 23:
            return "Anti-grav Boots";
        case 24:
            return "Fire Resistant Clothing";
        case 25:
            return "Auto-Tball";
        case 26:
            return "Ability Rune";
        case 27:
            return "Weapon Rune";
        case 28:
            return "Combo Rune";
        case 29:
            return "Reset Abilities/Weapons";
#ifndef REMOVE_RESPAWNS
            case 30:
                return "Respawns";
#endif
        default:
            return "<BAD ITEM NUMBER>";
    }
}

//************************************************************************************************
//	Weapon Strings (mods and weapon names)
//************************************************************************************************

char *GetShortWeaponString(int weapon_number) {
    //Returns a shorter form of a weapon, for the rune menu
    switch (weapon_number) {
        case WEAPON_BLASTER:
            return "Blaster";
        case WEAPON_SHOTGUN:
            return "SG";
        case WEAPON_SUPERSHOTGUN:
            return "SSG";
        case WEAPON_MACHINEGUN:
            return "MG";
        case WEAPON_CHAINGUN:
            return "CG";
        case WEAPON_GRENADELAUNCHER:
            return "GL";
        case WEAPON_ROCKETLAUNCHER:
            return "RL";
        case WEAPON_HYPERBLASTER:
            return "HB";
        case WEAPON_RAILGUN:
            return "RG";
        case WEAPON_BFG10K:
            return "BFG10K";
        case WEAPON_SWORD:
            return "Sword";
        case WEAPON_20MM:
            return "20mm";
        case WEAPON_HANDGRENADE:
            return "HG";
        default:
            return "<BAD WEAPON NUMBER>";
    }
}

//************************************************************************************************

char *GetWeaponString(int weapon_number) {
    switch (weapon_number) {
        case WEAPON_BLASTER:
            return "Blaster";
        case WEAPON_SHOTGUN:
            return "Shotgun";
        case WEAPON_SUPERSHOTGUN:
            return "Super Shotgun";
        case WEAPON_MACHINEGUN:
            return "Machinegun";
        case WEAPON_CHAINGUN:
            return "Chaingun";
        case WEAPON_GRENADELAUNCHER:
            return "Grenade Launcher";
        case WEAPON_ROCKETLAUNCHER:
            return "Rocket Launcher";
        case WEAPON_HYPERBLASTER:
            return "Hyperblaster";
        case WEAPON_RAILGUN:
            return "Railgun";
        case WEAPON_BFG10K:
            return "BFG10K";
        case WEAPON_SWORD:
            return "Sword";
        case WEAPON_20MM:
            return "20mm Cannon";
        case WEAPON_HANDGRENADE:
            return "Hand Grenade";
        default:
            return "<BAD WEAPON NUMBER>";
    }
}

//************************************************************************************************

char *GetModString(int weapon_number, int mod_number) {
    switch (mod_number) {
        case 0:
            return "Damage";
        case 1:
            switch (weapon_number) {
                case WEAPON_BLASTER:
                    return "Stun";
                case WEAPON_SHOTGUN:
                    return "Strike";
                case WEAPON_SUPERSHOTGUN:
                    return "Range";
                case WEAPON_MACHINEGUN:
                    return "Pierce";
                case WEAPON_CHAINGUN:
                    return "Spin";
                case WEAPON_GRENADELAUNCHER:
                    return "Radius";
                case WEAPON_ROCKETLAUNCHER:
                    return "Radius";
                case WEAPON_HYPERBLASTER:
                    return "Stun";
                case WEAPON_RAILGUN:
                    return "Pierce";
                case WEAPON_BFG10K:
                    return "Duration";
                case WEAPON_SWORD:
                    return "Forging";
                case WEAPON_20MM:
                    return "Range";
                case WEAPON_HANDGRENADE:
                    return "Range";
                default:
                    return "<BAD WEAPON NUMBER>";
            }
        case 2:
            switch (weapon_number) {
                case WEAPON_BLASTER:
                    return "Speed";
                case WEAPON_SHOTGUN:
                    return "Pellets";
                case WEAPON_SUPERSHOTGUN:
                    return "Pellets";
                case WEAPON_MACHINEGUN:
                    return "Tracers";
                case WEAPON_CHAINGUN:
                    return "Tracers";
                case WEAPON_GRENADELAUNCHER:
                    return "Range";
                case WEAPON_ROCKETLAUNCHER:
                    return "Speed";
                case WEAPON_HYPERBLASTER:
                    return "Speed";
                case WEAPON_RAILGUN:
                    return "Burn";
                case WEAPON_BFG10K:
                    return "Speed";
                case WEAPON_SWORD:
                    return "Length";
                case WEAPON_20MM:
                    return "Recoil";
                case WEAPON_HANDGRENADE:
                    return "Radius";
                default:
                    return "<BAD WEAPON NUMBER>";
            }
        case 3:
            switch (weapon_number) {
                case WEAPON_BLASTER:
                    return "Trails";
                case WEAPON_SHOTGUN:
                    return "Spread";
                case WEAPON_SUPERSHOTGUN:
                    return "Spread";
                case WEAPON_MACHINEGUN:
                    return "Spread";
                case WEAPON_CHAINGUN:
                    return "Spread";
                case WEAPON_GRENADELAUNCHER:
                    return "Trails";
                case WEAPON_ROCKETLAUNCHER:
                    return "Trails";
                case WEAPON_HYPERBLASTER:
                    return "Light";
                case WEAPON_RAILGUN:
                    return "Trails";
                case WEAPON_BFG10K:
                    return "Slide";
                case WEAPON_SWORD:
                    return "Burn";
                case WEAPON_20MM:
                    return "Caliber";
                case WEAPON_HANDGRENADE:
                    return "Trails";
                default:
                    return "<BAD WEAPON NUMBER>";
            }
        case 4:
            return "Noise/Flash";
        default:
            return "<BAD WEAPON MOD NUMBER>";
    }
}

//************************************************************************************************
//	Class Strings
//************************************************************************************************

char *classnames[] = {
    "Soldier",
    "Poltergeist",
    "Vampire",
    "Mage",
    "Engineer",
    "Knight",
    "Cleric",
    "Necromancer",
    "Shaman",
    "Alien",
    "Weapon Master"
};

char *vrx_get_class_string(int class_num) {
    if (class_num > CLASS_NULL && class_num < CLASS_MAX)
        return classnames[class_num - 1];

    return "Unknown";
}

//************************************************************************************************
//	"Talent Name" Strings
//************************************************************************************************

char *GetTalentString(int talent_ID) {
    switch (talent_ID) {
        //Soldier Talents
        case TALENT_IMP_STRENGTH:
            return "Imp. Strength";
        case TALENT_IMP_RESIST:
            return "Imp. Resist";
        case TALENT_BLOOD_OF_ARES:
            return "Blood of Ares";
        case TALENT_BASIC_HA:
            return "Improved H/A";
        case TALENT_BOMBARDIER:
            return "Bombardier";
        case TALENT_MONSTER_MASTERY:
            return "Mastery";
        //Poltergeist Talents
        case TALENT_MORPHING:
            return "Morphing";
        case TALENT_MORE_AMMO:
            return "More Ammo";
        case TALENT_SUPERIORITY:
            return "Superiority";
        case TALENT_RETALIATION:
            return "Retaliation";
        case TALENT_PACK_ANIMAL:
            return "Pack Animal";
        //Vampire Talents
        case TALENT_IMP_CLOAK:
            return "Imp. Cloak";
        case TALENT_ARMOR_VAMP:
            return "Armor Vampire";
        case TALENT_SECOND_CHANCE:
            return "2nd Chance";
        case TALENT_IMP_MINDABSORB:
            return "Mind Control";
        case TALENT_CANNIBALISM:
            return "Cannibalism";
        case TALENT_FATAL_WOUND:
            return "Fatal Wound";
        //Mage Talents
        case TALENT_ICE_BOLT:
            return "Ice Bolt";
        case TALENT_MEDITATION:
            return "Mana Charge";
        case TALENT_FROST_NOVA:
            return "Frost Nova";
        case TALENT_IMP_MAGICBOLT:
            return "Imp. Magicbolt";
        case TALENT_MANASHIELD:
            return "Mana Shield";
        case TALENT_OVERLOAD:
            return "Overload";
        //Engineer Talents
        //case TALENT_DEFENSIVE_CRATE:     return "Defensive Crate";
        case TALENT_LASER_PLATFORM:
            return "Laser Platform";
        case TALENT_ALARM:
            return "Laser Trap";
        case TALENT_ARMOR_REG:
            return "NanoSuit";
        case TALENT_RAPID_ASSEMBLY:
            return "Rapid Assembly";
        case TALENT_PRECISION_TUNING:
            return "Precision Tune";
        case TALENT_STORAGE_UPGRADE:
            return "Storage Upgrade";
        //Knight Talents
        case TALENT_REPEL:
            return "Repel";
        case TALENT_MAG_BOOTS:
            return "Mag Boots";
        case TALENT_LEAP_ATTACK:
            return "Leap Attack";
        case TALENT_MOBILITY:
            return "Mobility";
        case TALENT_DURABILITY:
            return "Durability";
        //Cleric Talents
        case TALENT_BALANCESPIRIT:
            return "Balance Spirit";
        case TALENT_HOLY_GROUND:
            return "Holy Ground";
        case TALENT_UNHOLY_GROUND:
            return "Unholy Ground";
        case TALENT_PURGE:
            return "Purge";
        case TALENT_BOOMERANG:
            return "Boomerang";
        //Weapon Master Talents
        case TALENT_BASIC_AMMO_REGEN:
            return "Ammo Regen";
        case TALENT_COMBAT_EXP:
            return "Combat Exp.";
        case TALENT_TACTICS:
            return "Tactics";
        case TALENT_SIDEARMS:
            return "Sidearms";
        //Necromancer Talents
        case TALENT_EVIL_CURSE:
            return "Evil Curse";
        case TALENT_HELLSPAWN_MASTERY:
            return "Hellspawn Mastery";
        case TALENT_CORPULENCE:
            return "Corpulence";
        case TALENT_OBLATION:
            return "Oblation";
        case TALENT_DIM_VISION:
            return "Dim Vision";
        case TALENT_BLACK_DEATH:
            return "Black Death";
        //Shaman Talents
        case TALENT_ICE:
            return "Ice";
        case TALENT_WIND:
            return "Wind";
        case TALENT_STONE:
            return "Stone";
        case TALENT_SHADOW:
            return "Shadow";
        case TALENT_PEACE:
            return "Peace";
        case TALENT_VOLCANIC:
            return "Volcanic";
        case TALENT_TOTEM:
            return "Totemic Focus";
        //Alien Talents
        case TALENT_PHANTOM_OBSTACLE:
            return "Hidden Obstacle";
        case TALENT_SUPER_HEALER:
            return "Super Healer";
        case TALENT_LIFE_REG:
            return "Heart";
        case TALENT_PHANTOM_COCOON:
            return "Phantom Cocoon";
        case TALENT_SWARMING:
            return "Swarming";
        case TALENT_EXPLODING_BODIES:
            return "Exploding Body";
        //case TALENT_FASTMOVE:			return "Run!";
        // Kamikaze talents
        case TALENT_MARTYR:
            return "Martyr";
        case TALENT_INSTANTPROXYS:
            return "Instant Proxys";
        case TALENT_MAGMINESELF:
            return "Magmine Self";
        case TALENT_BLAST_RESIST:
            return "Blast Resist";
        default:
            return va("talent ID = %d", talent_ID);
    }
}

//************************************************************************************************
//	Ability Strings
//************************************************************************************************

char *GetAbilityString(int ability_number) {
    switch (ability_number) {
        case VITALITY:
            return "Vitality";
        case REGENERATION:
            return "Regen";
        case RESISTANCE:
            return "Resist";
        case STRENGTH:
            return "Strength";
        case HASTE:
            return "Haste";
        case VAMPIRE:
            return "Vampire";
        case JETPACK:
            return "Jetpack";
        case CLOAK:
            return "Cloak";
        case WEAPON_KNOCK:
            return "Knock Weapon";
        case ARMOR_UPGRADE:
            return "Armor Upgrade";
        case BLINKSTRIKE:
            return "Blink Strike";
        case ID:
            return "ID";
        case MAX_AMMO:
            return "Max Ammo";
        case GRAPPLE_HOOK:
            return "Grapple Hook";
        case SUPPLY_STATION:
            return "Supply Station";
        case CREATE_QUAD:
            return "Auto-Quad";
        case CREATE_INVIN:
            return "Auto-Invin";
        case POWER_SHIELD:
            return "Power Shield";
        case CORPSE_EXPLODE:
            return "Detonate Body";
        case GHOST:
            return "Ghost";
        case SALVATION:
            return "Salvation Aura";
        case FORCE_WALL:
            return "Force Wall";
        case AMMO_REGEN:
            return "Ammo Regen";
        case POWER_REGEN:
            return "PC Regen";
        case BUILD_LASER:
            return "Lasers";
        case HA_PICKUP:
            return "H/A Pickup";
        case BUILD_SENTRY:
            return "Sentry Guns";
        case BOOST_SPELL:
            return "Boost Spell";
        case BLOOD_SUCKER:
            return "Parasite";
        case PROXY:
            return "Proxy Grenade";
        case MONSTER_SUMMON:
            return "Monster Summon";
        case SUPER_SPEED:
            return "Super Speed";
        case ARMOR_REGEN:
            return "Armor Regen";
        case BOMB_SPELL:
            return "Bomb Spell";
        case LIGHTNING:
            return "Chain Lightning";
        case ANTIGRAV:
            return "Antigrav";
        case HOLY_FREEZE:
            return "HolyFreeze Aura";
        case WORLD_RESIST:
            return "World Resist";
        case BULLET_RESIST:
            return "Bullet Resist";
        case SHELL_RESIST:
            return "Shell Resist";
        case ENERGY_RESIST:
            return "Energy Resist";
        case PIERCING_RESIST:
            return "Piercing Resist";
        case SPLASH_RESIST:
            return "Splash Resist";
        case CACODEMON:
            return "Cacodemon";
        case PLAGUE:
            return "Plague";
        case FLESH_EATER:
            return "Corpse Eater";
        case HELLSPAWN:
            return "Hell Spawn";
        case BRAIN:
            return "Brain";
        case BEAM:
            return "Beam";
        case MAGMINE:
            return "Mag Mine";
        case CRIPPLE:
            return "Cripple";
        case MAGICBOLT:
            return "Magic Bolt";
        case TELEPORT:
            return "Teleport";
        case NOVA:
            return "Nova";
        case EXPLODING_ARMOR:
            return "Exploding Armor";
        case MIND_ABSORB:
            return "Mind Absorb";
        case LIFE_DRAIN:
            return "Life Drain";
        case AMP_DAMAGE:
            return "Amp Damage";
        case CURSE:
            return "Curse";
        case WEAKEN:
            return "Weaken";
        case AMNESIA:
            return "Amnesia";
        case BLESS:
            return "Bless";
        case HEALING:
            return "Healing";
        case AMMO_UPGRADE:
            return "Ammo Efficiency";
        case YIN:
            return "Yin Spirit";
        case YANG:
            return "Yang Spirit";
        case FLYER:
            return "Flyer";
        case SPIKE:
            return "Spike";
        case MUTANT:
            return "Mutant";
        case MORPH_MASTERY:
            return "Morph Mastery";
        case NAPALM:
            return "Napalm";
        case TANK:
            return "Tank";
        case MEDIC:
            return "Medic";
        case BERSERK:
            return "Berserker";
        case METEOR:
            return "Meteor";
        case AUTOCANNON:
            return "Auto Cannon";
        case HAMMER:
            return "Blessed Hammer";
        case BLACKHOLE:
            return "Wormhole";
        case FIRE_TOTEM:
            return "Fire Totem";
        case WATER_TOTEM:
            return "Water Totem";
        case AIR_TOTEM:
            return "Air Totem";
        case EARTH_TOTEM:
            return "Earth Totem";
        case DARK_TOTEM:
            return "Darkness Totem";
        case NATURE_TOTEM:
            return "Nature Totem";
        case FURY:
            return "Fury";
        case TOTEM_MASTERY:
            return "Totem Mastery";
        case SHIELD:
            return "Shield";
        case CALTROPS:
            return "Caltrops";
        case BOX:
            return "Defensive boxes";
        case SPIKE_GRENADE:
            return "Spike Grenade";
        case DETECTOR:
            return "Detector";
        case CONVERSION:
            return "Conversion";
        case DEFLECT:
            return "Deflect";
        case SCANNER:
            return "Scanner";
        case EMP:
            return "EMP";
        case DOUBLE_JUMP:
            return "Double Jump";
        case LIFE_TAP:
            return "Life Tap";
        case FIREBALL:
            return "Fireball";
        case PLASMA_BOLT:
            return "Plasma Bolt";
        case LIGHTNING_STORM:
            return "Lightning Storm";
        case MIRV:
            return "Mirv Grenade";
        case SPIKER:
            return "Spiker";
        case OBSTACLE:
            return "Obstacle";
        case HEALER:
            return "Healer";
        case GASSER:
            return "Gasser";
        case SPORE:
            return "Spore";
        case ACID:
            return "Acid";
        case COCOON:
            return "Cocoon";
        case SELFDESTRUCT:
            return "Self Destruct";
        case FLASH:
            return "Flash";
        case DECOY:
            return "Decoy";
        case EXPLODING_BARREL:
            //      xxxxxxxxxxxxxx (14 chars)
            return "Barrel Bombs";
        default:
            return "<ERROR: NO NAME>";
    }
}

//************************************************************************************************
//	Rune Strings
//************************************************************************************************

typedef const char *class_rune_string_t[7];
class_rune_string_t soldier_rune_val = {
    "Newbie's",
    "Greenhorn's",
    "Sergent's",
    "Soldier's",
    "Warrior's",
    "Veteran's",
    "Master's"
};

class_rune_string_t mage_rune_val = {
    "Apprentice's",
    "Illusionist's",
    "Sage's",
    "Mage's",
    "Wizard's",
    "Sorcerer's",
    "Archimage's",
};

class_rune_string_t necromancer_rune_val = {
    "Exorcist's",
    "Theurgist's",
    "Shaman's",
    "Necromancer's",
    "Warlock's",
    "Demi-Lich's",
    "Lich's"
};

class_rune_string_t engineer_rune_val = {
    "Student's",
    "Assistant's",
    "Technician's",
    "Mechanic's",
    "Scientist's",
    "Physicist's",
    "Engineer's"
};

class_rune_string_t shaman_rune_val = {
    "Dabbler's",
    "Divinist's",
    "Mystic's",
    "Animist's",
    "Gnostic's",
    "Spiritualist's",
    "Spirits'"
};

class_rune_string_t vampire_rune_val = {
    "Ghoul's",
    "Geist's",
    "Wraith's",
    "Vampire's",
    "Revenant's",
    "Nosferatu's",
    "Dracula's"
};

class_rune_string_t alien_rune_val = {
    "Egg's",
    "Hatchling's",
    "Worm's",
    "Pupa's",
    "Insect's",
    "Guard's",
    "Royal's"
};

class_rune_string_t poltergeist_rune_val = {
    "Spook's",
    "Spirit's",
    "Phantom's",
    "Poltergeist's",
    "Apparition's",
    "Ghost's",
    "Monster's"
};

class_rune_string_t knight_rune_val = {
    "Commoner's",
    "Squire's",
    "Guard's",
    "Knight's",
    "Baron's",
    "Lord's",
    "King's"
};

class_rune_string_t cleric_rune_val = {
    "Follower's",
    "Acolyte's",
    "Preacher's",
    "Cleric's",
    "Pastor's",
    "Bishop's",
    "Pope's"
};

class_rune_string_t weaponmaster_rune_val = {
    "Amateur's",
    "Neophyte's",
    "Novice's",
    "Weapon Master's",
    "Guru's",
    "Expert's",
    "Elite's"
};

const char *GetRuneValString(item_t *rune) {
    int level = rune->itemLevel;

    switch (rune->itemtype) {
        case ITEM_WEAPON:
        case ITEM_COMBO:
        case ITEM_ABILITY: {
            switch (level / 2) {
                case 0:
                    return "Worthless";
                case 1:
                    return "Cracked";
                case 2:
                    return "Chipped";
                case 3:
                    return "Flawed";
                case 4:
                    return "Normal";
                case 5:
                    return "Good";
                case 6:
                    return "Great";
                case 7:
                    return "Exceptional";
                case 8:
                    return "Flawless";
                case 9:
                    return "Perfect";
                case 10:
                    return "Godly";
                default:
                    return "Cheater's";
            }
        }
        case ITEM_CLASSRUNE: {
            int idx = min(level / 2, 6);
            switch (rune->classNum) {
                case CLASS_SOLDIER:
                    return soldier_rune_val[idx];
                case CLASS_POLTERGEIST:
                    return poltergeist_rune_val[idx];
                case CLASS_VAMPIRE:
                    return vampire_rune_val[idx];
                case CLASS_MAGE:
                    return mage_rune_val[idx];
                case CLASS_ENGINEER:
                    return engineer_rune_val[idx];
                case CLASS_KNIGHT:
                    return knight_rune_val[idx];
                case CLASS_CLERIC:
                    return cleric_rune_val[idx];
                case CLASS_NECROMANCER:
                    return necromancer_rune_val[idx];
                case CLASS_SHAMAN:
                    return shaman_rune_val[idx];
                case CLASS_ALIEN:
                    return alien_rune_val[idx];
                case CLASS_WEAPONMASTER:
                    return weaponmaster_rune_val[idx];
                default:
                    return "<Unknown Class>";
            }
            break;
        default:
            return "Strange";
        }
    }
}

//************************************************************************************************

int CountRuneMods(item_t *rune) {
    int i;
    int count = 0;
    for (i = 0; i < MAX_VRXITEMMODS; ++i)
        if (rune->modifiers[i].type != TYPE_NONE && rune->modifiers[i].value > 0)
            ++count;
    return count;
}

//************************************************************************************************
//			Physical functions
//************************************************************************************************

float V_EntDistance(edict_t *ent1, edict_t *ent2)
//Gets the absolute vector distance between 2 entities
{
    vec3_t dist;

    dist[0] = ent1->s.origin[0] - ent2->s.origin[0];
    dist[1] = ent1->s.origin[1] - ent2->s.origin[1];
    dist[2] = ent1->s.origin[2] - ent2->s.origin[2];

    return sqrt((dist[0] * dist[0]) + (dist[1] * dist[1]) + (dist[2] * dist[2]));
}

//************************************************************************************************

void V_ResetAbilityDelays(edict_t *ent)
//Resets all of the selected ent's ability delays
{
    int i;

    //Reset ability cooldowns, even though almost none of them are used.
    for (i = 0; i < MAX_ABILITIES; ++i)
        ent->myskills.abilities[i].delay = 0.0f;

    //Reset talent cooldowns as well.
    for (i = 0; i < ent->myskills.talents.count; ++i)
        ent->myskills.talents.talent[i].delay = 0.0f;
}

//************************************************************************************************

qboolean V_CanUseAbilities(edict_t *ent, int ability_index, int ability_cost, qboolean print_msg) {
    if (!ent->client)
        return false;

    if (!G_EntIsAlive(ent))
        return false;

    if (ent->myskills.abilities[ability_index].general_skill == 2 &&
        pregame_time->value > level.time) // mobility in pregame
        return true;

    if (ent->myskills.abilities[ability_index].disable)
        return false;

    //4.2 can't use abilities in wormhole/noclip
    if (ent->flags & FL_WORMHOLE)
        return false;

    if (ent->manacharging)
        return false;

    // poltergeist cannot use abilities in human form
    if (vrx_is_morphing_polt(ent) && !ent->mtype) {
        // allow them to morph
        if (!PM_PlayerHasMonster(ent) &&
            (ability_index != CACODEMON) &&
            (ability_index != MUTANT) &&
            (ability_index != BRAIN) &&
            (ability_index != FLYER) &&
            (ability_index != MEDIC) &&
            (ability_index != BLOOD_SUCKER) &&
            (ability_index != TANK) &&
            (ability_index != BERSERK)) {
            if (print_msg)
                safe_cprintf(ent, PRINT_HIGH, "You can't use abilities in human form!\n");
            return false;
        }
    }

    if (ent->myskills.abilities[ability_index].current_level < 1) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You have to upgrade to use this ability!\n");
        return false;
    }

    // enforce special rules on flag carrier in CTF mode
    if (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent)) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "Flag carrier cannot use abilities.\n");
        return false;
    }

    if (que_typeexists(ent->curses, CURSE_FROZEN))
        return false;

    if (level.time < pregame_time->value && !trading->value) {
        if (!IsAllowedPregameSkills()) // allow use of abilities in pvm modes.
        {
            if (print_msg)
                safe_cprintf(ent, PRINT_HIGH, "You can't use abilities during pre-game.\n");
            return false;
        }
    }

    // can't use abilities immediately after a re-spawn
    if (ent->client->respawn_time > level.time) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You can't use abilities for another %2.1f seconds.\n",
                         ent->client->respawn_time - level.time);
        return false;
    }

    if (ent->client->ability_delay > level.time) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You can't use abilities for another %2.1f seconds.\n",
                         ent->client->ability_delay - level.time);
        return false;
    }

    if (ent->myskills.abilities[ability_index].delay > level.time) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You can't use this ability for another %2.1f seconds.\n",
                         ent->myskills.abilities[ability_index].delay - level.time);
        return false;
    }

    // can't use abilities if you don't have enough power cubes
    if (ability_cost && (ent->client->pers.inventory[power_cube_index] < ability_cost)) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You need more %d power cubes to use this ability.\n",
                         ability_cost - ent->client->pers.inventory[power_cube_index]);
        return false;
    }

    // players cursed by amnesia can't use abilities
    if (que_findtype(ent->curses, NULL, AMNESIA) != NULL) {
        if (print_msg)
            safe_cprintf(ent, PRINT_HIGH, "You have been cursed with amnesia and can't use any abilities!!\n");
        return false;
    }
    return true;
}

//************************************************************************************************

//Gives the player packs of ammo
qboolean V_GiveAmmoClip(edict_t *ent, float qty, int ammotype) {
    int amount;
    int *max;
    int *current;

    switch (ammotype) {
        case AMMO_SHELLS:
            amount = SHELLS_PICKUP;
            current = &ent->client->pers.inventory[shell_index];
            max = &ent->client->pers.max_shells;
            break;
        case AMMO_BULLETS:
            amount = BULLETS_PICKUP;
            current = &ent->client->pers.inventory[bullet_index];
            max = &ent->client->pers.max_bullets;
            break;
        case AMMO_GRENADES:
            amount = GRENADES_PICKUP;
            current = &ent->client->pers.inventory[grenade_index];
            max = &ent->client->pers.max_grenades;
            break;
        case AMMO_ROCKETS:
            amount = ROCKETS_PICKUP;
            current = &ent->client->pers.inventory[rocket_index];
            max = &ent->client->pers.max_rockets;
            break;
        case AMMO_SLUGS:
            amount = SLUGS_PICKUP;
            current = &ent->client->pers.inventory[slug_index];
            max = &ent->client->pers.max_slugs;
            break;
        case AMMO_CELLS:
            amount = CELLS_PICKUP;
            current = &ent->client->pers.inventory[cell_index];
            max = &ent->client->pers.max_cells;
            break;
        default:
            return false;
    }

    //don't pick up ammo if you are full already
    //if (*current >= *max) return false;
    if (*current >= *max && qty > 0.0) return false;

    //Multiply by the item quantity
    amount *= qty;

    //Multiply by the ammo efficiency skill
    //if (!ent->myskills.abilities[AMMO_UPGRADE].disable)
    //	amount *= AMMO_UP_BASE + (AMMO_UP_MULT * ent->myskills.abilities[AMMO_UPGRADE].current_level);

    //Talent: Improved HA Pickup
    if (vrx_get_talent_slot(ent, TALENT_BASIC_HA) != -1)
        amount *= 1.0 + (0.2 * vrx_get_talent_level(ent, TALENT_BASIC_HA));

    //Give them the ammo
    *current += amount;
    if (*current > *max)
        *current = *max;
    if (*current < 0)
        *current = 0;

    return true;
}

//************************************************************************************************

//Returns an ammo type based on the player's respawn weapon.
int V_GetRespawnAmmoType(edict_t *ent) {
    switch (ent->myskills.respawn_weapon) {
        case 2: //sg
        case 3: //ssg
        case 12: //20mm
            return AMMO_SHELLS;
        case 4: //mg
        case 5: //cg
            return AMMO_BULLETS;
        case 6: //gl
        case 11: //hg
            return AMMO_GRENADES;
        case 7: //rl
            return AMMO_ROCKETS;
        case 9: //rg
            return AMMO_SLUGS;
        case 8: //hb
        case 10: //bfg
            return AMMO_CELLS;
        default: //blaster/sword
            return 0; //nothing
    }
}

//************************************************************************************************

int GetClientNumber(edict_t *ent)
//Returns zero if a client was not found
{
    edict_t *temp;
    int i;

    for (i = 1; i <= game.maxclients; i++) {
        temp = &g_edicts[i];

        //Copied from id. do not crash
        if (!temp->inuse)
            continue;
        if (!temp->client)
            continue;

        //More checking
        if (strlen(temp->myskills.player_name) < 1)
            continue;

        if (Q_stricmp(ent->myskills.player_name, temp->myskills.player_name) == 0) //same name
            return i + 1;
    }
    return 0;
}

//************************************************************************************************

edict_t *V_getClientByNumber(int index)
//Gets client through an index number used in GetClientNumber()
{
    return &g_edicts[index];
}

//************************************************************************************************
//************************************************************************************************
//			New File I/O functions
//************************************************************************************************
//************************************************************************************************

char ReadChar(FILE *fptr) {
    char Value;
    fread(&Value, sizeof(char), 1, fptr);
    return Value;
}

//************************************************************************************************

void WriteChar(FILE *fptr, char Value) {
    fwrite(&Value, sizeof(char), 1, fptr);
}

//************************************************************************************************

void ReadString(char *buf, FILE *fptr) {
    int Length;

    //get the string length
    Length = ReadChar(fptr);

    //If string is empty, abort
    if (Length == 0) {
        buf[0] = 0;
        //return buf;
    }

    fread(buf, Length, 1, fptr);

    //Null terminate the string just read
    buf[Length] = 0;
}

//************************************************************************************************

void WriteString(FILE *fptr, char *String) {
    int Length = strlen(String);
    WriteChar(fptr, (char) Length);
    fwrite(String, Length, 1, fptr);
}

//************************************************************************************************

int ReadInteger(FILE *fptr) {
    int Value;
    fread(&Value, sizeof(int), 1, fptr);
    return Value;
}

//************************************************************************************************

void WriteInteger(FILE *fptr, int Value) {
    fwrite(&Value, sizeof(int), 1, fptr);
}

//************************************************************************************************

long ReadLong(FILE *fptr) {
    long Value;
    fread(&Value, sizeof(long), 1, fptr);
    return Value;
}

//************************************************************************************************

void WriteLong(FILE *fptr, long Value) {
    fwrite(&Value, sizeof(long), 1, fptr);
}

//************************************************************************************************

char *V_FormatFileName(char *name) {
    char filename[64];
    char buffer[64];
    int i, j = 0;

    //This bit of code formats invalid filename chars, like the '?' and
    //reformats them into a saveable format.
    Q_strncpy(buffer, name, sizeof(buffer) - 1);

    for (i = 0; i < strlen(buffer); ++i) {
        char tmp;
        switch (buffer[i]) {
            case '\\':
                tmp = '1';
                break;
            case '/':
                tmp = '2';
                break;
            case '?':
                tmp = '3';
                break;
            case '|':
                tmp = '4';
                break;
            case '"':
                tmp = '5';
                break;
            case ':':
                tmp = '6';
                break;
            case '*':
                tmp = '7';
                break;
            case '<':
                tmp = '8';
                break;
            case '>':
                tmp = '9';
                break;
            case '_':
                tmp = '_';
                break;
            default:
                tmp = '0';
                break;
        }

        if (tmp != '0') {
            filename[j++] = '_';
            filename[j++] = tmp;
        } else {
            filename[j++] = buffer[i];
        }
    }

    //make sure string is null-terminated
    filename[j] = 0;

    return va("%s", filename);
}

//************************************************************************************************
//************************************************************************************************
//			New File I/O functions (for parsing text files)
//************************************************************************************************
//************************************************************************************************

int V_tFileCountLines(FILE *fptr, long size) {
    int count = 0;
    char temp;
    int i = 0;

    do {
        temp = getc(fptr);
        if (temp == '\n')
            count++;
    } while (++i < size);

    rewind(fptr);
    return count - 1; //Last line is always empty
}

//************************************************************************************************

void V_tFileGotoLine(FILE *fptr, int linenumber, long size) {
    int count = 0;
    char temp;
    int i = 0;

    do {
        temp = fgetc(fptr);
        if (temp == '\n')
            count++;
        if (count == linenumber)
            return;
    } while (++i < size);
    return;
}

//************************************************************************************************


#define CHANGECLASS_MSG_CHANGE    1
#define CHANGECLASS_MSG_RESET    2
#define CHANGECLASS_MSG_ASCEND    3

void vrx_change_class(char *playername, int newclass, int msgtype) {
    int i;
    edict_t *player;

    if (!newclass)
        return;

    for (i = 1; i <= maxclients->value; i++) {
        player = &g_edicts[i];
        if (!player->inuse)
            continue;
        if (Q_strcasecmp(playername, player->myskills.player_name) != 0)
            continue;
        // Archon respawn only sword
        if (newclass == CLASS_KNIGHT)
            player->myskills.respawn_weapon = 1; //sword only

        //Reset player's skills and change their class
        memset(player->myskills.abilities, 0, sizeof(upgrade_t) * MAX_ABILITIES);
        memset(player->myskills.weapons, 0, sizeof(weapon_t) * MAX_WEAPONS);
        vrx_clear_talents(player);
        player->myskills.class_num = newclass;

        //Initialize their upgrades
        vrx_assign_abilities(player);
        vrx_reset_weapon_maximums(player);
        vrx_set_talents(player);
        vrx_prestige_reapply_all(player);

        //Re-apply equipment
        vrx_runes_unapply(player);
        for (i = 0; i < 3; ++i)
            vrx_runes_apply(player, &player->myskills.items[i]);

        if (msgtype == CHANGECLASS_MSG_CHANGE) {
            //Notify everyone
            safe_cprintf(NULL, PRINT_HIGH, "%s's class was changed to %s (%d).\n", playername,
                         vrx_get_class_string(newclass), newclass);
            vrx_write_to_logfile(player, va("Class was changed to %s (%d).\n", gi.argv(3), newclass));
            gi.dprintf("%s's class was changed to %s (%d).\n", playername, gi.argv(3), newclass);
        } else if (msgtype == CHANGECLASS_MSG_RESET) {
            safe_cprintf(player, PRINT_HIGH, "Your ability and weapon upgrades have been reset!\n");
            vrx_write_to_logfile(player, "Character data was reset.\n");
            gi.dprintf("%s's character data was reset.\n", playername);
        } else if (msgtype == CHANGECLASS_MSG_ASCEND) {
            char* msg = HiPrint(va("*** %s has ascended to level %d! ***", playername, player->myskills.prestige.total));
            safe_bprintf(PRINT_HIGH, "%s\n", msg);
            vrx_write_to_logfile(player, va("Ascended to level %d.\n", player->myskills.prestige.total));
            gi.dprintf("%s has ascended to level %d!\n", playername, player->myskills.prestige.total);
            safe_centerprintf(player, "You have ascended to level %d.", player->myskills.prestige.total);
            gi.sound(player, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NORM, 0);
            V_Free(msg);
        }

        player->myskills.level = 0;
        player->myskills.next_level = vrx_get_points_tnl(0);

        // reset experience to zero but only if they are ascending
        if (msgtype == CHANGECLASS_MSG_ASCEND) {
            player->myskills.experience = 0;
            for (int j = 1; j <= start_level->value; ++j) {
                player->myskills.experience += vrx_get_points_tnl(j - 1);
            }
        }

        player->myskills.speciality_points = player->myskills.prestige.abilityPoints;
        player->myskills.weapon_points = player->myskills.prestige.weaponPoints * 4;
        vrx_check_for_levelup(player, false);

        //Save the player.
        vrx_commit_character(player, false);
        return;
    }
    gi.dprintf("Can't find player: %s\n", playername);
}

char *V_TruncateString(char *string, int newStringLength) {
    static char buf[512];

    if (!newStringLength || (newStringLength > 512))
        return string;

    // it's already short enough
    if (strlen(string) <= newStringLength)
        return string;

    strncpy(buf, string, newStringLength);
    buf[newStringLength - 1] = '\0';

    return &buf[0];
}

void V_RegenAbilityAmmo(edict_t *ent, int ability_index, int regen_frames, int regen_delay) {
    int ammo;
    int max = ent->myskills.abilities[ability_index].max_ammo;
    int *current = &ent->myskills.abilities[ability_index].ammo;
    int *delay = &ent->myskills.abilities[ability_index].ammo_regenframe;

    if (*current > max)
        return;

    // don't regenerate ammo if player is firing
    if (ent->client->buttons & BUTTON_ATTACK)
        return;

    if (regen_delay > 0) {
        if (level.framenum < *delay)
            return;

        *delay = level.framenum + regen_delay;
    } else
        regen_delay = 1;

    ammo = floattoint((float) max / ((float) regen_frames / regen_delay));

    //gi.dprintf("ammo=%d, max=%d, frames=%d, delay=%d\n", ammo, max, regen_frames, regen_delay);

    if (ammo < 1)
        ammo = 1;

    *current += ammo;
    if (*current > max)
        *current = max;
}

void V_ModifyMorphedHealth(edict_t *ent, int type, qboolean morph) {
    float mult;

    switch (type) {
        case P_TANK: // multi-role fighter, slow moving with large hitbox
            mult = 2 + 0.15 * ent->myskills.abilities[TANK].current_level;
            if (mult > 3.5)
                mult = 3.5;
            break;
        case MORPH_BERSERK: // melee fighter with power screen
            mult = 1 + 0.1 * ent->myskills.abilities[BERSERK].current_level;
            if (mult > 2)
                mult = 2;
            break;
        case MORPH_MUTANT: // melee fighter with large hitbox
            mult = 1 + 0.15 * ent->myskills.abilities[MUTANT].current_level;
            if (mult > 2.5)
                mult = 2.5;
            break;
        case MORPH_CACODEMON: // slow moving flyer with large hitbox
            mult = 1 + 0.05 * ent->myskills.abilities[CACODEMON].current_level;
            if (mult > 1.5)
                mult = 1.5;
            break;
        case MORPH_BRAIN: // melee fighter with power screen
            mult = 1 + 0.05 * ent->myskills.abilities[BRAIN].current_level;
            if (mult > 1.5)
                mult = 1.5;
            break;
        case M_MYPARASITE: // melee fighter with vamp up to 2x health
            mult = 1 + 0.05 * ent->myskills.abilities[BLOOD_SUCKER].current_level;
            if (mult > 1.5)
                mult = 1.5;
            break;
        // nothing to modify
        default:
            return;
    }

    // apply multiplier
    if (morph) {
        //gi.dprintf("mult = %.2f, before = %d/%d, ", mult, ent->health, ent->max_health);
        ent->health = floattoint(ent->health * mult);
        ent->max_health = floattoint(ent->max_health * mult);
        //gi.dprintf("after = %d/%d\n", ent->health, ent->max_health);
    }
    // remove multiplier
    else {
        ent->health = floattoint(ent->health / mult);
        ent->max_health = MAX_HEALTH(ent);
    }
}

// attempts to push (move) away from nearby wall(s) by dist
void V_PushBackWalls(edict_t *self, float dist) {
    vec3_t start, end, v = {0, 0, 0};
    trace_t tr;
    qboolean move = false;

    VectorCopy(self->s.origin, start);

    // gather plane normal vector(s)
    for (int i = 0; i <= 3; i++) {
        VectorCopy(start, end);

        // trace in a circle around us to find nearby walls
        switch (i) {
            case 0:
                end[0] += dist;
                break;
            case 1:
                end[1] += dist;
                break;
            case 2:
                end[0] -= dist;
                break;
            case 3:
                end[1] -= dist;
                break;
        }
        tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_SOLID);

        // did we hit something?
        if (tr.fraction < 1) {
            move = true;
            if (VectorEmpty(v)) {
                // first intersecting plane normal--this is our escape vector from one wall
                VectorCopy(tr.plane.normal, v);
            } else {
                // second intersecting plane normal--this is another wall roughly perpendicular to the first
                // now add the two plane normals together to get our final escape vector
                VectorAdd(tr.plane.normal, v, v);
                break;
            }
        }
    }
    // we are adjacent to one or more walls, so try to push/move away
    if (move) {
        // ending position
        VectorMA(start, dist, v, end);

        tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_SOLID);
        // nothing detected
        if (!(tr.contents & MASK_SOLID) && tr.fraction == 1) {
            // try to move target 'e' to new position
            VectorCopy(end, self->s.origin);
            gi.linkentity(self);
        }
    }
}

// attempts to push/knock-away nearby entities by dist
void V_PushBackEnts(edict_t *self, float dist) {
    vec3_t start, end, v;
    trace_t tr;
    edict_t *e = NULL;

    //gi.dprintf("V_PushBackEnts()\n");

    while ((e = findradius(e, self->s.origin, dist)) != NULL) {
        if (!G_EntExists(e))
            continue;
        // ignore non-moving entities (e.g. forcewall)
        if (e->movetype == MOVETYPE_NONE)
            continue;
        // ignore entities that can't take damage or are non-solid
        if (!e->takedamage || e->solid == SOLID_NOT || e->svflags & SVF_NOCLIENT)
            continue;
        // ignore frozen entities
        if (que_typeexists(e->curses, CURSE_FROZEN))
            continue;
        // ignore non-visible entities
        if (!visible(self, e))
            continue;
        // ignore self
        if (e == self)
            continue;

        //gi.dprintf("found class: %s\n", e->classname);

        // calculate vector to target 'e' at the same Z height
        VectorCopy(e->s.origin, start);
        start[2] = self->s.origin[2];
        VectorSubtract(start, self->s.origin, v);
        VectorNormalize(v);

        // ending position
        VectorMA(start, dist, v, end);

        // trace from target 'e' (start) along same vector to ending position (end), a short distance 'range' away
        tr = gi.trace(start, e->mins, e->maxs, end, e, MASK_SOLID);

        // nothing detected
        if (!(tr.contents & MASK_SOLID) && tr.fraction == 1) {
            // try to move target 'e' to new position
            VectorCopy(end, e->s.origin);
            gi.linkentity(e);
        } else {
            //gi.dprintf("collided with solid\n");
        }
    }
}

void V_RestoreMorphed(edict_t *ent, int refund) {
    //gi.dprintf("V_RestoreMorphed()\n");

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/morph.wav"), 1, ATTN_NORM, 0);

    // if (isMorphingPolt(ent)) // az- always remove summonables
    vrx_remove_player_summonables(ent);

    if (PM_PlayerHasMonster(ent)) {
        V_ModifyMorphedHealth(ent, P_TANK, false);

        if (refund > 0)
            ent->client->pers.inventory[power_cube_index] += refund;

        // player's chasing this monster should now chase the player instead
        PM_UpdateChasePlayers(ent->owner);

        // remove the tank entity
        M_Remove(ent->owner, false, false);
        PM_RestorePlayer(ent);
        return;
    }

    V_ModifyMorphedHealth(ent, ent->mtype, false);

    if (ent->mtype && (refund > 0))
        ent->client->pers.inventory[power_cube_index] += refund;

    ent->viewheight = 22;
    ent->mtype = 0;
    ent->s.modelindex = 255;
    ent->s.skinnum = ent - g_edicts - 1;

    if (!ent->client->pers.weapon)
        vrx_pick_respawn_weapon(ent);

    ShowGun(ent);

    // az: restore v_flags we've set
    ent->v_flags &= ~SFLG_NO_BOB;
    ent->client->lock_frames = 0; //4.2 reset smart-rocket lock-on counter
}

void mutant_checkattack(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

void V_Touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf) {
    // made a sound
    if (other && other->inuse)
        other->lastsound = level.framenum;

    mutant_checkattack(self, other, plane, surf);
}


char *V_GetMonsterKind(int mtype) {
    switch (mtype) {
        case M_SOLDIER:
        case M_SOLDIERLT:
        case M_SOLDIERSS:
            return "soldier";
        case M_FLIPPER:
            return "flipper";
        case M_FLYER:
            return "flyer";
        case M_INFANTRY:
            return "enforcer";
        case M_INSANE:
        case M_RETARD:
            return "lost marine";
        case M_GUNNER:
            return "gunner";
        case M_CHICK:
            return "iron praetor";
        case M_PARASITE:
            return "parasite";
        case M_FLOATER:
            return "floater";
        case M_HOVER:
            return "hover";
        case M_BERSERK:
            return "berserker";
        case M_MEDIC:
            return "medic";
        case M_MUTANT:
            return "mutant";
        case M_BRAIN:
            return "brain";
        case M_GLADIATOR:
            return "gladiator";
        case M_TANK:
        case P_TANK:
            return "tank";
        case M_SUPERTANK:
            return "supertank";
        case M_SHAMBLER:
            return "shambler";
        case M_JORG:
            return "jorg";
        case M_MAKRON:
            return "makron";
        case M_COMMANDER:
            return "commander";
        case M_BARON_FIRE:
            return "fire baron";
        case M_MINISENTRY:
        case M_BEAMSENTRY:
            return "mini-sentry";
        case M_SENTRY:
            return "sentry";
        case M_FORCEWALL:
            return "force wall";
        case M_DECOY:
            return "decoy";
        case M_SKULL:
            return "hellspawn";
        case M_YINSPIRIT:
            return "yin spirit";
        case M_YANGSPIRIT:
            return "yang spirit";
        case M_BALANCESPIRIT:
            return "balance spirit";
        case M_AUTOCANNON:
            return "autocannon";
        case M_DETECTOR:
            return "detector";
        case TOTEM_FIRE:
            return "fire totem";
        case TOTEM_WATER:
            return "water totem";
        case TOTEM_AIR:
            return "air totem";
        case TOTEM_EARTH:
            return "earth totem";
        case TOTEM_NATURE:
            return "nature totem";
        case TOTEM_DARKNESS:
            return "darkness totem";
        case M_SPIKER:
            return "spiker";
        case M_OBSTACLE:
            return "obstacle";
        case M_BOX:
            return "box";
        case M_GASSER:
            return "gasser";
        case M_SPIKEBALL:
            return "spore";
        case M_HEALER:
            return "healer";
        case M_COCOON:
            return "cocoon";
        case M_LASERPLATFORM:
            return "laser platform";
        case M_PROXY:
            return "proxy grenade";
        case M_MAGMINE:
            return "magmine";
        case M_SPIKE_GRENADE:
            return "spike grenade";
        case INVASION_PLAYERSPAWN:
        case CTF_PLAYERSPAWN:
        case TBI_PLAYERSPAWN:
            return "spawn";
        case M_SUPPLYSTATION:
            return "depot";
        case M_ALARM:
            return "laser trap";
        case HW_FLAG:
            return "halo";
        case M_BARREL:
            return "barrel";
        default:
            return "<unknown>";
    }
}

char *V_GetMonsterName(edict_t *monster) {
    static char buf[50];

    buf[0] = 0;

    if (monster->monsterinfo.bonus_flags & BF_UNIQUE_FIRE) {
        strcat(buf, "Hephaestus");
        return &buf[0];
    }

    if (monster->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING) {
        strcat(buf, "Zeus");
        return &buf[0];
    }

    if (monster->monsterinfo.bonus_flags & BF_GHOSTLY)
        strcat(buf, "ghostly ");
    else if (monster->monsterinfo.bonus_flags & BF_BERSERKER)
        strcat(buf, "berserker ");
    else if (monster->monsterinfo.bonus_flags & BF_FANATICAL)
        strcat(buf, "fanatical ");
    else if (monster->monsterinfo.bonus_flags & BF_POSESSED)
        strcat(buf, "posessed ");
    else if (monster->monsterinfo.bonus_flags & BF_STYGIAN)
        strcat(buf, "stygian ");
    else if (monster->monsterinfo.bonus_flags & BF_CHAMPION)
        strcat(buf, "champion ");

    strcat(buf, V_GetMonsterKind(monster->mtype));

    return &buf[0];
}

qboolean IsAllowedPregameSkills() {
    if (pvm->value || invasion->value)
        return true;
    return false;
}

qboolean V_IsPVP(void) {
    return !pvm->value && !ctf->value && !ffa->value && !invasion->value && !ptr->value && !domination->value &&
           !tbi->value;
}

qboolean V_HealthCache(edict_t *ent, int max_per_second, int update_frequency_svframes) {
    int heal, delta, max;

    if (ent->health_cache_nextframe > level.framenum)
        return false;

    ent->health_cache_nextframe = level.framenum + update_frequency_svframes;

    if (ent->health_cache > 0 && ent->health < ent->max_health) {
        if (update_frequency_svframes <= sv_fps->value)
            max = max_per_second / (sv_fps->value / update_frequency_svframes);
        else
            max = max_per_second;

        if (max > ent->health_cache)
            max = ent->health_cache;

        delta = ent->max_health - ent->health;

        if (delta > max)
            heal = max;
        else
            heal = delta;

        ent->health += heal;
        ent->health_cache -= heal;

        //	gi.dprintf("healed %d / %d (max %d) at %d\n", heal, ent->health_cache, max, level.framenum);

        return true;
    }

    ent->health_cache = 0;

    return false;
}

qboolean V_ArmorCache(edict_t *ent, int max_per_second, int update_frequency) {
    int heal, delta, max, max_armor;
    int *armor;

    if (ent->armor_cache_nextframe > level.framenum)
        return false;

    if (ent->client) {
        armor = &ent->client->pers.inventory[body_armor_index];
        max_armor = MAX_ARMOR(ent);
    } else {
        armor = &ent->monsterinfo.power_armor_power;
        max_armor = ent->monsterinfo.max_armor;
    }

    ent->armor_cache_nextframe = level.framenum + update_frequency;

    if (ent->armor_cache > 0 && *armor < max_armor) {
        if (update_frequency <= sv_fps->value)
            max = max_per_second / (sv_fps->value / update_frequency);
        else
            max = max_per_second;
        if (max > ent->armor_cache)
            max = ent->armor_cache;

        delta = max_armor - *armor;

        if (delta > max)
            heal = max;
        else
            heal = delta;

        *armor += heal;
        ent->armor_cache -= heal;

        return true;
    }

    ent->armor_cache = 0;

    return false;
}

void vrx_reset_player_state(edict_t *ent) {
    //gi.dprintf("vrx_reset_player_state\n");
    if (PM_PlayerHasMonster(ent) || ent->mtype) {
        // restore the player to original state
        V_RestoreMorphed(ent, 0);
    }

    // reset railgun sniper frames
    ent->client->refire_frames = 0;

    // reset h/a cache
    ent->health_cache = 0;
    ent->health_cache_nextframe = 0;
    ent->armor_cache = 0;
    ent->armor_cache_nextframe = 0;

    // reset automag
    ent->automag = 0;

    // boot them out of a wormhole, but don't forget to teleport them to a valid location!
    ent->flags &= ~(FL_WORMHOLE|FL_BLACK_DEATH);
    ent->svflags &= ~SVF_NOCLIENT;
    ent->movetype = MOVETYPE_WALK;

    // reset detected state
    ent->flags &= ~FL_DETECTED;
    ent->detected_time = 0;

    // reset cocooned state
    ent->cocoon_time = 0;
    ent->cocoon_factor = 0;
    ent->cocoon_owner = NULL;

    // disble fury
    ent->fury_time = 0;

    // disable movement abilities
    //jetpack
    ent->client->thrusting = 0;
    //grapple hook
    ent->client->hook_state = HOOK_READY;
    // super speed
    ent->superspeed = false;
    // antigrav
    ent->antigrav = false;
    // mana shield
    ent->manashield = false;
    // reset holdtime
    ent->holdtime = 0;

    // powerups
    ent->client->invincible_framenum = 0;
    ent->client->quad_framenum = 0;

    // reset Blink Strike state
    ent->client->blinkStrike_targ = NULL;
    ent->client->tele_timeout = 0;

    // reset ability delay
    ent->client->ability_delay = 0;
    V_ResetAbilityDelays(ent);

    // reset their velocity
    VectorClear(ent->velocity);

    // remove all curses except plague, which will be transferred to a body upon player respawn
    CurseRemove(ent, 0, CURSE_PLAGUE);
    // remove auras
    AuraRemove(ent, 0);

    // remove movement penalty
    ent->Slower = 0;
    ent->slowed_factor = 1.0;
    ent->slowed_time = 0;
    ent->chill_level = 0;
    ent->chill_time = 0;
    ent->chill_owner = NULL;

    // remove all summonables
    vrx_remove_player_summonables(ent);

    // if teamplay mode, set team skin
    if (ctf->value || domination->value || tbi->value) {
        char *s = Info_ValueForKey(ent->client->pers.userinfo, "skin");
        vrx_assign_character_skin(ent, s);

        // drop the flag
        dom_dropflag(ent, FindItem("Flag"));
        CTF_DropFlag(ent, FindItem("Red Flag"));
        CTF_DropFlag(ent, FindItem("Blue Flag"));
    }

    if (hw->value) {
        hw_dropflag(ent, FindItem("Halo"));
    }

    // drop all techs
    tech_dropall(ent);

    // disable scanner
    ClearScanner(ent->client);

    // stop trading
    EndTrade(ent);
}

/*
============
V_TouchSolids
A modified version of G_TouchTriggers that will call the touch function
of any valid intersecting entities
============
*/
void InfectedCorpseTouch(edict_t* self, edict_t* other);
void V_TouchSolids(edict_t *ent) {
    int i, num;
    edict_t *touch[MAX_EDICTS], *hit;

    // sanity check
    if (!ent || !ent->inuse || !ent->takedamage)
        return;
    num = gi.BoxEdicts(ent->absmin, ent->absmax, touch, MAX_EDICTS, AREA_SOLID);

    // be careful, it is possible to have an entity in this
    // list removed before we get to it (killtriggered)
    for (i = 0; i < num; i++) {
        hit = touch[i];
        //if (ent->deadflag == DEAD_DEAD && G_EntIsAlive(hit))
        //    gi.dprintf("corpse touched live entity\n");
        InfectedCorpseTouch(ent, hit);
        if (!hit->inuse || !hit->touch/* || !hit->takedamage */ || hit == ent)
            continue;
        //gi.dprintf("V_TouchSolids called by %s hit %s\n", ent->classname, hit->classname);
        hit->touch(hit, ent, NULL, NULL);
    }
}

// returns true if an entity has any offensive
// (meaning they can do damage) deployed
qboolean V_HasSummons(edict_t *ent) {
    return (ent->num_sentries || ent->num_monsters || ent->skull || ent->num_spikers
            || ent->num_autocannon || ent->num_gasser || ent->num_caltrops || ent->num_proxy
            || ent->num_obstacle || ent->num_armor || ent->num_spikeball || ent->num_lasers);
}

void EmpEffects(edict_t *ent);

void SV_AddBlend(float r, float g, float b, float a, float *v_blend);

void V_ShellNonAbilityEffects(edict_t *ent) {
    qboolean finalEffects = true;
    edict_t *cl_ent = G_GetClient(ent);

    // ********** NON-ENTITY SPECIFIC EFFECTS BELOW **********
    // drones flash briefly when selected for orders
    if ((ent->monsterinfo.selected_time > level.time) && (sf2qf(level.framenum) & 6)) {
        ent->s.effects |= EF_COLOR_SHELL;
        ent->s.renderfx |= RF_SHELL_GREEN;
        return; // stop processing effects
    }


    // spree war
    if (SPREE_WAR == true) {
        // spree dude and his summons glow white
        if (G_GetSummoner(ent) == SPREE_DUDE) {
            ent->s.effects |= EF_COLOR_SHELL;
            ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
        }
        // players and player-summoned monsters in FFA/PvP mode glow red
        else if (cl_ent) {
            ent->s.effects |= EF_COLOR_SHELL;
            ent->s.renderfx |= (RF_SHELL_RED);
        }

        return; // stop processing effects
    }

    // these effects apply to players and player-spawned monsters
    if (cl_ent) {
        // if we have an aura or we are morphed/monster, apply a shell in CTF.
        if (que_typeexists(ent->auras, 0) || ent->mtype || !ent->client || PM_PlayerHasMonster(ent)) {
            if (ctf->value || pvm->value) {
                ent->s.effects |= EF_COLOR_SHELL;
                // red team shell
                if (cl_ent->teamnum == RED_TEAM)
                    ent->s.renderfx |= RF_SHELL_RED;
                // default blue shell (blue team and PvM aura or morphed/monster)
                else {
                    if (!V_IsPVP())
                        ent->s.renderfx |= RF_SHELL_BLUE;
                }
            }
        }


        // CTF/Domination/PtR/PvM mode effects
        if (ctf->value || domination->value || ptr->value || pvm->value || hw->value || tbi->value) {
            // flag effects for clients
            if (ent->client) {
                // red flag
                if (ent->client->pers.inventory[red_flag_index]) {
                    ent->s.effects |= EF_COLOR_SHELL;
                    ent->s.effects |= EF_FLAG1;
                    ent->s.renderfx |= RF_SHELL_RED;
                }
                // blue flag
                else if (ent->client->pers.inventory[blue_flag_index]) {
                    ent->s.effects |= EF_COLOR_SHELL;
                    ent->s.effects |= EF_FLAG2;
                    ent->s.renderfx |= RF_SHELL_BLUE;
                }
                // domination
                else if (ent->client->pers.inventory[flag_index]) {
                    ent->s.effects |= EF_COLOR_SHELL;
                    // red team
                    if (ent->teamnum == RED_TEAM) {
                        ent->s.effects |= EF_FLAG1;
                        ent->s.renderfx |= RF_SHELL_RED;
                    }
                    // blue team
                    else {
                        ent->s.effects |= EF_FLAG2;
                        ent->s.renderfx |= RF_SHELL_BLUE;
                    }
                }

                if (ent->client->pers.inventory[ITEM_INDEX(FindItem("Halo"))]) {
                    // Red shell for the saint.
                    ent->s.effects |= EF_FLAG1;
                    ent->s.renderfx |= RF_SHELL_YELLOW;
                    ent->s.effects |= EF_COLOR_SHELL;
                }
            }
            /*
            if (hw->value && !ent->client->pers.inventory[ITEM_INDEX(FindItem("Halo"))])
            {
            ent->s.effects |= EF_SPHERETRANS;
            }*/
        }

        // these effects only apply to FFA/PvP modes
        if (!ptr->value && !domination->value && !pvm->value && !ctf->value) {
            // boss/miniboss
            if (ent->myskills.boss > 0 || vrx_is_newbie_basher(ent)) {
                ent->s.effects |= EF_COLOR_SHELL;
                ent->s.renderfx |= (RF_SHELL_YELLOW);
            }
            // spree effect
            else if (ent->myskills.streak >= 6) {
                ent->s.effects |= EF_COLOR_SHELL;
                ent->s.renderfx |= (RF_SHELL_GREEN);
            }
        }

        if (finalEffects)
            return; // stop processing effects
    }

    // ********** NON-CLIENT SPECIFIC EFFECTS BELOW **********

    // invasion effects (quad/invin)
    //FIXME: should this go with ability effects?
    if (ent->monsterinfo.inv_framenum > level.framenum) {
        ent->s.effects |= EF_COLOR_SHELL;
        if (sf2qf(level.framenum) & 4)
            ent->s.renderfx |= RF_SHELL_RED;
        else
            ent->s.renderfx |= RF_SHELL_BLUE;
        return; // stop processing effects
    }

    // 4.5 monster bonus flags
    if (ent->monsterinfo.bonus_flags) {
        ent->s.effects |= EF_HALF_DAMAGE;
        ent->s.renderfx |= RF_SHELL_GREEN | RF_SHELL_RED;
    }
}

void V_ShellAbilityEffects(edict_t *ent) {
    // freeze/partial freeze effects
    if (que_typeexists(ent->curses, AURA_HOLYFREEZE) || que_typeexists(ent->curses, CURSE_FROZEN) ||
        (ent->chill_time > level.time)) {
        // client-specific
        if (ent->client) {
            SV_AddBlend(0.75, 0.75, 0.75, 0.6, ent->client->ps.blend);
            ent->client->ps.rdflags |= RDF_UNDERWATER;
        }

        // off-white color shell
        ent->s.effects |= EF_HALF_DAMAGE;
    }
    // detector ability
    else if (ent->flags & FL_DETECTED && ent->detected_time > level.time) {
        ent->s.effects |= EF_COLOR_SHELL | EF_TAGTRAIL;
        ent->s.renderfx |= (RF_SHELL_YELLOW);
    }
    // an active aura makes you glow cyan
    else if (que_typeexists(ent->auras, 0)) {
        ent->s.effects |= EF_COLOR_SHELL;
        ent->s.renderfx |= (RF_SHELL_CYAN);
    }

    if (ent->client) {
        int remaining;

        //BEGIN QUAD EFFECTS
        if (ent->client->quad_framenum > level.framenum) {
            remaining = sf2qf(ent->client->quad_framenum - level.framenum);
            if (remaining > 30 || (remaining & 4))
                ent->s.effects |= EF_QUAD;
            if (remaining == 30 && (ent->svflags & SVF_MONSTER)) // beginning to fade
                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
        }

        if (ent->client->quadfire_framenum > level.framenum) {
            remaining = sf2qf(ent->client->quadfire_framenum - level.framenum);
            if (remaining > 30 || (remaining & 4))
                ent->s.effects |= EF_QUAD;
            if (remaining == 30 && (ent->svflags & SVF_MONSTER)) // beginning to fade
                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire2.wav"), 1, ATTN_NORM, 0);
        }

        if (ent->client->invincible_framenum > level.framenum && (sf2qf(level.framenum) & 8)) {
            remaining = sf2qf(ent->client->invincible_framenum - level.framenum);
            if (remaining > 30 || (remaining & 4))
                ent->s.effects |= EF_PENT;
            if (remaining == 30 && (ent->svflags & SVF_MONSTER)) // beginning to fade
                gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
        }
        //END QUAD EFFECTS
    }
}

void V_NonShellEffects(edict_t *ent) {
    int pa_type;

    // ********** NON-ENTITY SPECIFIC EFFECTS BELOW **********

    // make all entities that have effects visible to low-light/IR vision
    ent->s.renderfx |= RF_IR_VISIBLE;

    // power armor
    if (ent->powerarmor_time > level.time) {
        // get power armor type
        if (ent->client)
            pa_type = PowerArmorType(ent);
        else
            pa_type = ent->monsterinfo.power_armor_type;

        // only non-shell effects are added here, so power shield is intentionally omitted
        if (pa_type == POWER_ARMOR_SCREEN)
            ent->s.effects |= EF_POWERSCREEN;
    }

    // ********** CLIENT-SPECIFIC EFFECTS BELOW **********
    if (ent->client) {
        // shield ability effect
        if (level.time > ent->shield_activate_time) {
            if (ent->shield == 1)
                ent->s.effects |= EF_POWERSCREEN;
            // only non-shell effects are added here, so power shield effect is intentionally omitted
        }

        // ghost effect applies to all classes except Poltergeist (who gets it for free)
        else if (ent->myskills.abilities[GHOST].current_level > 0 ||
                 vrx_is_morphing_polt(ent))
            ent->s.effects |= EF_PLASMA;

        // super speed effect
        if (ent->superspeed)
            ent->s.effects |= EF_TAGTRAIL;

        // cloak transparency effect
        if ((level.time > ent->client->ability_delay) && !que_typeexists(ent->auras, 0)
            && !ent->myskills.abilities[CLOAK].disable && (ent->myskills.abilities[CLOAK].current_level > 0)) {
            if (ent->client->idle_frames >= qf2sf(5))
                ent->s.effects |= EF_SPHERETRANS;
            else
                ent->s.effects |= EF_PLASMA;
        }

        // chat protect changes view, but doesn't add effects
        //FIXME: move this?
        if (ent->flags & FL_CHATPROTECT)
            SV_AddBlend(0.5, 0, 0, 0.3, ent->client->ps.blend);

        // manashield effects = EF_HALF_DAMAGE and EF_TAGTRAIL
        if (ent->manashield)
            ent->s.effects |= 1073741824 | 536870912;

        return; // only non-client effects from here on!
    }
    // ********** NON-CLIENT SPECIFIC EFFECTS BELOW **********

    // barrel is transparent if it has an owner
    if (ent->mtype == M_BARREL && ent->owner && ent->owner->inuse) // && ent->owner->client)
        ent->s.effects |= EF_SPHERETRANS;

    // obstacle becomes transparent before it cloaks
    if (ent->mtype == M_OBSTACLE) {
        if (ent->monsterinfo.idle_frames >= ent->monsterinfo.nextattack - 10)
            ent->s.effects |= EF_SPHERETRANS;
        else if (ent->monsterinfo.idle_frames >= ent->monsterinfo.nextattack - 20)
            ent->s.effects |= EF_PLASMA;
    }

    //Talent: Phantom Cocoon
    // cocoon becomes transparent before it cloaks
    if (ent->mtype == M_COCOON && ent->monsterinfo.jumpdn != -1) {
        if (ent->monsterinfo.jumpup >= ent->monsterinfo.jumpdn - 10)
            ent->s.effects |= EF_SPHERETRANS;
        else if (ent->monsterinfo.jumpup >= ent->monsterinfo.jumpdn - 20)
            ent->s.effects |= EF_PLASMA;
    }

    // conversion sparks
    if (ent->flags & FL_CONVERTED)
        CurseEffects(ent, 10, 210);

    // EMP sparks
    if ((ent->empeffect_time > level.time) && (random() > 0.9))
        EmpEffects(ent);

    //4.5 monster bonus flags
    if (ent->monsterinfo.bonus_flags & BF_GHOSTLY)
        ent->s.effects |= EF_PLASMA;
}

void V_SetEffects(edict_t *ent) {
    int effects, r_effects;

    // clear all effects
    ent->s.effects = ent->s.renderfx = 0;

    // plague flies
    if (que_typeexists(ent->curses, CURSE_PLAGUE))
        ent->s.effects |= EF_FLIES;

    if (ent->mtype != M_MAGMINE && ent->health < 1)
        return;

    // apply non-ability shell effects
    V_ShellNonAbilityEffects(ent);

    // save current effects
    effects = ent->s.effects;
    r_effects = ent->s.renderfx;

    // apply ability shell effects
    V_ShellAbilityEffects(ent);

    // has there been a change?
    if (ent->s.renderfx != r_effects) {
        // swap back and forth between effects
        if (sf2qf(level.framenum) & 8) {
            // clear all effects
            ent->s.effects = ent->s.renderfx = 0;
            // apply ability shell effects
            V_ShellAbilityEffects(ent);
        } else {
            // apply saved non-ability shell effects
            ent->s.effects = effects;
            ent->s.renderfx = r_effects;
        }
    }

    // apply non-shell effects
    V_NonShellEffects(ent);
}

/*
void V_ClearNode (nodedata_t *node)
{
memset(node, 0, sizeof(nodedata_t));
}

void V_ClearNodes (nodedata_t **nodelist)
{
memset(nodelist, 0, sizeof(nodedata_t[MAX_NODES]));
}

qboolean node_visible (nodedata_t *start, nodedata_t *end)
{
return ((gi.inPVS(start->pos, end->pos))
&& (gi.trace(start->pos, vec3_origin, vec3_origin, end->pos, NULL, MASK_SOLID).fraction == 1.0));
}
*/

qboolean isMonster(edict_t *ent) {
    return (ent && ent->mtype && ent->mtype < 100 && ent->svflags & SVF_MONSTER);
}

qboolean vrx_is_morphing_polt(edict_t *ent) {
    return ent->myskills.class_num == CLASS_POLTERGEIST &&
           ent->myskills.abilities[MORPH_MASTERY].current_level &&
           !ent->myskills.abilities[MORPH_MASTERY].disable;
}
