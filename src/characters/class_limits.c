#include "g_local.h"
#include "class_limits.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

int MAX_ARMOR(struct edict_s *ent) {
    int vitlvl = 0;
    int talentlevel;
    int value;

    if (!ent->myskills.abilities[VITALITY].disable)
        vitlvl = ent->myskills.abilities[VITALITY].current_level;

    switch (ent->myskills.class_num) {
        case CLASS_SOLDIER:
            value = INITIAL_ARMOR_SOLDIER + LEVELUP_ARMOR_SOLDIER * ent->myskills.level;
            break;
        case CLASS_VAMPIRE:
            value = INITIAL_ARMOR_VAMPIRE + LEVELUP_ARMOR_VAMPIRE * ent->myskills.level;
            break;
        case CLASS_ENGINEER:
            value = INITIAL_ARMOR_ENGINEER + LEVELUP_ARMOR_ENGINEER * ent->myskills.level;
            break;
        case CLASS_MAGE:
            value = INITIAL_ARMOR_MAGE + LEVELUP_ARMOR_MAGE * ent->myskills.level;
            break;
        case CLASS_POLTERGEIST:
            value = INITIAL_ARMOR_POLTERGEIST + LEVELUP_ARMOR_POLTERGEIST * ent->myskills.level;
            break;
        case CLASS_ALIEN:
            value = INITIAL_ARMOR_ALIEN + LEVELUP_ARMOR_ALIEN * ent->myskills.level;
            break;
        case CLASS_KNIGHT:
            value = INITIAL_ARMOR_KNIGHT + LEVELUP_ARMOR_KNIGHT * ent->myskills.level;
            break;
        case CLASS_WEAPONMASTER:
            value = INITIAL_ARMOR_WEAPONMASTER + LEVELUP_ARMOR_WEAPONMASTER * ent->myskills.level;
            break;
        case CLASS_CLERIC:
            value = INITIAL_ARMOR_CLERIC + LEVELUP_ARMOR_CLERIC * ent->myskills.level;
            break;
        case CLASS_SHAMAN:
            value = INITIAL_ARMOR_SHAMAN + LEVELUP_ARMOR_SHAMAN * ent->myskills.level;
            break;
        case CLASS_NECROMANCER:
            value = INITIAL_ARMOR_NECROMANCER + LEVELUP_ARMOR_NECROMANCER * ent->myskills.level;
            break;
        default:
            value = 100 + 5 * ent->myskills.level;
            break;
    }

    //Talent: Tactics	(weaponmaster)
    talentlevel = vrx_get_talent_level(ent, TALENT_TACTICS);
    if (talentlevel > 0) value += ent->myskills.level * talentlevel;    //+1 armor per upgrade

    value *= 1.0 + VITALITY_MULT * vitlvl;

    return value;
}

#pragma clang diagnostic pop

int MAX_HEALTH(const edict_t *ent) {
    int vitlvl = 0;
    int talentlevel;
    int value;

    // if using special flag carrier rules in CTF then the fc
    // gets health based on level and nothing else
    if (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
        return INITIAL_HEALTH_FC + ADDON_HEALTH_FC * ent->myskills.level;

    if (!ent->myskills.abilities[VITALITY].disable)
        vitlvl = ent->myskills.abilities[VITALITY].current_level;

    switch (ent->myskills.class_num) {
        case CLASS_SOLDIER:
            value = INITIAL_HEALTH_SOLDIER + LEVELUP_HEALTH_SOLDIER * ent->myskills.level;
            break;
        case CLASS_VAMPIRE:
            value = INITIAL_HEALTH_VAMPIRE + LEVELUP_HEALTH_VAMPIRE * ent->myskills.level;
            break;
        case CLASS_ENGINEER:
            value = INITIAL_HEALTH_ENGINEER + LEVELUP_HEALTH_ENGINEER * ent->myskills.level;
            break;
        case CLASS_MAGE:
            value = INITIAL_HEALTH_MAGE + LEVELUP_HEALTH_MAGE * ent->myskills.level;
            break;
        case CLASS_POLTERGEIST:
            value = INITIAL_HEALTH_POLTERGEIST + LEVELUP_HEALTH_POLTERGEIST * ent->myskills.level;
            break;
        case CLASS_ALIEN:
            value = INITIAL_HEALTH_ALIEN + LEVELUP_HEALTH_ALIEN * ent->myskills.level;
            break;
        case CLASS_KNIGHT:
            value = INITIAL_HEALTH_KNIGHT + LEVELUP_HEALTH_KNIGHT * ent->myskills.level;
            break;
        case CLASS_WEAPONMASTER:
            value = INITIAL_HEALTH_WEAPONMASTER + LEVELUP_HEALTH_WEAPONMASTER * ent->myskills.level;
            break;
        case CLASS_CLERIC:
            value = INITIAL_HEALTH_CLERIC + LEVELUP_HEALTH_CLERIC * ent->myskills.level;
            break;
        case CLASS_SHAMAN:
            value = INITIAL_HEALTH_SHAMAN + LEVELUP_HEALTH_SHAMAN * ent->myskills.level;
            break;
        case CLASS_NECROMANCER:
            value = INITIAL_HEALTH_NECROMANCER + LEVELUP_HEALTH_NECROMANCER * ent->myskills.level;
            break;
        default:
            value = 100 + 5 * ent->myskills.level;
    }

    //Note: These talents are all different so they can be tweaked individually.

    //Talent: Super Vitality	(vamps)
//	talentlevel = vrx_get_talent_level(ent, TALENT_SUPERVIT);
//	if(talentlevel > 0)		value += ent->myskills.level * talentlevel;	//+1 health per upgrade

    //Talent: Durability		(knight)
    talentlevel = vrx_get_talent_level(ent, TALENT_DURABILITY);
    if (talentlevel > 0) value += ent->myskills.level * talentlevel;    //+1 health per upgrade

    //Talent: Tactics			(weaponmaster)
    talentlevel = vrx_get_talent_level(ent, TALENT_TACTICS);
    if (talentlevel > 0) value += ent->myskills.level * talentlevel;    //+1 health per upgrade

    //Talent: Improved Vitality	(necromancer)
//	talentlevel = vrx_get_talent_level(ent, TALENT_IMP_VITALITY);
//	if(talentlevel > 0)		value += ent->myskills.level * talentlevel;	//+1 health per upgrade

    value *= 1.0 + VITALITY_MULT * vitlvl;

    return value;
}

int MAX_BULLETS(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (100 * ent->myskills.abilities[MAX_AMMO].current_level);
}

int MAX_SHELLS(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (50 * ent->myskills.abilities[MAX_AMMO].current_level);
}

int MAX_ROCKETS(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (25 * ent->myskills.abilities[MAX_AMMO].current_level);
}

int MAX_GRENADES(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (25 * ent->myskills.abilities[MAX_AMMO].current_level);
}

int MAX_CELLS(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (100 * ent->myskills.abilities[MAX_AMMO].current_level);
}

int MAX_SLUGS(struct edict_s *ent) {
    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;
    return (25 * ent->myskills.abilities[MAX_AMMO].current_level);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

int MAX_POWERCUBES(struct edict_s *ent) {
    int value = 100, clvl;

    if (ent->myskills.abilities[MAX_AMMO].disable)
        return 0;

    clvl = ent->myskills.level;

    switch (ent->myskills.class_num) {
        case CLASS_SOLDIER:
            value = INITIAL_POWERCUBES_SOLDIER + LEVELUP_POWERCUBES_SOLDIER * clvl;
            break;
        case CLASS_VAMPIRE:
            value = INITIAL_POWERCUBES_VAMPIRE + LEVELUP_POWERCUBES_VAMPIRE * clvl;
            break;
        case CLASS_KNIGHT:
            value = INITIAL_POWERCUBES_KNIGHT + LEVELUP_POWERCUBES_KNIGHT * clvl;
            break;
        case CLASS_MAGE:
            value = INITIAL_POWERCUBES_MAGE + LEVELUP_POWERCUBES_MAGE * clvl;
            break;
        case CLASS_ALIEN:
            value = INITIAL_POWERCUBES_ALIEN + LEVELUP_POWERCUBES_ALIEN * clvl;
            break;
        case CLASS_POLTERGEIST:
            value = INITIAL_POWERCUBES_POLTERGEIST + LEVELUP_POWERCUBES_POLTERGEIST * clvl;
            break;
        case CLASS_ENGINEER:
            value = INITIAL_POWERCUBES_ENGINEER + LEVELUP_POWERCUBES_ENGINEER * clvl;
            break;
        case CLASS_WEAPONMASTER:
            value = INITIAL_POWERCUBES_WEAPONMASTER + LEVELUP_POWERCUBES_WEAPONMASTER * clvl;
            break;
        case CLASS_CLERIC:
            value = INITIAL_POWERCUBES_CLERIC + LEVELUP_POWERCUBES_CLERIC * ent->myskills.level;
            break;
        case CLASS_SHAMAN:
            value = INITIAL_POWERCUBES_SHAMAN + LEVELUP_POWERCUBES_SHAMAN * ent->myskills.level;
            break;
        case CLASS_NECROMANCER:
            value = INITIAL_POWERCUBES_NECROMANCER + LEVELUP_POWERCUBES_NECROMANCER * ent->myskills.level;
            break;
        default:
            value = 200 + 10 * clvl;
    }

    return value * (1 + (0.1 * ent->myskills.abilities[MAX_AMMO].current_level));
}

#pragma clang diagnostic pop

void vrx_update_health_max(edict_t *ent) {
    ent->max_health = MAX_HEALTH(ent);
    ent->health = ent->max_health;
    ent->client->pers.health = ent->max_health;
    ent->client->pers.max_health = ent->max_health;
}

void vrx_update_all_character_maximums(edict_t *ent) {
    vrx_update_health_max(ent);

    ent->client->pers.max_bullets = 200 + MAX_BULLETS(ent);
    ent->client->pers.max_shells = 100 + MAX_SHELLS(ent);
    ent->client->pers.max_rockets = 50 + MAX_ROCKETS(ent);
    ent->client->pers.max_grenades = 50 + MAX_GRENADES(ent);
    ent->client->pers.max_cells = 200 + MAX_CELLS(ent);
    ent->client->pers.max_slugs = 50 + MAX_SLUGS(ent);

    ent->client->pers.max_powercubes = MAX_POWERCUBES(ent);
}