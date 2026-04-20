// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

hover

This file combines the original hover monster (blaster) with the Xatrix expansion
hover (rocket), as well as daedalus variants (blaster2 and grenades).

==============================================================================
*/

#include "g_local.h"
#include "m_hover.h"
#include "m_flash.h"
#include "shared.h"
#include "horde/g_horde_scaling.h"
#include "monster_constants.h"

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_death1;
static cached_soundindex sound_death2;
static cached_soundindex sound_sight;
static cached_soundindex sound_search1;
static cached_soundindex sound_search2;

// ROGUE
// daedalus sounds
static cached_soundindex daed_sound_pain1;
static cached_soundindex daed_sound_pain2;
static cached_soundindex daed_sound_death1;
static cached_soundindex daed_sound_death2;
static cached_soundindex daed_sound_sight;
static cached_soundindex daed_sound_search1;
static cached_soundindex daed_sound_search2;
// ROGUE

// FIX: Forward declare the master spawn function so other spawn functions can call it.
void SP_monster_hover(edict_t* self);

// FIX: This helper function is now the single source of truth for identifying a Daedalus.
bool IsDaedalusType(const edict_t* ent)
{
    if (!ent) return false;
    const auto id = static_cast<horde::MonsterTypeID>(ent->monsterinfo.monster_type_id);
    return (id == horde::MonsterTypeID::DAEDALUS ||
            id == horde::MonsterTypeID::DAEDALUS_BOMBER);
}

struct hover_style_t
{
    enum weapon_t
    {
        Blaster = 0,
        Rocket = 1,
        Blaster2 = 2,
        Grenade = 3
    };

    weapon_t weapon;

    // This constructor is now reliable because the monster_type_id is set correctly at spawn.
    hover_style_t(edict_t* self)
    {
          //  gi.Com_PrintFmt("Hover attacking with ID: {}\n", self->monsterinfo.monster_type_id);

        // Use a switch on the definitive monster_type_id
        switch (static_cast<horde::MonsterTypeID>(self->monsterinfo.monster_type_id))
        {
            case horde::MonsterTypeID::HOVER:
                weapon = Rocket;
                break;
            case horde::MonsterTypeID::DAEDALUS:
                weapon = Blaster2;
                break;
            case horde::MonsterTypeID::DAEDALUS_BOMBER:
                weapon = Grenade;
                break;
            case horde::MonsterTypeID::HOVER_VANILLA:
            default: // Fallback to the standard blaster
                weapon = Blaster;
                break;
        }
    }

    // Keep these as constexpr since they only depend on the weapon value
    constexpr bool is_vanilla() const { return weapon < Blaster; }
    constexpr bool is_xatrix() const { return weapon >= Blaster2; }
    constexpr bool has_blaster() const { return weapon == Blaster; }
    constexpr bool has_rocket() const { return weapon == Rocket; }
    constexpr bool has_blaster2() const { return weapon == Blaster2; }
    constexpr bool has_grenade() const { return weapon == Grenade; }
};

MONSTERINFO_SIGHT(hover_sight) (edict_t* self, edict_t* other) -> void
{
    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (!IsDaedalusType(self))
        gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_VOICE, daed_sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(hover_search) (edict_t* self) -> void
{
    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (!IsDaedalusType(self))
    {
        if (frandom() < 0.5f)
            gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
    }
    else
    {
        if (frandom() < 0.5f)
            gi.sound(self, CHAN_VOICE, daed_sound_search1, 1, ATTN_NORM, 0);
        else
            gi.sound(self, CHAN_VOICE, daed_sound_search2, 1, ATTN_NORM, 0);
    }
}

void hover_run(edict_t* self);
void hover_dead(edict_t* self);
void hover_attack(edict_t* self);
void hover_reattack(edict_t* self);
void hover_fire_weapon(edict_t* self);

mframe_t hover_frames_stand[] = {
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
    { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(hover_move_stand) = { FRAME_stand01, FRAME_stand30, hover_frames_stand, nullptr };

mframe_t hover_frames_pain3[] = {
    { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
    { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(hover_move_pain3) = { FRAME_pain301, FRAME_pain309, hover_frames_pain3, hover_run };

mframe_t hover_frames_pain2[] = {
    { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
    { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
    { ai_move }, { ai_move }
};
MMOVE_T(hover_move_pain2) = { FRAME_pain201, FRAME_pain212, hover_frames_pain2, hover_run };

mframe_t hover_frames_pain1[] = {
    { ai_move }, { ai_move }, { ai_move, 2 }, { ai_move, -8 }, { ai_move, -4 },
    { ai_move, -6 }, { ai_move, -4 }, { ai_move, -3 }, { ai_move, 1 }, { ai_move },
    { ai_move }, { ai_move }, { ai_move, 3 }, { ai_move, 1 }, { ai_move },
    { ai_move, 2 }, { ai_move, 3 }, { ai_move, 2 }, { ai_move, 7 }, { ai_move, 1 },
    { ai_move }, { ai_move }, { ai_move, 2 }, { ai_move }, { ai_move },
    { ai_move, 5 }, { ai_move, 3 }, { ai_move, 4 }
};
MMOVE_T(hover_move_pain1) = { FRAME_pain101, FRAME_pain128, hover_frames_pain1, hover_run };

mframe_t hover_frames_walk[] = {
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 },
    { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }, { ai_walk, 4 }
};
MMOVE_T(hover_move_walk) = { FRAME_forwrd01, FRAME_forwrd35, hover_frames_walk, nullptr };

mframe_t hover_frames_run[] = {
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
    { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }
};
MMOVE_T(hover_move_run) = { FRAME_forwrd01, FRAME_forwrd35, hover_frames_run, nullptr };

static void hover_gib(edict_t* self)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS, false);

    self->s.skinnum /= 2;

    ThrowGibs(self, 150, {
        { 2, "models/objects/gibs/sm_meat/tris.md2" },
        { 2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
        { "models/monsters/hover/gibs/chest.md2", GIB_SKINNED },
        { 2, "models/monsters/hover/gibs/ring.md2", GIB_SKINNED | GIB_METALLIC },
        { 2, "models/monsters/hover/gibs/foot.md2", GIB_SKINNED },
        { "models/monsters/hover/gibs/head.md2", GIB_SKINNED | GIB_HEAD },
        });
}

THINK(hover_deadthink) (edict_t* self) -> void
{
    if (!self->groundentity && level.time < self->timestamp)
    {
        self->nextthink = level.time + FRAME_TIME_S;
        return;
    }
    hover_gib(self);
}

void hover_dying(edict_t* self)
{
    if (self->groundentity)
    {
        hover_deadthink(self);
        return;
    }
    if (brandom())
        return;
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_PLAIN_EXPLOSION);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PHS, false);
    if (brandom())
        ThrowGibs(self, 120, { { "models/objects/gibs/sm_meat/tris.md2" } });
    else
        ThrowGibs(self, 120, { { "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC } });
}

mframe_t hover_frames_death1[] = {
    { ai_move }, { ai_move, 0.f, hover_dying }, { ai_move }, { ai_move, 0.f, hover_dying },
    { ai_move }, { ai_move, 0.f, hover_dying }, { ai_move, -10, hover_dying }, { ai_move, 3 },
    { ai_move, 5, hover_dying }, { ai_move, 4, hover_dying }, { ai_move, 7 }
};
MMOVE_T(hover_move_death1) = { FRAME_death101, FRAME_death111, hover_frames_death1, hover_dead };

mframe_t hover_frames_start_attack[] = {
    { ai_charge, 1 }, { ai_charge, 1 }, { ai_charge, 1 }
};
MMOVE_T(hover_move_start_attack) = { FRAME_attak101, FRAME_attak103, hover_frames_start_attack, hover_attack };

mframe_t hover_frames_attack1[] = {
    { ai_charge, -10, hover_fire_weapon },
    { ai_charge, -10, hover_fire_weapon },
    { ai_charge, 0, hover_reattack },
};
MMOVE_T(hover_move_attack1) = { FRAME_attak104, FRAME_attak106, hover_frames_attack1, nullptr };

mframe_t hover_frames_end_attack[] = {
    { ai_charge, 1 }, { ai_charge, 1 }
};
MMOVE_T(hover_move_end_attack) = { FRAME_attak107, FRAME_attak108, hover_frames_end_attack, hover_run };

mframe_t hover_frames_attack2[] = {
    { ai_charge, 10, hover_fire_weapon },
    { ai_charge, 10, hover_fire_weapon },
    { ai_charge, 10, hover_reattack },
};
MMOVE_T(hover_move_attack2) = { FRAME_attak104, FRAME_attak106, hover_frames_attack2, nullptr };

void hover_reattack(edict_t* self)
{
    hover_style_t style(self);
    float reattack_chance = 0.5f;

    // Daedalus with Blaster2 is more aggressive
    if (style.has_blaster2())
        reattack_chance = 0.6f;
    // Daedalus with Grenades is less aggressive
    else if (style.has_grenade())
        reattack_chance = 0.4f;

    // FIX: Add a check to ensure the enemy is valid before accessing its members.
    if (self->enemy && self->enemy->inuse && self->enemy->health > 0)
    {
        if (visible(self, self->enemy))
        {
            if (frandom() <= reattack_chance)
            {
                if (self->monsterinfo.attack_state == AS_STRAIGHT)
                {
                    M_SetAnimation(self, &hover_move_attack1);
                    return;
                }
                else if (self->monsterinfo.attack_state == AS_SLIDING)
                {
                    M_SetAnimation(self, &hover_move_attack2);
                    return;
                }
            }
        }
    }
    M_SetAnimation(self, &hover_move_end_attack);
}

void hover_fire_blaster(edict_t* self)
{
    if (!M_HasValidTarget(self))
    {
        return; // Stop immediately if the target is invalid.
    }

    vec3_t start, forward, right, end, dir;
    int config_speed = M_BLASTER_SPEED(self);
    int blasterSpeed = config_speed > 0 ? config_speed : 1230;
    AngleVectors(self->s.angles, forward, right, nullptr);
    vec3_t const o = monster_flash_offset[(self->s.frame & 1) ? MZ2_HOVER_BLASTER_2 : MZ2_HOVER_BLASTER_1];
    start = M_ProjectFlashSource(self, o, forward, right);
    end = self->enemy->s.origin;
    end[2] += self->enemy->viewheight;

    // Check if muzzle origin can see enemy before firing
    trace_t trace = gi.traceline(start, end, self, MASK_PROJECTILE);
    if (!(trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP))
        return;

    dir = end - start;
    dir.normalize();
    PredictAim(self, self->enemy, start, blasterSpeed / 1.5, true, 0.f, &dir, &end);
    int damage = M_BLASTER_DMG(self);
    monster_fire_blaster(self, start, dir, damage > 0 ? damage : 12, blasterSpeed,
        (self->s.frame & 1) ? MZ2_HOVER_BLASTER_2 : MZ2_HOVER_BLASTER_1,
        (self->s.frame % 4) ? EF_NONE : EF_BLASTER);
}

void hover_fire_blaster2(edict_t* self)
{
    if (!M_HasValidTarget(self))
    {
        return; // Stop immediately if the target is invalid.
    }

    vec3_t start, forward, right, end, dir;
    int config_speed = M_BLASTER2_SPEED(self);
    int blasterSpeed = config_speed > 0 ? config_speed : 1230;
    AngleVectors(self->s.angles, forward, right, nullptr);
    vec3_t const o = monster_flash_offset[(self->s.frame & 1) ? MZ2_HOVER_BLASTER_2 : MZ2_HOVER_BLASTER_1];
    start = M_ProjectFlashSource(self, o, forward, right);
    end = self->enemy->s.origin;
    end[2] += self->enemy->viewheight;

    // Check if muzzle origin can see enemy before firing
    trace_t trace = gi.traceline(start, end, self, MASK_PROJECTILE);
    if (!(trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP))
        return;

    dir = end - start;
    dir.normalize();
    PredictAim(self, self->enemy, start, blasterSpeed / 1.5, true, 0.f, &dir, &end);
    int damage = M_BLASTER2_DMG(self);
    monster_fire_blaster2(self, start, dir, damage > 0 ? damage : 12, blasterSpeed,
        (self->s.frame & 1) ? MZ2_DAEDALUS_BLASTER_2 : MZ2_DAEDALUS_BLASTER,
        (self->s.frame % 4) ? EF_NONE : EF_BLASTER);
}

void hover_fire_rocket(edict_t* self)
{
    // Basic enemy check - blindfire logic needs to execute
    if (!M_HasEnemy(self))
        return;

    vec3_t forward, right, start, dir, vec, target;
    trace_t trace;
    int config_speed = M_ROCKET_SPEED(self);
    int rocketSpeed = config_speed > 0 ? config_speed : 850;
    bool blindfire = (self->monsterinfo.aiflags & AI_MANUAL_STEERING);
    AngleVectors(self->s.angles, forward, right, nullptr);
    start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right);

    if (blindfire && !visible(self, self->enemy))
    {
        if (!self->monsterinfo.blind_fire_target)
            return;
        target = self->monsterinfo.blind_fire_target;
        vec = target;
        dir = vec - start;
    }
    else
    {
        // Not blindfiring - need fully valid target
        if (!M_HasValidTarget(self))
            return;

        target = self->enemy->s.origin;
        if (frandom() < 0.33f || (start[2] < self->enemy->absmin[2])) {
            vec = target;
            vec[2] += self->enemy->viewheight;
            dir = vec - start;
        } else {
            vec = target;
            vec[2] = self->enemy->absmin[2] + 1;
            dir = vec - start;
        }
        if (frandom() < 0.35f)
            PredictAim(self, self->enemy, start, rocketSpeed / 1.5, false, 0.f, &dir, &vec);
    }

    dir.normalize();
    trace = gi.traceline(start, vec, self, MASK_PROJECTILE);
    if (trace.fraction > 0.5f || trace.ent == self->enemy || trace.ent->solid != SOLID_BSP)
        monster_fire_rocket(self, start, dir, M_ROCKET_DMG(self), M_ROCKET_SPEED(self), MZ2_BOSS2_ROCKET_3);
}

void hover_fire_grenades(edict_t* self)
{
    // Basic enemy check - blindfire logic needs to execute
    if (!M_HasEnemy(self))
        return;

    vec3_t forward, right, up, aim, target, offset{};
    monster_muzzleflash_id_t flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_1;
    bool blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;

    if (self->s.frame == FRAME_attak104) offset = { 1.7f, 7.0f, 11.3f };
    else if (self->s.frame == FRAME_attak105) offset = { 1.7f, -7.0f, 11.3f };

    AngleVectors(self->s.angles, forward, right, up);
    const vec3_t start = G_ProjectSource2(self->s.origin, offset, forward, right, up);

    // PMM - blindfire support
    if (blindfire && !visible(self, self->enemy))
    {
        if (!self->monsterinfo.blind_fire_target)
            return;
        target = self->monsterinfo.blind_fire_target;
        aim = target - start;
    }
    else
    {
        // Not blindfiring - need fully valid target
        if (!M_HasValidTarget(self))
            return;

        target = self->enemy->s.origin;
        PredictAim(self, self->enemy, start, 800, false, 0.f, &aim, nullptr);
    }
    // pmm

    aim.normalize();
    monster_fire_grenade(self, start, aim, M_GRENADE_DMG(self), M_GRENADE_SPEED(self), flash_number,
        (crandom_open() * 10.0f), 200.f + (crandom_open() * 10.0f));
}

void hover_fire_weapon(edict_t* self)
{
    if (!M_HasValidTarget(self))
    {
        return; // Stop immediately if the target is invalid.
    }

    hover_style_t style(self);
    if (style.has_blaster()) hover_fire_blaster(self);
    else if (style.has_rocket()) hover_fire_rocket(self);
    else if (style.has_blaster2()) hover_fire_blaster2(self);
    else if (style.has_grenade()) hover_fire_grenades(self);
}

MONSTERINFO_STAND(hover_stand) (edict_t* self) -> void { M_SetAnimation(self, &hover_move_stand); }
MONSTERINFO_RUN(hover_run) (edict_t* self) -> void {
    if (self->monsterinfo.aiflags & AI_STAND_GROUND) M_SetAnimation(self, &hover_move_stand);
    else M_SetAnimation(self, &hover_move_run);
}
MONSTERINFO_WALK(hover_walk) (edict_t* self) -> void { M_SetAnimation(self, &hover_move_walk); }
MONSTERINFO_ATTACK(hover_start_attack) (edict_t* self) -> void { M_SetAnimation(self, &hover_move_start_attack); }

void hover_attack(edict_t* self)
{
    monster_done_dodge(self);

    hover_style_t style(self);
    float strafe_chance = 0.5f;

    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (IsDaedalusType(self))
        strafe_chance += 0.1f;
    if (style.has_rocket())
        strafe_chance += 0.1f;
    if (style.has_grenade())
        strafe_chance -= 0.15f;

    if (frandom() > strafe_chance) {
        M_SetAnimation(self, &hover_move_attack1);
        self->monsterinfo.attack_state = AS_STRAIGHT;
    } else {
        if (frandom() <= 0.5f) self->monsterinfo.lefty = !self->monsterinfo.lefty;
        M_SetAnimation(self, &hover_move_attack2);
        self->monsterinfo.attack_state = AS_SLIDING;
    }
}

PAIN(hover_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
    if (level.time < self->pain_debounce_time) return;
    self->pain_debounce_time = level.time + 3_sec;

    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (!IsDaedalusType(self)) {
        if (frandom() < 0.5f) gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
        else gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
    } else {
        if (frandom() < 0.5f) gi.sound(self, CHAN_VOICE, daed_sound_pain1, 1, ATTN_NORM, 0);
        else gi.sound(self, CHAN_VOICE, daed_sound_pain2, 1, ATTN_NORM, 0);
    }

    if (!M_ShouldReactToPain(self, mod)) return;

    // Apply knockback velocity away from damage source
    if (other && other->inuse)
    {
        vec3_t knockback_dir = (self->s.origin - other->s.origin).normalized();
        float knockback_strength = min(200.f, 50.f + damage * 2.f);
        self->velocity += knockback_dir * knockback_strength;
    }

    if (damage <= 25) {
        if (frandom() < 0.5f) M_SetAnimation(self, &hover_move_pain3);
        else M_SetAnimation(self, &hover_move_pain2);
    } else {
        if (frandom() < 0.3f) M_SetAnimation(self, &hover_move_pain1);
        else M_SetAnimation(self, &hover_move_pain2);
    }
}

MONSTERINFO_SETSKIN(hover_setskin) (edict_t* self) -> void
{
    if (self->health < (self->max_health / 2)) self->s.skinnum |= 1;
    else self->s.skinnum &= ~1;
}

void hover_dead(edict_t* self)
{
    self->mins = { -16, -16, -24 };
    self->maxs = { 16, 16, -8 };
    self->movetype = MOVETYPE_TOSS;
    self->think = hover_deadthink;
    self->nextthink = level.time + FRAME_TIME_S;
    self->timestamp = level.time + 15_sec;
    gi.linkentity(self);
}

DIE(hover_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
    self->s.effects = EF_NONE;
    self->monsterinfo.power_armor_type = IT_NULL;
    if (M_CheckGib(self, mod)) {
        hover_gib(self);
        return;
    }
    if (self->deadflag) return;

    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (!IsDaedalusType(self)) {
        if (frandom() < 0.5f) gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
        else gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
    } else {
        if (frandom() < 0.5f) gi.sound(self, CHAN_VOICE, daed_sound_death1, 1, ATTN_NORM, 0);
        else gi.sound(self, CHAN_VOICE, daed_sound_death2, 1, ATTN_NORM, 0);
    }

    self->deadflag = true;
    self->takedamage = true;
    M_SetAnimation(self, &hover_move_death1);
}

static void hover_set_fly_parameters(edict_t* self)
{
    hover_style_t style(self);

    // FIX: Use the reliable IsDaedalusType helper instead of checking mass.
    if (!IsDaedalusType(self))
    {
        self->monsterinfo.fly_thrusters = false;
        self->monsterinfo.fly_acceleration = 20.f;
        self->monsterinfo.fly_speed = 270.f;
        self->monsterinfo.fly_min_distance = 325.f;
        self->monsterinfo.fly_max_distance = 670.f;
    }
    else // Is a Daedalus
    {
        self->monsterinfo.fly_thrusters = false;
        self->monsterinfo.fly_acceleration = 20.f;
        if (style.has_grenade()) {
            self->monsterinfo.fly_speed = 320.f;
            self->monsterinfo.fly_min_distance = 500.f;
            self->monsterinfo.fly_max_distance = 850.f;
        } else { // Blaster2 Daedalus
            self->monsterinfo.fly_speed = 290.f;
            self->monsterinfo.fly_min_distance = 400.f;
            self->monsterinfo.fly_max_distance = 850.f;
        }
    }
}

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
This is the master spawn function for all hover variants.
*/
void SP_monster_hover(edict_t* self)
{
    // FIX: This is the new central logic. It sets the monster_type_id from the classname
    // if it hasn't been set already. This makes the system work for all spawn methods.
	if (self->monsterinfo.monster_type_id == MONSTER_TYPE_UNKNOWN) // Check if it hasn't been set yet
		self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::HOVER);

	const spawn_temp_t& st = ED_GetSpawnTemp();

    if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
    gi.modelindex("models/monsters/hover/gibs/chest.md2");
    gi.modelindex("models/monsters/hover/gibs/foot.md2");
    gi.modelindex("models/monsters/hover/gibs/head.md2");
    gi.modelindex("models/monsters/hover/gibs/ring.md2");

    self->mins = { -24, -24, -24 };
    self->maxs = { 24, 24, 32 };

    int base_health = M_HOVER_INITIAL_HEALTH;
    if (g_horde && g_horde->integer && current_wave_level > 0) {
        bool is_boss = self->monsterinfo.IS_BOSS && !self->monsterinfo.BOSS_DEATH_HANDLED;
        self->health = ScaleMonsterHealth(base_health, current_wave_level, is_boss);
    } else {
        self->health = base_health * st.health_multiplier;
    }
    self->gib_health = -100;

    // Only set mass if it hasn't been set already (for non-Daedalus types).
    if (self->mass <= 150) {
        self->mass = 150;
    }

	// Power armor configuration
	if (!st.was_key_specified("power_armor_type") && M_HOVER_POWER_ARMOR_TYPE != IT_NULL) {
		self->monsterinfo.power_armor_type = static_cast<item_id_t>(M_HOVER_POWER_ARMOR_TYPE);
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = M_HOVER_ADDON_POWER_ARMOR(self);
	}

	// Regular armor configuration
	if (!st.was_key_specified("armor_type") && M_HOVER_INITIAL_ARMOR > 0) {
		self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
		if (!st.was_key_specified("armor_power"))
			self->monsterinfo.armor_power = M_HOVER_ADDON_ARMOR(self);
	}

    self->pain = hover_pain;
    self->die = hover_die;
    self->s.scale = 1.15f;
    self->monsterinfo.stand = hover_stand;
    self->monsterinfo.walk = hover_walk;
    self->monsterinfo.run = hover_run;
    self->monsterinfo.attack = hover_start_attack;
    self->monsterinfo.sight = hover_sight;
    self->monsterinfo.search = hover_search;
    self->monsterinfo.setskin = hover_setskin;

    // Standard hover sound setup
    self->yaw_speed = 18;
    sound_pain1.assign("hover/hovpain1.wav");
    sound_pain2.assign("hover/hovpain2.wav");
    sound_death1.assign("hover/hovdeth1.wav");
    sound_death2.assign("hover/hovdeth2.wav");
    sound_sight.assign("hover/hovsght1.wav");
    sound_search1.assign("hover/hovsrch1.wav");
    sound_search2.assign("hover/hovsrch2.wav");
    gi.soundindex("hover/hovatck1.wav");
    self->monsterinfo.engine_sound = gi.soundindex("hover/hovidle1.wav");

    gi.linkentity(self);
    M_SetAnimation(self, &hover_move_stand);
    self->monsterinfo.scale = MODEL_SCALE;
    flymonster_start(self);
    self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
    hover_set_fly_parameters(self);
    ApplyMonsterBonusFlags(self);
}

// FIX: This function is for the "monster_hover" classname (Rocket Hover).
// It just needs to call the master spawn function.
/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
This is now the rocket variant.
*/
// The engine links "monster_hover" to SP_monster_hover, so we don't need a separate function.

// FIX: This function is for the "monster_hover_vanilla" classname (Blaster Hover).
/*QUAKED monster_hover_vanilla (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_hover_vanilla(edict_t* self)
{
    const spawn_temp_t& st = ED_GetSpawnTemp();

    self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::HOVER_VANILLA);    SP_monster_hover(self);

     	// Power armor configuration
	if (!st.was_key_specified("power_armor_type") && M_HOVER_VANILLA_POWER_ARMOR_TYPE != IT_NULL) {
		self->monsterinfo.power_armor_type = static_cast<item_id_t>(M_HOVER_VANILLA_POWER_ARMOR_TYPE);
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = M_HOVER_VANILLA_ADDON_POWER_ARMOR(self);
	}

	// Regular armor configuration
	if (!st.was_key_specified("armor_type") && M_HOVER_VANILLA_INITIAL_ARMOR > 0) {
		self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
		if (!st.was_key_specified("armor_power"))
			self->monsterinfo.armor_power = M_HOVER_VANILLA_ADDON_ARMOR(self);
	}
    
}

// FIX: This function is for the "monster_daedalus" classname (Blaster2 Daedalus).
/*QUAKED monster_daedalus (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_daedalus(edict_t* self)
{
    const spawn_temp_t& st = ED_GetSpawnTemp();

	    if (self->monsterinfo.monster_type_id == MONSTER_TYPE_UNKNOWN) // Check if it hasn't been set yet
        self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::DAEDALUS); 

        if (!M_AllowSpawn(self)) {
        G_FreeEdict(self);
        return;
    }

    int base_health = M_DAEDALUS_INITIAL_HEALTH;
    if (g_horde && g_horde->integer && current_wave_level > 0) {
        bool is_boss = self->monsterinfo.IS_BOSS && !self->monsterinfo.BOSS_DEATH_HANDLED;
        self->health = ScaleMonsterHealth(base_health, current_wave_level, is_boss);
    } else {
        self->health = base_health * st.health_multiplier;
    }
    self->s.skinnum = 2;
    // Set properties common to ALL Daedalus types
    self->mass = 225;
    self->yaw_speed = 23;
	// Power armor configuration
	if (!st.was_key_specified("power_armor_type") && M_DAEDALUS_POWER_ARMOR_TYPE != IT_NULL) {
		self->monsterinfo.power_armor_type = static_cast<item_id_t>(M_DAEDALUS_POWER_ARMOR_TYPE);
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = M_DAEDALUS_ADDON_POWER_ARMOR(self);
	}

	// Regular armor configuration
	if (!st.was_key_specified("armor_type") && M_DAEDALUS_INITIAL_ARMOR > 0) {
		self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
		if (!st.was_key_specified("armor_power"))
			self->monsterinfo.armor_power = M_DAEDALUS_ADDON_ARMOR(self);
	}
    
    daed_sound_pain1.assign("daedalus/daedpain1.wav");
    daed_sound_pain2.assign("daedalus/daedpain2.wav");
    daed_sound_death1.assign("daedalus/daeddeth1.wav");
    daed_sound_death2.assign("daedalus/daeddeth2.wav");
    daed_sound_sight.assign("daedalus/daedsght1.wav");
    daed_sound_search1.assign("daedalus/daedsrch1.wav");
    daed_sound_search2.assign("daedalus/daedsrch2.wav");

    // Now call the master hover spawn function.
    SP_monster_hover(self);
}

// FIX: This function is for the "monster_daedalus_bomber" classname (Grenade Daedalus).
/*QUAKED monster_daedalus_bomber (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_daedalus_bomber(edict_t* self)
{
	const spawn_temp_t &st = ED_GetSpawnTemp();

    self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::DAEDALUS_BOMBER);    // A grenade Daedalus IS a Daedalus. Call its spawn function first
    // to set up mass, sounds, power armor, etc.
    SP_monster_daedalus(self);

	// Power armor configuration
	if (!st.was_key_specified("power_armor_type") && M_DAEDALUS_BOMBER_POWER_ARMOR_TYPE != IT_NULL) {
		self->monsterinfo.power_armor_type = static_cast<item_id_t>(M_DAEDALUS_BOMBER_POWER_ARMOR_TYPE);
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = M_DAEDALUS_BOMBER_ADDON_POWER_ARMOR(self);
	}

	// Regular armor configuration
	if (!st.was_key_specified("armor_type") && M_DAEDALUS_BOMBER_INITIAL_ARMOR > 0) {
		self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
		if (!st.was_key_specified("armor_power"))
			self->monsterinfo.armor_power = M_DAEDALUS_BOMBER_ADDON_ARMOR(self);
	}

	int base_health = M_DAEDALUS_BOMBER_INITIAL_HEALTH;
	if (g_horde && g_horde->integer && current_wave_level > 0) {
		bool is_boss = self->monsterinfo.IS_BOSS && !self->monsterinfo.BOSS_DEATH_HANDLED;
		self->health = ScaleMonsterHealth(base_health, current_wave_level, is_boss);
	} else {
		self->health = base_health * st.health_multiplier;
	}
    // The classname is still "monster_daedalus_bomber", so when SP_monster_hover
    // is eventually called, it will get the correct ID from the registry.
}