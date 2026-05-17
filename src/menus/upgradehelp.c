#include "g_local.h"
#include "upgradehelp.h"

static struct upgradehelp_s help[] =
{
    //GENERAL
    {
        VITALITY, {
            "Passive ability. Increases",
            "maximum health.",
            nullptr,
        }
    },
    {
        MAX_AMMO, {
            "Passive ability. Increases",
            "maximum ammunition and power",
            "cubes.",
            nullptr,
        }
    },
    {
        POWER_REGEN, {
            "Passive ability. Increases",
            "power cube regeneration rate.",
            nullptr,
        }
    },
    {
        WORLD_RESIST, {
            "Passive ability. Reduces",
            "world damage (e.g. lava).",
            nullptr,
        }
    },
    {
        AMMO_REGEN, {
            "Passive ability. Regenerates",
            "ammunition for weapons.",
            nullptr,
        }
    },
    {
        SHELL_RESIST, {
            "Passive ability. Reduces",
            "damage from shell-based",
            "weapons.",
            nullptr,
        }
    },
    {
        BULLET_RESIST, {
            "Passive ability. Reduces",
            "damage from bullet-based",
            "weapons.",
            nullptr,
        }
    },
    {
        PIERCING_RESIST, {
            "Passive ability. Reduces",
            "damage from piercing",
            "weapons.",
            nullptr,
        }
    },
    {
        ENERGY_RESIST, {
            "Passive ability. Reduces",
            "damage from energy-based",
            "weapons.",
            nullptr,
        }
    },
    {
        SPLASH_RESIST, {
            "Passive ability. Reduces",
            "damage from explosive",
            "weapons.",
            nullptr,
        }
    },
    {
        SCANNER, {
            "Shows nearby enemies and",
            "allies on your HUD.",
            nullptr,
        },
        .command = "scanner",
    },
    {
        HA_PICKUP, {
            "Increases the amount of",
            "health and armor provided by",
            "items. Passive ability.",
            nullptr,
        }
    },
    {
        STRENGTH, {
            "Passive ability. Increases",
            "weapon damage.",
            nullptr,
        }
    },
    {
        RESISTANCE, {
            "Passive ability. Reduces",
            "damage from all sources.",
            nullptr,
        }
    },
    {
        NAPALM, {
            "Throw a grenade that",
            "continuously explodes,",
            "spawning flames and",
            "catching things on fire.",
            "Consumes power cubes.",
            nullptr,
        },
        .command = "napalm",
    },
    {
        SPIKE_GRENADE, {
            "Throw a grenade that",
            "fires deadly spikes in all",
            "directions. Consumes power",
            "cubes.",
            nullptr,
        },
        .command = "spikegrenade",
    },
    {
        EMP, {
            "Throw a grenade that, upon",
            "exploding, disables monsters",
            "and other summonables. Also",
            "detonates ammunition.",
            "Consumes power cubes.",
            nullptr,
        },
        .command = "emp",
    },
    {
        MIRV, {
            "Throw a grenade that",
            "launches additional grenades",
            "in all directions. Consumes",
            "power cubes.",
            nullptr,
        },
        .command = "mirv",
    },
    {
        EXPLODING_BARREL, {
            "Toss a barrel that explodes",
            "and throws shrapnel after",
            "being destroyed.",
            nullptr,
        },
        .command = "barrel [remove]",
    },
    {
        CREATE_INVIN, {
            "Passive ability. Provides",
            "temporary invincibility.",
            "Activates after 10 kills.",
            nullptr,
        }
    },
    {
        CREATE_QUAD, {
            "Passive ability. Provides",
            "temporary quad damage.",
            "Activates after 10 kills.",
            nullptr,
        }
    },
    {
        GRAPPLE_HOOK, {
            "Shoot a grapple hook! Allows",
            "for improved mobility and",
            "access to hard-to-reach",
            "places. Uses power cubes.",
            nullptr,
        },
        .command = "hook|unhook"
    },
    //VAMPIRE
    {
        VAMPIRE, {
            "Passive ability. Receive",
            "health from damage inflicted",
            "by weapons.",
            nullptr,
        }
    },
    {
        GHOST, {
            "Passive ability. Chance for",
            "damage taken to be reduced to",
            "zero.",
            nullptr,
        }
    },
    {
        LIFE_DRAIN, {
            "Cast a spell that steals life",
            "from nearby enemies. Stolen",
            "health is added to yours!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "lifedrain",
    },
    {
        FLESH_EATER, {
            "Passive ability.",
            "Take a bite out of nearby",
            "bodies. Inflicts damage on",
            "enemies and restores your",
            "health.",
            nullptr,
        }
    },
    {
        CORPSE_EXPLODE, {
            "Causes target corpse to",
            "detonate, inflicting damage",
            "on nearby enemies.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "detonatebody\nspell_corpseexplode"
    },
    {
        MIND_ABSORB, {
            "Passive ability.",
            "Inflict damage on nearby",
            "enemies and steal power",
            "cubes.",
            nullptr,
        }
    },
    {
        BLINKSTRIKE, {
            "Teleports behind an enemy for",
            "several seconds. During this",
            "time, a damage bonus applies",
            "as long as you remain unseen.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "blinkstrike",
    },
    {
        CONVERSION, {
            "Temporarily makes monsters",
            "and other summonables",
            "friendly. Uses power cubes.",
            nullptr,
        },
        .command = "convert",
    },
    {
        CLOAK, {
            "Passive ability.",
            "Become invisible after a",
            "short period of time",
            "of not moving or crawling.",
            nullptr,
        }
    },
    //NECROMANCER
    {
        MONSTER_SUMMON, {
            "Summon monsters to protect",
            "you and fight your enemies!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "Summon:\n"
        "monster [gunner|parasite\n"
        "brain|praetor|medic|tank\n"
        "mutant|gladiator|berserker\n"
        "soldier|enforcer|flyer\n"
        "floater|hover]\n"
        "Utility:\n"
        "monster [remove|command\n"
        "follow me|count|attack]",
    },
    {
        SKELETON, {
            "Raise skeletons to protect",
            "you and fight your enemies!",
            "Uses power cubes.",
            "Summon commands:",
            "skeleton [ice|poison|fire]",
            "Utility commands:",

            nullptr,
        },
        .command = "Summon:\n"
        "skeleton [ice|poison|fire]\n"
        "Utility:\n"
        "skeleton [remove|command\n"
        "follow me]",
    },
    {
        GOLEM, {
            "Raise a golem to protect",
            "you and fight your enemies!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "golem [remove|command|follow me]",
    },
    {
        HELLSPAWN, {
            "Summon a hellspawn to protect",
            "you and fight your enemies!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "hellspawn [attack|recall]",
    },
    {
        PLAGUE, {
            "Passive ability.",
            "Infects nearby enemies with",
            "a life-sapping contagion!",
            nullptr,
        }
    },
    {
        LIFE_TAP, {
            "Curse your enemies, allowing.",
            "attacks to leech life!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "lifetap",
    },
    {
        AMP_DAMAGE, {
            "Curse your enemies, causing",
            "increased damage.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "ampdamage",
    },
    {
        STATIC_FIELD, {
            "Reduces an enemy's health",
            "by a percentage. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "staticfield",
    },
    {
        CURSE, {
            "Curse your enemies, causing",
            "drunkenness and stupor!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "curse",
    },
    {
        WEAKEN, {
            "Curse your enemies, reducing",
            "the effectiveness of their",
            "attacks. Uses power cubes.",
            nullptr,
        },
        .command = "weaken",
    },
    {
        JETPACK, {
            "Pushes you to new heights,",
            "allowing air travel!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "+thrust",
    },
    //ENGINEER
    {
        PROXY, {
            "Attach a device to a surface",
            "that explodes when nearby",
            "enemies are detected. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "proxy [count|remove]",
    },
    {
        BUILD_SENTRY, {
            "Build a sentry gun to fight",
            "your enemies! Consumes ammo",
            "and power cubes.",
            nullptr,
        },
        .command = "sentry [rotate|remove]\n"
        "minisentry [beam|aim|aimall\n"
        "|remove]"
    },
    {
        SUPPLY_STATION, {
            "Build a supply station that",
            "creates and stores armor",
            "and ammunition. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "supplystation",
    },
    {
        BUILD_LASER, {
            "Attach a device to a surface",
            "that emits a laser beam.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "laser [remove]",
    },
    {
        MAGMINE, {
            "Toss a device that attracts",
            "nearby enemies, holding them",
            "in-place. Uses power cubes.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "magmine [count|remove]",
    },
    {
        CALTROPS, {
            "Drop spiked devices onto the",
            "floor. Enemies that step on",
            "them take damage and are",
            "slowed. Uses power cubes.",
            nullptr,
        },
        .command = "caltrops",
    },
    {
        AUTOCANNON, {
            "Build an autocannon that",
            "fires on enemies that cross",
            "its muzzle. Uses power cubes",
            "and ammunition.",
            nullptr,
        },
        .command = "autocannon\n[remove|aim|aimall]",
    },
    {
        DETECTOR, {
            "Attach a device to a surface",
            "that sounds an alarm on",
            "nearby enemies and attracts",
            "projectiles. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "detector [remove]",
    },
    {
        DECOY, {
            "Summons mirror images of",
            "yourself to harrass enemies.",
            "Explodes on death! Uses",
            "power cubes.",
            "Commands: decoy",
            "[remove|solid|notsolid]",
            nullptr,
        }
    },
    {
        EXPLODING_ARMOR, {
            "Toss a suit of armor that",
            "explodes and throws shrapnel.",
            "Uses armor.",
            nullptr,
        },
        .command = "armorbomb [<3-120>|remove]",
    },
    {
        ANTIGRAV, {
            "Temporarily reduces the",
            "effects of gravity, allowing",
            "you to float! Uses power",
            "cubes.",
            nullptr,
        },
        .command = "antigrav",
    },
    //SHAMAN
    {
        FIRE_TOTEM, {
            "Creates a fire totem that",
            "throws fire at nearby",
            "enemies!. Uses power cubes.",
            nullptr,
        },
        .command = "firetotem|totem remove"
    },
    {
        WATER_TOTEM, {
            "Creates a water totem that",
            "chills nearby enemies,",
            "reducing their movement and",
            "attack speed. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "watertotem|totem remove"
    },
    {
        AIR_TOTEM, {
            "Creates an air totem that",
            "absorbs damage you take.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "airtotem [protect]|\ntotem remove"
    },
    {
        EARTH_TOTEM, {
            "Creates an earth totem that",
            "provides a physical damage",
            "boost to nearby friendly",
            "players. Uses power cubes.",
            nullptr,
        },
        .command = "earthtotem|totem remove"
    },
    {
        DARK_TOTEM, {
            "Creates a darkness totem that",
            "allows friendly players to",
            "steal health with their",
            "attacks. Uses power cubes.",
            nullptr,
        },
        .command = "darknesstotem|totem remove",
    },
    {
        NATURE_TOTEM, {
            "Creates a nature totem that",
            "heals nearby allies. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "naturetotem|totem remove"
    },
    {
        HASTE, {
            "Passive ability. Increases",
            "weapon rate of fire.",
            nullptr,
        }
    },
    {
        TOTEM_MASTERY, {
            "Passive ability. Totems will",
            "automatically regenerate",
            "health.",
            nullptr,
        }
    },
    {
        SUPER_SPEED, {
            "Activate to move at double",
            "speed! Uses power cubes.",
            nullptr,
        },
        .command = "sspeed|nosspeed",
    },
    {
        FURY, {
            "Passive ability.",
            "Chance to activate the fury!",
            "Provides health regeneration,",
            "increased damage to enemies,",
            "and reduced damage from",
            "enemies while active.",
            nullptr,
        }
    },
    //MAGE
    {
        MAGICBOLT, {
            "Fires a magic bolt!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "magicbolt",
    },
    {
        NOVA, {
            "Creates a nova explosion",
            "that damages nearby enemies.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "nova",
    },
    {
        BOMB_SPELL, {
            "Causes bombs to fall from",
            "the sky or drop on top of",
            "enemies! Uses power cubes.",
            "Commands:",
            "bombspell [forward|area]",
            nullptr,
        }
    },
    {
        FORCE_WALL, {
            "Spawns a wall that sets",
            "enemies that touch it on",
            "fire, or a solid wall that",
            "provides shelter. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "forcewall [solid]"
    },
    {
        LIGHTNING, {
            "Fires a bolt of lightning",
            "that damages enemies and",
            "jumps between them. Uses",
            "power cubes.",
            "Synergy: Lightning",
            nullptr,
        },
        .command = "chainlightning",
    },
    {
        METEOR, {
            "Drops a meteor from the sky",
            "causing radius damage on",
            "impact, setting enemies",
            "on fire and throwing",
            "flames! Uses power cubes.",
            "Synergy: Fire",
            nullptr,
        },
        .command = "meteor",
    },
    {
        FIREBALL, {
            "Toss a fireball, causing",
            "radius damage on impact,",
            "setting enemies on fire and",
            "throwing flames! Uses power",
            "cubes.",
            "Synergy: Fire",
            nullptr,
        },
        .command = "fireball",
    },
    {
        LIGHTNING_STORM, {
            "Creates a lightning storm,",
            "causing bolts to shoot from",
            "the sky and strike your",
            "enemies! Uses power cubes.",
            "Synergy: Lightning",
            nullptr,
        },
        .command = "lightningstorm",
    },
    {
        FIREWALL, {
            "Creates an inferno that",
            "quickly grows to a wall of",
            "flames! Uses power cubes.",
            "Synergy: Fire",
            nullptr,
        },
        .command = "firewall",
    },
    {
        GLACIAL_SPIKE, {
            "Fires a glacial spike",
            "that damages and freezes",
            "enemies! Uses power cubes.",
            "Synergy: Ice",
            nullptr,
        },
        .command = "glacialspike",
    },
    {
        FROZEN_ORB, {
            "Fires a frozen orb that",
            "shoots icy shards to damage",
            "and slow enemies. Uses",
            "power cubes.",
            "Synergy: Ice",
            nullptr,
        },
        .command = "frozenorb",
    },
    {
        TELEPORT, {
            "Teleport in the direction you",
            "aiming! Uses power cubes.",
            nullptr,
        },
        .command = "teleport_fwd",
    },
    //CLERIC
    {
        SALVATION, {
            "Activates salvation aura,",
            "protecting yourself and",
            "allies and reducing damage",
            "received from both physical",
            "and magical sources. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "salvation",
    },
    {
        HEALING, {
            "Blesses target with healing!",
            "Restores health and armor.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "heal [self]",
    },
    {
        BLESS, {
            "Blesses target with speed,",
            "increased damage output,",
            "and damage resistance! Uses",
            "power cubes.",
            nullptr,
        },
        .command = "bless [self]",
    },
    {
        YIN, {
            "Spawns a Yin Spirit!",
            "Destroys corpses in exchange",
            "for health, armor, and ammo.",
            "Uses power cubes.",
            nullptr,
        },
        .command = "yin",
    },
    {
        YANG, {
            "Spawns a Yang Spirit!",
            "Attacks your enemies. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "yang",
    },
    {
        HAMMER, {
            "Fires a spinning magical",
            "hammer that spirals away from",
            "you, and causes damage to",
            "anything it touches. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "blessedhammer",
    },
    {
        DEFLECT, {
            "Blesses target with",
            "deflection! Causes",
            "projectiles to harmlessly",
            "bounce away. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "deflect [self]",
    },
    {
        DOUBLE_JUMP, {
            "Allows you to jump a second",
            "time while airborne!",
            "Passive ability.",
            nullptr,
        }
    },
    {
        HOLY_FREEZE, {
            "Activates holy freeze aura!",
            "Alows movement and attacks of",
            "enemies. Uses power cubes.",
            nullptr,
        },
        .command = "holyfreeze",
    },
    //KNIGHT
    {
        ARMOR_UPGRADE, {
            "Increases the effectiveness",
            "of armor. Passive ability.",
            nullptr,
        }
    },
    {
        REGENERATION, {
            "Automatically regenerates",
            "your health.",
            "Passive ability.",
            nullptr,
        }
    },
    {
        POWER_SHIELD, {
            "Activate power screen in",
            "your inventory to provide",
            "frontal protection. Each",
            "upgrade increases",
            "effectiveness.",
            nullptr,
        },
        .command = "use power screen",
    },
    {
        ARMOR_REGEN, {
            "Automatically regenerates",
            "your armor. Passive ability.",
            nullptr,
        }
    },
    {
        BEAM, {
            "Fires a laser beam!",
            nullptr,
        },
        .command = "beam_on|beam_off",
    },
    {
        PLASMA_BOLT, {
            "Fires a plasma bolt,",
            "exploding each time it",
            "impacts a surface.",
            nullptr,
        },
        .command = "plasmabolt",
    },
    {
        SHIELD, {
            //					xxxxxxxxxxxxxxxxxxxxxxxxxxxxx (max 21 lines)
            "Activate shield to provide",
            "frontal protection. Similar",
            "to power screen, but uses",
            "charge instead of cells.",
            "You can't attack while",
            "shield is activated.",
            "Commands: shieldon|shieldoff",
            nullptr,
        }
    },
    {
        BOOST_SPELL, {
            "Boosts you in the direction",
            "you are aiming!",
            nullptr,
        },
        .command = "boost",
    },
    //ALIEN
    {
        SPIKER, {
            "Spawns an organism that",
            "shoots spikes at enemies.",
            "Receives synergy bonus",
            "from spike. Users power",
            "cubes.",
            nullptr,
        },
        .command = "spiker [remove]",
    },
    {
        OBSTACLE, {
            "Spawns an organism that",
            "damages enemies that touch",
            "it. Uses power cubes.",
            nullptr,
        },
        .command = "obstacle [remove]",
    },
    {
        GASSER, {
            "Spawns an organism that",
            "spits a damaging gas cloud",
            "at enemies. Receives synergy",
            "bonus from acid. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "gasser [remove]",
    },
    {
        HEALER, {
            "Spawns an organism that",
            "heals friendly units. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "healer",
    },
    {
        SPORE, {
            "Throws a spiked organism",
            "that attacks enemies. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "spore [move|remove]",
    },
    {
        SPIKE, {
            "Fires a volley of spikes that",
            "damage and stun enemies they",
            "touch. Receives synergy bonus",
            "from spiker. Users power cubes.",
            nullptr,
        },
        .command = "+spike",
    },
    {
        ACID, {
            "Spits a volume of highly",
            "poisonous and corrosive",
            "liquid. Uses power cubes.",
            "Receives synergy bonus",
            "from gassers.",
            nullptr,
        },
        .command = "+acid",
    },
    {
        COCOON, {
            "Spawns an organism that",
            "can boost your attack damage",
            "and resistance. Uses power",
            "cubes.",
            nullptr,
        },
        .command = "cocoon",
    },
    {
        BLACKHOLE, {
            "Creates a wormhole that you",
            "can enter to temporarily",
            "move about the map in noclip",
            "mode. Use again to exit. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "wormhole",
    },
    //POLTERGEIST
    {
        MORPH_MASTERY, {
            "Adds secondary weapon modes",
            "to various morphs. Passive",
            "ability.",
            nullptr,
        }
    },
    {
        BERSERK, {
            "Morph into a berserker! Has",
            "the ability to sprint short",
            "distances with the +sprint",
            "command. Uses power cubes.",
            "Increases melee damage of",
            "other morphs.",
            nullptr,
        },
        .command = "berserker",
    },
    {
        CACODEMON, {
            "Morph into the cacodemon!",
            "Uses power cubes.",
            nullptr,
        },
        .command = "cacodemon",
    },
    {
        BLOOD_SUCKER, {
            "Morph into a parasite! Uses",
            "power cubes. Adds vampire",
            "effect to other morphs.",
            nullptr,
        },
        .command = "parasite",
    },
    {
        BRAIN, {
            "Morph into a brain! Uses",
            "power cubes.",
            nullptr,
        },
        .command = "brain",
    },
    {
        FLYER, {
            "Morph into a flyer! Uses",
            "power cubes.",
            nullptr,
        },
        .command = "flyer",
    },
    {
        MUTANT, {
            "Morph into a mutant! Uses",
            "power cubes.",
            nullptr,
        },
        .command = "mutant",
    },
    {
        TANK, {
            "Morph into a tank! Uses",
            "power cubes. Increases",
            "health of other morphs.",
            nullptr,
        },
        .command = "tank",
    },
    {
        MEDIC, {
            "Morph into a medic! Can heal",
            "allies and resurrect dead",
            "monsters, among others. Uses",
            "power cubes.",
            nullptr,
        },
        .command = "medic",
    },
    {
        FLASH, {
            "Teleports you to a random",
            "location on the map.",
            nullptr,
        },
        .command = "flash",
    },
};

const struct upgradehelp_s* vrx_upgradehelp_get(int abilityIndex) {
    for (int i = 0; i < sizeof(help) / sizeof(help[0]); i++) {
        if (help[i].abilityIndex == abilityIndex) {
            return &help[i];
        }
    }
    return nullptr;
}

int vrx_upgradehelp_add_menu_lines(edict_t *ent, const struct upgradehelp_s *help)
{
    int lines = 0;
    if (!help) {
        menu_add_line(ent, "No description available.", 0);
        return 1;
    }

    const auto line = help->descriptionLines;
    for (int i = 0; line[i] ; i++) {
        int firstline = ent->client->menustorage.num_of_lines;
        int endline = menu_add_line_nl(ent, line[i],  MENU_WHITE_CENTERED);
        lines += endline - firstline;
    }

    if (help->command) {
        int firstline = ent->client->menustorage.num_of_lines;
        menu_add_line(ent, "Command(s)", MENU_GREEN_CENTERED);
        int endline = menu_add_line_nl(ent, help->command, MENU_WHITE_CENTERED);
        lines += endline - firstline;
    }

    return lines;
}
