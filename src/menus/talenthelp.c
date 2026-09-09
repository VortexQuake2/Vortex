#include "g_local.h"
#include "upgradehelp.h"
#include "characters/Talents.h"

//Soldier talents
const struct upgradehelp_s help[] = {
    {
        TALENT_IMP_STRENGTH, {
            "Increases damage,",
            "but reduces resist.",
            nullptr,
        }
    },
    {
        TALENT_IMP_RESIST, {
            "Increases resist,",
            "but reduces damage.",
            nullptr,
        }
    },
    {
        TALENT_BLOOD_OF_ARES, {
            "Increases the damage you",
            "give/take as you spree.",
            nullptr,
        }
    },
    {
        TALENT_BASIC_HA, {
            "Increases ammo pickups.",
            nullptr,
        }
    },
    {
        TALENT_BOMBARDIER, {
            "Reduces self-inflicted",
            "grenade damage and",
            "reduces cost.",
            nullptr,
        }
    },
    //Poltergeist talents
    {
        TALENT_MELEE_MASTERY, {
            "Upgrades the attack of",
            "the following morphs:",
            "Berserker, Mutant",
            "Parasite, and Brain.",
            nullptr,
        }
    },
    {
        TALENT_MORE_AMMO, {
            "Increases maximum ammo",
            "capacity for",
            "tank/caco/flyer/medic.",
            nullptr,
        }
    },
    {
        TALENT_SUPERIORITY, {
            "Increased damage and",
            "resistance to monsters.",
            nullptr,
        }
    },
    {
        TALENT_RANGE_MASTERY, {
            "Upgrades the attack of",
            "the following morphs:",
            "Tank, Medic, Flyer",
            "and Cacodemon.",
            nullptr,
        }
    },
    {
        TALENT_PACK_ANIMAL, {
            "Increased damage and",
            "resistance when near",
            "friendly morphed",
            "players.",
            nullptr,
        }
    },
    //Vampire talents
    {
        TALENT_IMP_CLOAK, {
            "Move while cloaked",
            "(must be crouching).",
            "1/3 pc cost at night!",
            nullptr,
        }
    },
    {
        TALENT_ARMOR_VAMP, {
            "Also gain armor using",
            "your vampire skill.",
            nullptr,
        }
    },
    {
        TALENT_FATAL_WOUND, {
            "Adds chance for flesh",
            "eater to make the",
            "victim bleed out.",
            nullptr,
        }
    },
    {
        TALENT_SECOND_CHANCE, {
            "100% chance of ghost",
            "working when hit.",
            nullptr,
        }
    },
    {
        TALENT_IMP_MINDABSORB, {
            "Increases frequency of",
            "mind absorb attacks.",
            nullptr,
        }
    },
    {
        TALENT_CANNIBALISM, {
            "Increases your maximum",
            "health using corpse eater.",
            nullptr,
        }
    },
    //Mage talents
    {
        TALENT_MEDITATION, {
            "Recharge your power",
            "cubes at a whim (cmd '+manacharge').",
            nullptr,
        },
    },
    {
        TALENT_OVERLOAD, {
            "Use extra power cubes",
            "to overload abilities,",
            "increasing their",
            "effectiveness",
            "(cmd 'overload').",
            nullptr,
        }
    },
    {
        TALENT_WIZARDRY, {
            "Switches spell timers to be",
            "ability-specific instead of",
            "global, allowing you to use",
            "them simultaneously!",
            nullptr,
        }
    },
    {
        TALENT_NOVA_ORB, {
            "Adds chance for frozen orb",
            "to explode into a frost nova!",
            nullptr,
        }
    },
    {
        TALENT_CL_STORM, {
            "Adds chance for lightning",
            "storms to fire chain",
            "lightning!",
            nullptr,
        }
    },
    {
        TALENT_METEORIC_FIRE, {
            "Adds chance for meteor",
            "to create a firewall on",
            "impact.",
            nullptr,
        }
    },
    {
        TALENT_MANASHIELD, {
            "Reduces physical damage",
            "by 80%%. All damage",
            "absorbed consumes power",
            "cubes. (cmd manashield)",
            nullptr,
        }
    },
    //Engineer talents
    {
        TALENT_LASER_PLATFORM, {
            "Create a laser platform",
            "(cmd 'laserplatform').",
            nullptr,
        }
    },
    {
        TALENT_ALARM, {
            "Detected enemies take",
            "more damage.",
            nullptr,
        }
    },
    {
        TALENT_RAPID_ASSEMBLY, {
            "Reduces build time.",
            "Can't be combined with",
            "Precision Tune.",
            nullptr,
        }
    },
    {
        TALENT_PRECISION_TUNING, {
            "Increased cost and",
            "build time to build",
            "higher level devices.",
            "Can't be combined with",
            "Rapid Assembly.",
            nullptr,
        }
    },
    {
        TALENT_STORAGE_UPGRADE, {
            "Increases ammunition",
            "capacity of SS/sentry/AC.",
            nullptr,
        }
    },
    //Knight talents
    {
        TALENT_REPEL, {
            "Adds chance for projectiles",
            "to deflect from shield.",
            nullptr,
        }
    },
    {
        TALENT_MAG_BOOTS, {
            "Reduces effect of knockback.",
            nullptr,
        }
    },
    {
        TALENT_LEAP_ATTACK, {
            "Adds stun/knockback effect",
            "to boost spell when landing.",
            nullptr,
        }
    },
    {
        TALENT_MOBILITY, {
            "Reduces boost cooldown",
            nullptr,
        }
    },
    {
        TALENT_DURABILITY, {
            "Increases your health",
            "per level bonus!",
            nullptr,
        }
    },
    //Cleric talents
    {
        TALENT_BALANCESPIRIT, {
            "New spirit that can",
            "use the skills of both",
            "yin and yang spirits.",
            nullptr,
        }
    },
    {
        TALENT_HOLY_GROUND, {
            "Designate an area as",
            "holy ground to regenerate",
            "teammates (cmd 'holyground').",
            nullptr,
        }
    },
    {
        TALENT_UNHOLY_GROUND, {
            "Designate an area as",
            "unholy ground to damage",
            "enemies (cmd 'unholyground').",
            nullptr,
        }
    },
    {
        TALENT_BOOMERANG, {
            "Turns blessed hammers",
            "into boomerangs",
            "(cmd 'boomerang').",
            nullptr,
        }
    },
    {
        TALENT_PURGE, {
            "Removes curses and grants",
            "temporary invincibility",
            "and immunity to curses",
            "(cmd 'purge').",
            nullptr,
        }
    },
    //Weaponmaster talents
    {
        TALENT_BASIC_AMMO_REGEN, {
            "Basic ammo regeneration.",
            "Regenerates one ammo pack",
            "for the weapon in use.",
            nullptr,
        }
    },
    {
        TALENT_COMBAT_EXP, {
            "Increases physical,",
            "damage, but reduces",
            "resistance.",
            nullptr,
        }
    },
    {
        TALENT_TACTICS, {
            "Increases your levelup",
            "health and armor bonus!",
            nullptr,
        }
    },
    {
        TALENT_SIDEARMS, {
            "Gives you additional",
            "respawn weapons. Weapon",
            "choice is determined by",
            "weapon upgrade level.",
            nullptr,
        }
    },
    //Necromancer talents
    {
        TALENT_HELLSPAWN_MASTERY, {
            "Improves hellspawn. Adds",
            "secondary attack.",
            nullptr,
        }
    },
    {
        TALENT_GOLEM_MASTERY, {
            "Improves golem. Adds thorns",
            "aura, causing damage",
            "inflicted on your golem to",
            "be reflected back to the",
            "enemy!",
            nullptr,
        }
    },
    {
        TALENT_CORPULENCE, {
            "Increases monster health/armor",
            "Can't combine with Life Tap.",
            nullptr,
        }
    },
    {
        TALENT_OBLATION, {
            "Increases monster damage.",
            "Can't combine with",
            "Corpulence.",
            nullptr,
        }
    },
    {
        TALENT_AUTOCURSE, {
            "Adds chance to",
            "automatically curse",
            "enemies that attack you.",
            nullptr,
        }
    },
    {
        TALENT_BLACK_DEATH, {
            "Enemies that touch infected",
            "corpses will take extra",
            "damage from plague.",
            nullptr,
        }
    },
    //Shaman talents
    {
        TALENT_TOTEM, {
            "Allows you to spawn",
            "healthier totems. Totem can not",
            "be of the opposite element.",
            nullptr,
        }
    },
    {
        TALENT_ICE, {
            "Allows your water totem",
            "to shoot frozen orbs.",
            nullptr,
        }
    },
    {
        TALENT_WIND, {
            "Allows your air totem to",
            "ghost attacks for you.",
            nullptr,
        }
    },
    {
        TALENT_STONE, {
            "Allows your earth totem to",
            "increase your resistance.",
            nullptr,
        }
    },
    {
        TALENT_SHADOW, {
            "Allows your darkness totem",
            "to let you vamp beyond your",
            "maximum health limit.",
            nullptr,
        }
    },
    {
        TALENT_PEACE, {
            "Allows your nature totem to",
            "regenerate your power cubes.",
            nullptr,
        }
    },
    {
        TALENT_VOLCANIC, {
            "Gives your fire totem a",
            "secondary meteor attack.",
            nullptr,
        }
    },
    //Alien talents
    {
        TALENT_SPITTING_GASSER, {
            "Adds chance that acid will",
            "spawn a gas cloud on impact.",
            nullptr,
        }
    },
    {
        TALENT_SUPER_HEALER, {
            "Allows healer to heal",
            "beyond maximum health.",
            nullptr,
        }
    },
    {
        TALENT_DEADLY_SPIKES, {
            "Adds chance that",
            "spikers will stun.",
            nullptr,
        }
    },
    {
        TALENT_SWARMING, {
            "Increases spore damage.",
            nullptr,
        }
    },
    {
        TALENT_MAGNETISM, {
            "Makes obstacles magnetic,",
            "causing enemies to be",
            "pulled toward them.",
            nullptr,
        }
    },
    {
        TALENT_TELECOON, {
            "Allows cocoon to teleport",
            "friendly units. Upgrades",
            "increase range.",
            nullptr,
        }
    },
    // Kamikaze talents
    {
        TALENT_MARTYR, {
            "Creates an explotion",
            "when you die.",
            nullptr,
        }
    },
    {
        TALENT_BLAST_RESIST, {
            "Increases defense against",
            "radius damage.",
            nullptr,
        }
    },
    {
        TALENT_MAGMINESELF, {
            "Gain the ability",
            "to turn into a living magmine",
            "using 'magmine self'.",
            nullptr,
        }
    },
    {
        TALENT_INSTANTPROXYS, {
            "Makes proxys be removed",
            "instantly when they explode.",
            "On level 2, it removes",
            "hold time when building them.",
            nullptr,
        }
    },
};

const struct upgradehelp_s* vrx_talenthelp_get(int abilityIndex) {
    for (int i = 0; i < sizeof(help) / sizeof(help[0]); i++) {
        if (help[i].abilityIndex == abilityIndex) {
            return &help[i];
        }
    }
    return nullptr;
}