#include "../../quake2/g_local.h"

#define HOLYGROUND_HOLY					1
#define HOLYGROUND_UNHOLY				2

void holyground_sparks (vec3_t org, int num, int radius, int color)
{
	int		i;
	vec3_t	start, end;
	trace_t	tr;

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	for (i=0; i<num; i++)
	{
		// calculate random position on 2-D plane and trace to it
		VectorCopy(org, start);
		VectorCopy(start, end);
		end[0] += crandom() * GetRandom(0, radius);
		end[1] += crandom() * GetRandom(0, radius);
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);

		// push to the floor
		VectorCopy(tr.endpos, start);
		VectorCopy(start, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		VectorCopy(tr.endpos, start);
		start[2] += GetRandom(0, 8);

		// throw sparks
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(1); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(color); // color
		gi.multicast(start, MULTICAST_PVS);
	}
}

qboolean holyground_visible (edict_t *self, edict_t *target)
{
	trace_t	tr;
	vec3_t	start, end;

	// trace from start to end at the same height
	VectorCopy(self->s.origin, start);
	VectorCopy(target->s.origin, end);
	end[2] = start[2];
	tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
	if (tr.fraction < 1)
		return false;

	// check for obstructions
	VectorCopy(end, start);
	tr = gi.trace(start, NULL, NULL, target->s.origin, NULL, MASK_SOLID);
	if (tr.fraction < 1)
		return false;

	// we didn't hit any walls, so we are good
	return true;
}

void holyground_attack (edict_t *self, float radius)
{
	edict_t *target = NULL;

	while ((target = findradius(target, self->s.origin, radius)) != NULL)
	{
		// standard entity checks
		if (!G_EntIsAlive(target))
			continue;
		if (!target->takedamage || (target->solid == SOLID_NOT))
			continue;
		if (target->client && target->client->invincible_framenum > level.framenum)
			continue;
		if (target->flags & FL_CHATPROTECT || target->flags & FL_GODMODE || target->flags & FL_NOTARGET)
			continue;
		if (target->svflags & SVF_NOCLIENT && target->mtype != M_FORCEWALL)
			continue;
		if (que_typeexists(target->curses, CURSE_FROZEN))
			continue;

		// target must be on the ground
		if (!target->groundentity)
			continue;

		if (!holyground_visible(self, target))
			continue;
		
		if (OnSameTeam(self->owner, target))
		{
			if (self->style == HOLYGROUND_HOLY)
			{
				// heal
				if (target->health < target->max_health)
				{
					int health = target->max_health - target->health;
					if (health > self->dmg)
						health = self->dmg;
					target->health += health;
				}
						
				// throw green sparks
				holyground_sparks(target->s.origin, 1, (int)(target->maxs[1]+32), 209);
			}
		}
		else if (self->style == HOLYGROUND_UNHOLY)
		{
			if (level.framenum > self->monsterinfo.nextattack)
			{
				// deal damage
				T_Damage(target, self, self->owner, vec3_origin, target->s.origin, vec3_origin, 50, 0, 0, MOD_UNHOLYGROUND);
				self->monsterinfo.nextattack = (int)(level.framenum + 0.5 / FRAMETIME);
				//gi.dprintf("%.1.f %.1f\n", self->monsterinfo.nextattack, level.time);
			}

			// throw red sparks
			holyground_sparks(target->s.origin, 1, (int)(target->maxs[1]+32), 240);
		}
	}
}

void holyground_remove (edict_t *ent, edict_t *self)
{
	// if ent isn't specified, then always remove; otherwise, make sure this entity is ours
	if (!ent || (self && self->inuse && self->mtype == M_HOLYGROUND 
		&& ent->inuse && self->owner && self->owner->inuse && self->owner == ent))
	{
		// prep entity for removal
		self->think = G_FreeEdict;
		self->nextthink = level.time + FRAMETIME;

		// clear owner's pointer to this entity
		if (self->owner && self->owner->inuse)
			self->owner->holyground = NULL;
	}
	// this isn't ours to remove, but clear the invalid pointer
	else if (ent)
		ent->holyground = NULL;
}


void holyground_think (edict_t *self)
{
	if (!G_EntIsAlive(self->owner) || level.time > self->delay)
	{
		holyground_remove(NULL, self);
		return;
	}

	holyground_sparks(self->s.origin, 1, 256, 15);
	holyground_attack(self, 256);

	self->nextthink = level.time + FRAMETIME;
}

void CreateHolyGround (edict_t *ent, int type, int skill_level)
{
	edict_t *e;

	e = G_Spawn();
	e->owner = ent;
	e->dmg = 1;
	e->mtype = M_HOLYGROUND;
	e->think = holyground_think;
	e->nextthink = level.time + FRAMETIME;
	e->classname = "holy_ground";
	e->svflags |= SVF_NOCLIENT;
	e->monsterinfo.level = skill_level;
	e->delay = level.time + HOLYGROUND_INITIAL_DURATION + HOLYGROUND_ADDON_DURATION * skill_level;
	e->style = type;

	ent->holyground = e;

	VectorCopy(ent->s.origin, e->s.origin);
	gi.linkentity(e);

	ent->client->ability_delay = level.time + HOLYGROUND_DELAY;
	ent->client->pers.inventory[power_cube_index] -= HOLYGROUND_COST;
}

void Cmd_HolyGround_f (edict_t *ent)
{
	//Talent: Holy Ground
    int talentLevel = vrx_get_talent_level(ent, TALENT_HOLY_GROUND);

	if (level.time < pregame_time->value)
		return;

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade this talent before you can use it!\n");
		return;
	}
	if (ent->holyground && ent->holyground->inuse)
	{
		holyground_remove(ent, ent->holyground);
		safe_cprintf(ent, PRINT_HIGH, "Holy ground removed.\n");
		return;
	}
	if (ent->client->ability_delay > level.time)
	{
		safe_cprintf(ent, PRINT_HIGH, "You must wait %.1f seconds before you can use this talent.\n", 
			ent->client->ability_delay-level.time);
		return;
	}
	if (ent->client->pers.inventory[power_cube_index] < HOLYGROUND_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need %d cubes before you can use this talent.\n", 
			HOLYGROUND_COST-ent->client->pers.inventory[power_cube_index]);
		return;
	}
	
	CreateHolyGround(ent, HOLYGROUND_HOLY, talentLevel);
    gi.sound(ent, CHAN_AUTO, gi.soundindex("abilities/sanctuary.wav"), 1, ATTN_NORM, 0);
}

void Cmd_UnHolyGround_f (edict_t *ent)
{
	//Talent: Unholy Ground
    int talentLevel = vrx_get_talent_level(ent, TALENT_UNHOLY_GROUND);

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade this talent before you can use it!\n");
		return;
	}
	if (ent->holyground && ent->holyground->inuse)
	{
		holyground_remove(ent, ent->holyground);
		safe_cprintf(ent, PRINT_HIGH, "Un-holy ground removed.\n");
		return;
	}
	if (ent->client->ability_delay > level.time)
	{
		safe_cprintf(ent, PRINT_HIGH, "You must wait %.1f seconds before you can use this talent.\n", 
			ent->client->ability_delay-level.time);
		return;
	}
	if (ent->client->pers.inventory[power_cube_index] < HOLYGROUND_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need %d cubes before you can use this talent.\n", 
			HOLYGROUND_COST-ent->client->pers.inventory[power_cube_index]);
		return;
	}

	CreateHolyGround(ent, HOLYGROUND_UNHOLY, talentLevel);
    gi.sound(ent, CHAN_AUTO, gi.soundindex("abilities/sanctuary.wav"), 1, ATTN_NORM, 0);
}
