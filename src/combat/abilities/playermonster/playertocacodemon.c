#include "../../../quake2/g_local.h"

#define CACODEMON_FRAME_IDLE_START        0
#define CACODEMON_FRAME_IDLE_END        3
#define CACODEMON_FRAME_BLINK_START        4
#define CACODEMON_FRAME_BLINK_END        7
#define CACODEMON_FRAME_ATTACK_START    8
#define CACODEMON_FRAME_ATTACK_FIRE        11
#define CACODEMON_FRAME_ATTACK_END        14

// additional parameters in morph.h

void bskull_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf) {
    int num;
    int damage = CACODEMON_ADDON_BURN * self->owner->myskills.abilities[CACODEMON].current_level;

    // deal direct damage
    if (G_EntExists(other))
        T_Damage(other, self, self->owner, self->velocity, self->s.origin,
                 plane->normal, self->dmg, 1, DAMAGE_RADIUS, MOD_CACODEMON_FIREBALL);
    // deal radius damage
    T_RadiusDamage(self, self->owner, self->radius_dmg, other,
                   self->dmg_radius, MOD_CACODEMON_FIREBALL);
/*
	if (self->owner->myskills.abilities[MORPH_MASTERY].current_level > 0)
	{
		num = GetRandom(4, 8);
		damage *= 1.5;
	}
	else
*/
    num = GetRandom(3, 6);

    SpawnFlames(self->owner, self->s.origin, num, damage, 100);
    BecomeExplosion1(self);
}

void bskull_think(edict_t *self) {
    if (!G_EntIsAlive(self->owner) || (self->delay < level.time)) {
        G_FreeEdict(self);
        return;
    }
    self->nextthink = level.time + FRAMETIME;
}

void fire_skull(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius) {
    edict_t *skull;

    skull = G_Spawn();
    VectorCopy(start, skull->s.origin);
    VectorCopy(dir, skull->movedir);
    vectoangles(dir, skull->s.angles);
    VectorScale(dir, speed, skull->velocity);
    skull->movetype = MOVETYPE_TOSS;
    skull->clipmask = MASK_SHOT;
    skull->solid = SOLID_BBOX;
    VectorClear (skull->mins);
    VectorClear (skull->maxs);
    skull->s.modelindex = gi.modelindex("models/objects/gibs/skull/tris.md2");
    skull->owner = self;
    skull->touch = bskull_touch;
    skull->dmg = damage;
    skull->s.effects = EF_GIB;
    skull->radius_dmg = damage;
    skull->dmg_radius = damage_radius;
    skull->classname = "skull";
    skull->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
    skull->delay = level.time + 10;
    skull->think = bskull_think;
    skull->nextthink = level.time + FRAMETIME;
//	if (self->client)
//		check_dodge (self, skull->s.origin, dir, speed, damage_radius);
    gi.linkentity(skull);
}

void cacodemon_attack(edict_t *ent) {
    int damage, radius;
    vec3_t forward, right, start, offset;

    // check for sufficient ammo
    if (!ent->myskills.abilities[CACODEMON].ammo)
        return;

    if (level.time > ent->monsterinfo.attack_finished) {
        damage =
                (CACODEMON_INITIAL_DAMAGE + CACODEMON_ADDON_DAMAGE * ent->myskills.abilities[CACODEMON].current_level) *
                2;
        radius = CACODEMON_INITIAL_RADIUS + CACODEMON_ADDON_RADIUS * ent->myskills.abilities[CACODEMON].current_level;

        ent->s.frame = CACODEMON_FRAME_ATTACK_FIRE;

        AngleVectors(ent->client->v_angle, forward, right, NULL);
        VectorScale(forward, -3, ent->client->kick_origin);
        VectorSet(offset, 0, 7, ent->viewheight - 8);
        P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

        fire_skull(ent, start, forward, damage, CACODEMON_SKULL_SPEED, radius);
        ent->monsterinfo.attack_finished = level.time + CACODEMON_REFIRE;

        // use ammo
        ent->myskills.abilities[CACODEMON].ammo--;
    }
}

void RunCacodemonFrames(edict_t *ent, usercmd_t *ucmd) {
    int frame;

    // if we aren't a cacodemon or we are dead, we shouldn't be here!
    if ((ent->mtype != MORPH_CACODEMON) || (ent->deadflag == DEAD_DEAD))
        return;

    if (level.framenum >= ent->count) {
        MorphRegenerate(ent, qf2sf(CACODEMON_REGEN_DELAY), qf2sf(CACODEMON_REGEN_FRAMES));
        //	ent->client->ability_delay = level.time + CACODEMON_DELAY; // can't use abilities

        if (ent->client->buttons & BUTTON_ATTACK) {
            ent->client->idle_frames = 0;
            // run attack frames
            G_RunFrames(ent, CACODEMON_FRAME_ATTACK_START, CACODEMON_FRAME_ATTACK_END, false);
            cacodemon_attack(ent);
        } else {
            frame = ent->s.frame;
            if (((random() <= 0.5) && (frame == CACODEMON_FRAME_IDLE_END))
                || ((frame >= CACODEMON_FRAME_BLINK_START) && (frame < CACODEMON_FRAME_BLINK_END)))
                // run blink frames
                G_RunFrames(ent, CACODEMON_FRAME_BLINK_START, CACODEMON_FRAME_BLINK_END, false);
            else
                // run idle frames
                G_RunFrames(ent, CACODEMON_FRAME_IDLE_START, CACODEMON_FRAME_IDLE_END, false);
        }

        // add thrust
        if (ucmd->upmove > 0) {
            if (ent->groundentity)
                ent->velocity[2] = 150;
            else if (ent->velocity[2] < 0)
                ent->velocity[2] += 200;
            else
                ent->velocity[2] += 100;

        }

        ent->count = (int)(level.framenum + (0.1 / FRAMETIME));
    }
}

void Cmd_PlayerToCacodemon_f(edict_t *ent) {
    vec3_t mins, maxs;
    trace_t tr;
    int caco_cubecost = CACODEMON_INIT_COST;
    //Talent: More Ammo
    int talentLevel = vrx_get_talent_level(ent, TALENT_MORE_AMMO);

    if (debuginfo->value)
        gi.dprintf("DEBUG: %s just called Cmd_PlayerToCacodemon_f()\n", ent->client->pers.netname);

    // try to switch back
    if (ent->mtype || PM_PlayerHasMonster(ent)) {
        // don't let a player-tank unmorph if they are cocooned
        if (ent->owner && ent->owner->inuse && ent->owner->movetype == MOVETYPE_NONE)
            return;

        if (que_typeexists(ent->curses, 0)) {
            safe_cprintf(ent, PRINT_HIGH, "You can't morph while cursed!\n");
            return;
        }

        V_RestoreMorphed(ent, 0);
        return;
    }

    if (HasFlag(ent) && !hw->value) {
        safe_cprintf(ent, PRINT_HIGH, "Can't morph while carrying flag!\n");
        return;
    }

    //Talent: Morphing
    if (vrx_get_talent_slot(ent, TALENT_MORPHING) != -1)
        caco_cubecost *= 1.0 - 0.25 * vrx_get_talent_level(ent, TALENT_MORPHING);

    //if (!G_CanUseAbilities(ent, ent->myskills.abilities[CACODEMON].current_level, caco_cubecost))
    //	return;
    if (!V_CanUseAbilities(ent, CACODEMON, caco_cubecost, true))
        return;

    // don't get stuck
    VectorSet(mins, -24, -24, -24);
    VectorSet(maxs, 24, 24, 24);
    tr = gi.trace(ent->s.origin, mins, maxs, ent->s.origin, ent, MASK_SHOT);
    if (tr.fraction < 1) {
        safe_cprintf(ent, PRINT_HIGH, "Not enough room to morph.\n");
        return;
    }

    V_ModifyMorphedHealth(ent, MORPH_CACODEMON, true);

    VectorCopy(mins, ent->mins);
    VectorCopy(maxs, ent->maxs);

    ent->client->pers.inventory[power_cube_index] -= caco_cubecost;
    ent->client->ability_delay = level.time + CACODEMON_DELAY;

    ent->mtype = MORPH_CACODEMON;
    ent->s.modelindex = gi.modelindex("models/monsters/idg2/head/tris.md2");
    ent->s.modelindex2 = 0;
    ent->s.skinnum = 0;

    ent->monsterinfo.attack_finished = level.time + 0.5;// can't attack immediately

    // set maximum skull ammo
    ent->myskills.abilities[CACODEMON].max_ammo = CACODEMON_SKULL_INITIAL_AMMO + CACODEMON_SKULL_ADDON_AMMO
                                                                                 *
                                                                                 ent->myskills.abilities[CACODEMON].current_level;

    // Talent: More Ammo
    // increases ammo 10% per talent level
    if (talentLevel > 0) ent->myskills.abilities[CACODEMON].max_ammo *= 1.0 + 0.1 * talentLevel;

    // give them some starting ammo
    ent->myskills.abilities[CACODEMON].ammo = CACODEMON_SKULL_START_AMMO;

    ent->client->refire_frames = 0; // reset charged weapon
    ent->client->weapon_mode = 0; // reset weapon mode
    ent->client->pers.weapon = NULL;
    ent->client->ps.gunindex = 0;
    lasersight_off(ent);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/morph.wav"), 1, ATTN_NORM, 0);
}






