#include "g_local.h"
#include "../../quake2/monsterframes/m_runnertank.h"

static int sound_thud;
static int sound_pain;
static int sound_idle;
static int sound_die;
static int sound_step;
static int sound_sight;
static int sound_windup;
static int sound_strike;

#define RUNNERTANK_JUMP_ATTACK_DELAY 12.0f
#define RUNNERTANK_JUMP_ATTACK_FOV 35
#define RUNNERTANK_JUMP_ATTACK_DROP_RADIUS 90.0f
#define RUNNERTANK_JUMP_ATTACK_DROP_SPEED 900.0f
#define RUNNERTANK_JUMP_ATTACK_DROP_GRAVITY 3.0f

static void runnertank_stand(edict_t *self);
static void runnertank_walk(edict_t *self);
static void runnertank_walk_loop(edict_t *self);
static void runnertank_run(edict_t *self);
static void runnertank_attack(edict_t *self);
static void runnertank_melee(edict_t *self);
static void runnertank_restrike(edict_t *self);
static void runnertank_reattack_blast(edict_t *self);
static void runnertank_refire_rocket(edict_t *self);
static void runnertank_doattack_rocket(edict_t *self);
static void runnertank_jump_attack_takeoff(edict_t *self);
static void runnertank_jump_attack_hold(edict_t *self);

static void runnertank_footstep(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void runnertank_thud(edict_t *self)
{
    gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

static void runnertank_windup(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

static void runnertank_slam_effect(edict_t *self)
{
    vec3_t forward, right, start, offset, up;
    trace_t tr;

    AngleVectors(self->s.angles, forward, right, NULL);
    VectorSet(offset, 20, -14.3f, -21);
    G_ProjectSource(self->s.origin, offset, forward, right, start);
    tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_SOLID);
    VectorSet(up, 0, 0, 1);

    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_BERSERK_SLAM);
    gi.WritePosition(tr.endpos);
    gi.WriteDir(up);
    gi.multicast(tr.endpos, MULTICAST_PHS);
}

static void runnertank_idle(edict_t *self)
{
    gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
    self->superspeed = false;
}

static void runnertank_sight(edict_t *self, edict_t *other)
{
    (void)other;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

mframe_t runnertank_frames_stand[] =
{
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL
};
mmove_t runnertank_move_stand = { FRAME_stand01, FRAME_stand30, runnertank_frames_stand, NULL };

static void runnertank_stand(edict_t *self)
{
    self->monsterinfo.currentmove = &runnertank_move_stand;
}

mframe_t runnertank_frames_walk_start[] =
{
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL,
    drone_ai_walk, 0, NULL
};
mmove_t runnertank_move_walk_start = { FRAME_walk01, FRAME_walk15, runnertank_frames_walk_start, runnertank_walk_loop };

mframe_t runnertank_frames_walk[] =
{
    drone_ai_walk, 4, runnertank_footstep,
    drone_ai_walk, 4, NULL,
    drone_ai_walk, 3, NULL,
    drone_ai_walk, 5, NULL,
    drone_ai_walk, 4, NULL,
    drone_ai_walk, 5, NULL,
    drone_ai_walk, 7, NULL,
    drone_ai_walk, 7, NULL,
    drone_ai_walk, 6, runnertank_footstep,
    drone_ai_walk, 6, NULL,
    drone_ai_walk, 4, NULL,
    drone_ai_walk, 5, NULL,
    drone_ai_walk, 3, NULL,
    drone_ai_walk, 4, NULL,
    drone_ai_walk, 6, NULL,
	drone_ai_walk, 7, NULL,
    drone_ai_walk, 5, NULL
};
mmove_t runnertank_move_walk = { FRAME_walk22, FRAME_walk38, runnertank_frames_walk, NULL };

static void runnertank_walk_loop(edict_t *self)
{
    if (!self->goalentity)
        self->goalentity = world;
    self->monsterinfo.currentmove = &runnertank_move_walk;
}

static void runnertank_walk(edict_t *self)
{
    if (!self->goalentity)
        self->goalentity = world;
        
    if (self->s.frame >= FRAME_walk22 && self->s.frame <= FRAME_walk38)
        self->monsterinfo.currentmove = &runnertank_move_walk;
    else
        self->monsterinfo.currentmove = &runnertank_move_walk_start;
}

mframe_t runnertank_frames_run[] =
{
    drone_ai_run, 14, runnertank_footstep,
    drone_ai_run, 18, NULL,
    drone_ai_run, 15, NULL,
    drone_ai_run, 15, NULL,
    drone_ai_run, 15, NULL,
    drone_ai_run, 19, runnertank_footstep,
    drone_ai_run, 15, NULL,
    drone_ai_run, 13, NULL,
    drone_ai_run, 18, NULL,
    drone_ai_run, 17, NULL
};
mmove_t runnertank_move_run = { FRAME_run01, FRAME_run10, runnertank_frames_run, NULL };

static void runnertank_run(edict_t *self)
{
    if (self->deadflag == DEAD_DEAD)
        return;

    if (self->enemy && self->enemy->client)
        self->monsterinfo.aiflags |= AI_BRUTAL;
    else
        self->monsterinfo.aiflags &= ~AI_BRUTAL;

    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
    {
        self->monsterinfo.currentmove = &runnertank_move_stand;
        return;
    }

    self->monsterinfo.currentmove = &runnertank_move_run;
}

static void runnertank_rail(edict_t *self)
{
    int flash_number, damage;
    vec3_t forward, start;

    if (self->s.frame == FRAME_attak110)
        flash_number = MZ2_TANK_BLASTER_1;
    else if (self->s.frame == FRAME_attak113)
        flash_number = MZ2_TANK_BLASTER_2;
    else
        flash_number = MZ2_TANK_BLASTER_3;

    damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
    if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
        damage = M_RAILGUN_DMG_MAX;

    MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash_number, forward, start);
    monster_fire_railgun(self, start, forward, damage, damage, MZ2_GLADIATOR_RAILGUN_1);
}

static void runnertank_rocket(edict_t *self)
{
    int flash_number, damage, speed;
    vec3_t forward, start;

    if (!self->enemy || !self->enemy->inuse)
        return;

    if (self->s.frame == FRAME_attak324)
        flash_number = MZ2_TANK_ROCKET_1;
    else if (self->s.frame == FRAME_attak325)
        flash_number = MZ2_TANK_ROCKET_2;
    else
        flash_number = MZ2_TANK_ROCKET_3;

    damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
    if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
        damage = M_ROCKETLAUNCHER_DMG_MAX;
    speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
    if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
        speed = M_ROCKETLAUNCHER_SPEED_MAX;

    MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash_number, forward, start);
    monster_fire_rocket(self, start, forward, damage, speed, flash_number);
}

static void runnertank_plasma(edict_t *self)
{
    vec3_t forward, start;
    int flash_number, damage, speed, radius_damage;

    if (!self->enemy || !self->enemy->inuse)
        return;

    flash_number = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);
    damage = M_PLASMA_DMG_BASE + M_PLASMA_DMG_ADDON * drone_damagelevel(self);
    if (M_PLASMA_DMG_MAX && damage > M_PLASMA_DMG_MAX)
        damage = M_PLASMA_DMG_MAX;
    speed = M_PLASMA_SPEED_BASE + M_PLASMA_SPEED_ADDON * drone_damagelevel(self);
    if (M_PLASMA_SPEED_MAX && speed > M_PLASMA_SPEED_MAX)
        speed = M_PLASMA_SPEED_MAX;
    radius_damage = max(1, damage / 2);

    MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
    fire_plasma(self, start, forward, damage, speed, M_PLASMA_DAMAGE_RADIUS, radius_damage);
}

static void runnertank_strike_sound(edict_t *self)
{
    gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);
}

static void runnertank_meleeattack(edict_t *self)
{
    int damage;
    trace_t tr;
    edict_t *other = NULL;
    vec3_t v;

    if (!self->groundentity)
        return;

    damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
    if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
        damage = M_MELEE_DMG_MAX;

    gi.sound(self, CHAN_AUTO, sound_strike, 1, ATTN_NORM, 0);
    runnertank_slam_effect(self);

    while ((other = findradius(other, self->s.origin, 128)) != NULL)
    {
        if (!G_ValidTarget(self, other, true, true))
            continue;
        if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
            continue;

        VectorSubtract(other->s.origin, self->s.origin, v);
        VectorNormalize(v);
        tr = gi.trace(self->s.origin, NULL, NULL, other->s.origin, self, (MASK_PLAYERSOLID | MASK_MONSTERSOLID));
        T_Damage(other, self, self, v, tr.endpos, tr.plane.normal, damage, 200, 0, MOD_TANK_PUNCH);
    }
}

static void runnertank_attack_finished(edict_t *self)
{
    M_DelayNextAttack(self, 0.5, false);
    runnertank_run(self);
}

mframe_t runnertank_frames_attack_blast[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rail,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rail,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rail
};
mmove_t runnertank_move_attack_blast = { FRAME_attak101, FRAME_attak116, runnertank_frames_attack_blast, runnertank_reattack_blast };

mframe_t runnertank_frames_reattack_blast[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rail,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rail
};
mmove_t runnertank_move_reattack_blast = { FRAME_attak111, FRAME_attak116, runnertank_frames_reattack_blast, runnertank_reattack_blast };

mframe_t runnertank_frames_attack_post_blast[] =
{
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 2, NULL,
    ai_move, 3, NULL,
    ai_move, 2, NULL,
    ai_move, -2, runnertank_footstep
};
mmove_t runnertank_move_attack_post_blast = { FRAME_attak117, FRAME_attak122, runnertank_frames_attack_post_blast, runnertank_attack_finished };

static void runnertank_reattack_blast(edict_t *self)
{
    if (!G_ValidTarget(self, self->enemy, true, true) || !visible(self, self->enemy))
    {
        self->monsterinfo.currentmove = &runnertank_move_attack_post_blast;
        M_DelayNextAttack(self, 0, true);
        return;
    }

    M_ContinueAttack(self, &runnertank_move_reattack_blast, &runnertank_move_attack_post_blast, 0, 1024, 0.5);
}

mframe_t runnertank_frames_strike[] =
{
    ai_move, 0, runnertank_windup,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, runnertank_meleeattack
};
mmove_t runnertank_move_strike = { FRAME_attak222, FRAME_attak226, runnertank_frames_strike, runnertank_restrike };

mframe_t runnertank_frames_post_strike[] =
{
    ai_move, 0, NULL,
    ai_move, -1, NULL,
    ai_move, -1, NULL,
    ai_move, -1, NULL,
    ai_move, -1, NULL,
    ai_move, -1, NULL,
    ai_move, -3, NULL,
    ai_move, -10, NULL,
    ai_move, -10, NULL,
    ai_move, -2, NULL,
    ai_move, -3, NULL,
    ai_move, -2, runnertank_footstep
};
mmove_t runnertank_move_post_strike = { FRAME_attak227, FRAME_attak238, runnertank_frames_post_strike, runnertank_attack_finished };

static void runnertank_restrike(edict_t *self)
{
    M_ContinueAttack(self, &runnertank_move_strike, &runnertank_move_post_strike, 0, 128, 0.66);
}

mframe_t runnertank_frames_attack_pre_rocket[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL
};
mmove_t runnertank_move_attack_pre_rocket = { FRAME_attak303, FRAME_attak312, runnertank_frames_attack_pre_rocket, runnertank_doattack_rocket };

mframe_t runnertank_frames_attack_fire_rocket[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_rocket,
    ai_charge, 1, NULL,
    ai_charge, 2, NULL,
    ai_charge, 7, runnertank_rocket,
    ai_charge, 7, NULL,
    ai_charge, 7, runnertank_footstep,
    ai_charge, 0, runnertank_rocket,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL
};
mmove_t runnertank_move_attack_fire_rocket = { FRAME_attak312, FRAME_attak321, runnertank_frames_attack_fire_rocket, runnertank_refire_rocket };

mframe_t runnertank_frames_attack_post_rocket[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, -9, NULL,
    ai_charge, -8, NULL,
    ai_charge, -7, NULL,
    ai_charge, -1, NULL,
    ai_charge, -1, runnertank_footstep,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, NULL
};
mmove_t runnertank_move_attack_post_rocket = { FRAME_attak322, FRAME_attak335, runnertank_frames_attack_post_rocket, runnertank_attack_finished };

static void runnertank_refire_rocket(edict_t *self)
{
    if (!G_ValidTarget(self, self->enemy, true, true) || !visible(self, self->enemy))
    {
        self->monsterinfo.currentmove = &runnertank_move_attack_post_rocket;
        M_DelayNextAttack(self, 0, true);
        return;
    }

    M_ContinueAttack(self, &runnertank_move_attack_fire_rocket, &runnertank_move_attack_post_rocket, 0, 768, 0.35);
}

static void runnertank_doattack_rocket(edict_t *self)
{
    self->monsterinfo.currentmove = &runnertank_move_attack_fire_rocket;
}

mframe_t runnertank_frames_attack_chain[] =
{
    ai_charge, 0, NULL,
    ai_charge, 0, NULL,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, runnertank_plasma,
    ai_charge, 0, NULL
};
mmove_t runnertank_move_attack_chain = { FRAME_attak404, FRAME_attak415, runnertank_frames_attack_chain, runnertank_attack_finished };

static void runnertank_jump_attack_takeoff(edict_t *self)
{
    const int speed = 800;
    vec3_t forward;
    float height_diff;

    if (!G_ValidTarget(self, self->enemy, true, true))
        return;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    self->lastsound = level.framenum;

    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] += 1;
    self->groundentity = NULL;

    VectorScale(forward, speed, self->velocity);
    height_diff = self->enemy->absmin[2] - self->absmin[2];
    self->velocity[2] = 260;
    if (height_diff > 32)
        self->velocity[2] += min(height_diff, 120);

    self->gravity = 1.35;
    self->monsterinfo.pausetime = level.time + 1.6;
    self->monsterinfo.melee_finished = level.time + RUNNERTANK_JUMP_ATTACK_DELAY;
    self->monsterinfo.aiflags |= AI_DUCKED;
}

static void runnertank_jump_attack_hold(edict_t *self)
{
    vec3_t v;
    qboolean close_enough = false;

    if (G_ValidTarget(self, self->enemy, true, true))
    {
        VectorSubtract(self->enemy->s.origin, self->s.origin, v);
        self->ideal_yaw = vectoyaw(v);
        M_ChangeYaw(self);
        close_enough = entdist(self, self->enemy) <= RUNNERTANK_JUMP_ATTACK_DROP_RADIUS;

        if (!self->groundentity && close_enough)
        {
            self->velocity[0] *= 0.35f;
            self->velocity[1] *= 0.35f;
            if (self->velocity[2] > -RUNNERTANK_JUMP_ATTACK_DROP_SPEED)
                self->velocity[2] = -RUNNERTANK_JUMP_ATTACK_DROP_SPEED;
            self->gravity = RUNNERTANK_JUMP_ATTACK_DROP_GRAVITY;
            if (self->monsterinfo.pausetime > level.time + 0.35f)
                self->monsterinfo.pausetime = level.time + 0.35f;
        }
    }

    if (self->groundentity || (self->waterlevel > 1) || (level.time > self->monsterinfo.pausetime))
    {
        self->monsterinfo.aiflags &= ~(AI_HOLD_FRAME | AI_DUCKED);
        self->gravity = 1.0;

        if (self->groundentity && G_ValidTarget(self, self->enemy, true, true) && entdist(self, self->enemy) <= RUNNERTANK_JUMP_ATTACK_DROP_RADIUS)
            self->monsterinfo.currentmove = &runnertank_move_strike;
    }
    else
    {
        self->monsterinfo.aiflags |= AI_HOLD_FRAME;
    }
}

mframe_t runnertank_frames_jump_attack[] =
{
    ai_charge, 15, NULL,
    ai_move, 0, runnertank_jump_attack_takeoff,
    ai_move, 0, runnertank_jump_attack_hold,
    ai_move, 0, runnertank_jump_attack_hold,
    ai_move, 0, runnertank_jump_attack_hold
};
mmove_t runnertank_move_jump_attack = { FRAME_run01, FRAME_run05, runnertank_frames_jump_attack, runnertank_attack_finished };

static void runnertank_start_jump_attack(edict_t *self)
{
    self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
    self->monsterinfo.currentmove = &runnertank_move_jump_attack;
    self->monsterinfo.nextframe = FRAME_run01;
}

static qboolean runnertank_can_jump_attack(edict_t *self, float range)
{
    float height_diff;

    if (level.time < self->monsterinfo.melee_finished)
        return false;
    if (!self->groundentity || (self->waterlevel > 1))
        return false;
    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
        return false;
    if ((range <= 128) || (range > 512))
        return false;
    if (!nearfov(self, self->enemy, 0, RUNNERTANK_JUMP_ATTACK_FOV))
        return false;

    height_diff = self->enemy->absmin[2] - self->absmin[2];
    return (height_diff > -64) && (height_diff < 128);
}

static void runnertank_melee(edict_t *self)
{
    float range;

    if (!G_ValidTarget(self, self->enemy, true, true))
        return;

    range = entdist(self, self->enemy);
    if ((range <= 128) && self->groundentity)
        self->monsterinfo.currentmove = &runnertank_move_strike;
    else if (runnertank_can_jump_attack(self, range))
        runnertank_start_jump_attack(self);
}

static void runnertank_attack(edict_t *self)
{
    float r, range;
    qboolean attack_started = false;

    if (!G_ValidTarget(self, self->enemy, true, true))
        return;

    r = random();
    range = entdist(self, self->enemy);

    if ((range <= 128) && self->groundentity)
    {
        self->monsterinfo.currentmove = &runnertank_move_strike;
        attack_started = true;
    }
    else if (runnertank_can_jump_attack(self, range))
    {
        runnertank_start_jump_attack(self);
        attack_started = true;
    }
    else if (range <= 256)
    {
        if (r <= 0.35)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_chain;
            attack_started = true;
        }
        else if (r <= 0.5)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_pre_rocket;
            attack_started = true;
        }
    }
    else if (range <= 640)
    {
        if (r <= 0.25)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_chain;
            attack_started = true;
        }
        else if (r <= 0.55)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_blast;
            attack_started = true;
        }
        else if (r <= 0.7)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_pre_rocket;
            attack_started = true;
        }
    }
    else
    {
        if (r <= 0.35)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_blast;
            attack_started = true;
        }
        else if (r <= 0.55)
        {
            self->monsterinfo.currentmove = &runnertank_move_attack_pre_rocket;
            attack_started = true;
        }
    }

    if (attack_started)
        M_DelayNextAttack(self, 0, true);
}

mframe_t runnertank_frames_pain1[] =
{
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL
};
mmove_t runnertank_move_pain1 = { FRAME_pain201, FRAME_pain204, runnertank_frames_pain1, runnertank_run };

mframe_t runnertank_frames_pain3[] =
{
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, runnertank_footstep
};
mmove_t runnertank_move_pain3 = { FRAME_pain301, FRAME_pain316, runnertank_frames_pain3, runnertank_run };

static void runnertank_pain(edict_t *self, edict_t *other, float kick, int damage)
{
    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + 3.0;
    gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

    if (skill->value == 3)
        return;

    if (damage <= 30 || random() < 0.5)
        self->monsterinfo.currentmove = &runnertank_move_pain1;
    else
        self->monsterinfo.currentmove = &runnertank_move_pain3;
}

static void runnertank_dead(edict_t *self)
{
    VectorSet(self->mins, -16, -16, -16);
    VectorSet(self->maxs, 16, 16, 0);
    self->movetype = MOVETYPE_TOSS;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
    M_PrepBodyRemoval(self);
}

static void runnertank_shrink(edict_t *self)
{
    self->maxs[2] = 0;
    self->svflags |= SVF_DEADMONSTER;
    gi.linkentity(self);
}

mframe_t runnertank_frames_death[] =
{
    ai_move, -7, NULL,
    ai_move, -2, NULL,
    ai_move, -2, NULL,
    ai_move, 1, NULL,
    ai_move, 3, NULL,
    ai_move, 6, NULL,
    ai_move, 1, NULL,
    ai_move, 1, NULL,
    ai_move, 2, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, -2, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, -3, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, -4, NULL,
    ai_move, -6, NULL,
    ai_move, -4, NULL,
    ai_move, -5, NULL,
    ai_move, -7, runnertank_shrink,
    ai_move, -15, runnertank_thud,
    ai_move, -5, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL,
    ai_move, 0, NULL
};
mmove_t runnertank_move_death = { FRAME_death01, FRAME_death32, runnertank_frames_death, runnertank_dead };

static void runnertank_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
    int n;

    M_Notify(self);

#ifdef OLD_NOLAG_STYLE
    if (nolag->value)
    {
        M_Remove(self, false, true);
        return;
    }
#endif

    if (self->health <= self->gib_health)
    {
        gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        if (vrx_spawn_nonessential_ent(self->s.origin))
        {
            for (n = 0; n < 1; n++)
                ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
            for (n = 0; n < 4; n++)
                ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
            ThrowGib(self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
        }
#ifdef OLD_NOLAG_STYLE
        M_Remove(self, false, false);
#else
        if (nolag->value)
            M_Remove(self, false, true);
        else
            M_Remove(self, false, false);
#endif
        return;
    }

    if (self->deadflag == DEAD_DEAD)
        return;

    DroneList_Remove(self);

    gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
    self->deadflag = DEAD_DEAD;
    self->takedamage = DAMAGE_YES;
    self->monsterinfo.currentmove = &runnertank_move_death;

    if (self->activator && !self->activator->client)
        self->activator->num_monsters_real--;
}

void init_drone_runnertank(edict_t *self)
{
    self->s.modelindex = gi.modelindex("models/vault/monsters/tank/tris.md2");
    VectorSet(self->mins, -28, -28, -14);
    VectorSet(self->maxs, 28, 28, 56);
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;

    sound_pain = gi.soundindex("tank/tnkpain2.wav");
    sound_idle = gi.soundindex("tank/tnkidle1.wav");
    sound_die = gi.soundindex("tank/death.wav");
    sound_step = gi.soundindex("tank/step.wav");
    sound_windup = gi.soundindex("tank/tnkatck4.wav");
    sound_strike = gi.soundindex("tank/tnkatck5.wav");
    sound_sight = gi.soundindex("tank/sight1.wav");
    sound_thud = gi.soundindex("tank/tnkdeth2.wav");

    gi.soundindex("tank/tnkatck1.wav");
    gi.soundindex("tank/tnkatk2a.wav");
    gi.soundindex("tank/tnkatk2b.wav");
    gi.soundindex("tank/tnkatk2c.wav");
    gi.soundindex("tank/tnkatk2d.wav");
    gi.soundindex("tank/tnkatk2e.wav");
    gi.soundindex("tank/tnkatck3.wav");

    self->health = M_RUNNERTANK_INITIAL_HEALTH + M_RUNNERTANK_ADDON_HEALTH * self->monsterinfo.level;
    self->max_health = self->health;
    self->gib_health = -200;
    self->mass = 500;

    self->pain = runnertank_pain;
    self->die = runnertank_die;

    self->monsterinfo.stand = runnertank_stand;
    self->monsterinfo.walk = runnertank_walk;
    self->monsterinfo.run = runnertank_run;
    self->monsterinfo.attack = runnertank_attack;
    self->monsterinfo.melee = runnertank_melee;
    self->monsterinfo.sight = runnertank_sight;
    self->monsterinfo.idle = runnertank_idle;
    self->monsterinfo.jumpup = 64;
    self->monsterinfo.jumpdn = 512;
    self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

    self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
    self->monsterinfo.power_armor_power = M_RUNNERTANK_INITIAL_ARMOR + M_RUNNERTANK_ADDON_ARMOR * self->monsterinfo.level;
    self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
    self->monsterinfo.control_cost = M_TANK_CONTROL_COST;
    self->monsterinfo.cost = M_TANK_COST;
    self->mtype = M_RUNNERTANK;
    self->s.renderfx |= RF_CUSTOMSKIN;
    self->s.skinnum = gi.imageindex("models/vault/monsters/tank/skin.pcx");

    gi.linkentity(self);

    self->monsterinfo.currentmove = &runnertank_move_stand;
    self->monsterinfo.scale = MODEL_SCALE;
    self->nextthink = level.time + FRAMETIME;
}
