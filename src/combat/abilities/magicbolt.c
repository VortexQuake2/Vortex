#include "g_local.h"

void MagicBoltExplode(edict_t *self, edict_t *other)
{
	//Do the damage
	T_RadiusDamage(self, self->owner, self->radius_dmg,other, self->dmg_radius, MOD_MAGICBOLT);
}

void magicbolt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//Talent: Improved Magic Bolt
    //int talentLevel = vrx_get_talent_level(self->owner, TALENT_IMP_MAGICBOLT);

	if (G_EntExists(other))
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, /*self->dmg*/ 0, 0, MOD_MAGICBOLT);

		// scoring a hit refunds power cubes
		//if (talentLevel > 0)
		//	self->owner->client->pers.inventory[power_cube_index] += self->monsterinfo.cost * (0.4 * talentLevel);
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

    gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/boom2.wav"), 1, ATTN_NORM, 0);

	if(self->dmg_radius > 0)
		MagicBoltExplode(self, other);

	G_FreeEdict(self);
}

void fire_magicbolt (edict_t *ent, int damage, int radius_damage, float damage_radius, float cost_mult)
{
	edict_t	*bolt;
	vec3_t	offset, forward, right, start;
    
	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// set-up firing parameters
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorScale (forward, -3, ent->client->kick_origin);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	// create bolt
	bolt = G_Spawn();
	VectorSet (bolt->mins, -10, -10, 10);
	VectorSet (bolt->maxs, 10, 10, 10);
	bolt->s.modelindex = gi.modelindex ("models/objects/flball/tris.md2");
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (forward, bolt->s.angles);
	VectorScale (forward, BOLT_SPEED, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_COLOR_SHELL;
	bolt->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = ent;
	bolt->think = G_FreeEdict;
	bolt->touch = magicbolt_touch;
	bolt->nextthink = level.time + BOLT_DURATION;
	bolt->monsterinfo.cost = BOLT_COST * cost_mult;
	bolt->dmg = damage;
	bolt->dmg_radius = damage_radius;
	bolt->radius_dmg = radius_damage;
	bolt->classname = "magicbolt";	
	gi.linkentity (bolt);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/holybolt2.wav"), 1, ATTN_NORM, 0);
}

void Cmd_Magicbolt_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int damage, radius_damage=0, cost=BOLT_COST*cost_mult;
	float radius=0;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[MAGICBOLT].current_level, cost))
		return;
	if (ent->myskills.abilities[MAGICBOLT].disable)
		return;

	damage = (BOLT_INITIAL_DAMAGE+BOLT_ADDON_DAMAGE*ent->myskills.abilities[MAGICBOLT].current_level)*skill_mult;

	ent->client->ability_delay = level.time + BOLT_DELAY; // vrc 2.32 remove  magicbolt delay to spam!

	fire_magicbolt(ent, damage, 0, 0, cost_mult);

	ent->client->pers.inventory[power_cube_index] -= cost;
}
