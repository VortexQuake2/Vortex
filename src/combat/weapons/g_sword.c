#include "g_local.h"

extern qboolean is_quad;

#define SWORD_DEATHMATCH_DAMAGE sabre_damage_initial->value//130
#define SWORD_KICK sabre_kick->value//400
#define SWORD_RANGE sabre_range->value//60
extern byte		is_silenced;//K03

/*
=============
fire_sword

attacks with the beloved sword of the highlander
edict_t *self - entity producing it, yourself
vec3_t start - The place you are
vec3_t aimdir - Where you are looking at in this case
int damage - the damage the sword inflicts
int kick - how much you want that bitch to be thrown back
=============
*/

// az: putting this in this order in particular doesn't make much sense to me, 
// but it's kept like that so we can compare with the newvrx sword.
void Laser_PreThink(edict_t *self)
{	
	VectorCopy(self->moveinfo.start_origin, self->s.origin);
	VectorCopy(self->moveinfo.end_origin, self->s.old_origin);
	//VectorCreateVelocity(self->s.old_origin, self->s.origin, self->velocity);
	VectorSubtract(self->s.old_origin, self->s.origin, self->velocity);
	VectorScale(self->velocity, 10, self->velocity);

	//if the radius isn't the final length then adjust it
	if (self->s.frame != self->health)
	{
		int difference = self->health - self->max_health;
		float total_time = self->delay - self->wait;
		float time_ellapsed = level.time - self->wait;
		self->s.frame = self->max_health + (int)((difference / total_time) * time_ellapsed);
	}

	if (level.time >= self->delay) 
	{
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

/*
colors
The way laser colors work is based on color palette indexes found in pak0 pics/colormap.pcx
Each byte of the long represents 1 stage of the laser and each of the 4 bytes get cycled through in order

0xf2f2f0f0 - red
0xd0d1d2d3 - green
0xf3f3f1f1 - blue
0xdcdddedf - yellow
0xe0e1e2e3 - orange
0x80818283 - dark purple
0x70717273 - light blue
0x90919293 - different green
0xb0b1b2b3 - purple
0x40414243 - different red
0xe2e5e3e6 - orange
0xd0f1d3f3 - blue and green
0xf2f3f0f1 - inner = red, outer = blue
0xf3f2f1f0 - inner = blue, outer = red
0xdad0dcd2 - inner = green, outer = yellow
0xd0dad2dc - inner = yellow, outer = green 
*/
edict_t *CreateLaser(edict_t *owner, vec3_t start, vec3_t end, int effects, int color, int diameter, int final_diameter, float time)
{
	edict_t *laser = G_Spawn();
	laser->classname = "laser"; 
	laser->owner = owner;
	laser->think = Laser_PreThink;
	laser->nextthink = level.time + FRAMETIME;
	laser->s.frame = diameter;
	laser->s.skinnum = color;
	laser->s.renderfx = RF_BEAM | RF_TRANSLUCENT;
	laser->s.effects = effects;
	laser->movetype = MOVETYPE_NOCLIP;
	laser->clipmask = MASK_ALL;
	laser->model = NULL;
	laser->s.modelindex = 1;
	laser->max_health = diameter; //store original diameter here 
	laser->health = final_diameter; //store final diameter here
	laser->wait = level.time; //store original time created here for calculating final size
	laser->delay = level.time + time;

	//store the start and end points between frames
	VectorCopy(start, laser->moveinfo.start_origin);
	VectorCopy(end, laser->moveinfo.end_origin);

	//kick off the lasers movement
	laser->think(laser);

	return laser;
}

// az: imported from newvrx, this is here only for Reference
void fire_sword_old ( edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{    
    trace_t tr; //detect whats in front of you up to range "vec3_t end"

    vec3_t end;
    vec3_t begin;
    vec3_t begin_offset;
    float sword_bonus = 1;

    // calling entity made a sound, used to alert monsters
    self->lastsound = level.framenum;

    float swordrange = SABRE_INITIAL_RANGE * sword_bonus +
                 (SABRE_ADDON_RANGE * self->myskills.weapons[WEAPON_SWORD].mods[2].current_level);

    VectorSet(begin_offset, 0, 0, self->viewheight - 8);
    VectorAdd(self->s.origin, begin_offset, begin);

    // Figure out what we hit, if anything:

    VectorMA(start, swordrange, aimdir, end);  //calculates the range vector

    tr = gi.trace (begin, NULL, NULL, end, self, MASK_SHOT);
                        // figuers out what in front of the player up till "end"

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_LASER);
	gi.WritePosition (begin);
	gi.WritePosition (tr.endpos);
	gi.multicast (begin, MULTICAST_PHS);
	
	  
	// Figure out what to do about what we hit, if anything
    if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))    
    {
        if (tr.fraction < 1.0)        
        {            
            if (tr.ent->takedamage)            
            {
				if (self->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1)
					gi.sound (self, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav") , 1, ATTN_NORM, 0); 

				if (T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_SWORD))
				{
					if (self->myskills.weapons[WEAPON_SWORD].mods[3].current_level >= 1)
						burn_person(tr.ent, self, (int)(SABRE_ADDON_HEATDAMAGE * self->myskills.weapons[WEAPON_SWORD].mods[3].current_level * sword_bonus));
				}

            }        
            //else gi.multicast(begin,MULTICAST_PHS);
        }

		if (self->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1)
		{
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (self-g_edicts);
			gi.WriteByte (MZ_IONRIPPER|MZ_SILENCED);
			gi.multicast (self->s.origin, MULTICAST_PVS);
		}
    }
}

void fire_sword (edict_t *self, vec3_t start, vec3_t dir, int damage, int length, int color)
{
	edict_t	*ignore;
	trace_t	tr, tr2;
	vec3_t from, end;
	vec3_t mins, maxs;
	qboolean water;
	int mask;
	int kick = 100;
	int count = 0;
	int swordrange;


	VectorSet(mins, -2, -2, -2);
	VectorSet(maxs, 2, 2, 2);
	
	// calling entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	swordrange = SABRE_INITIAL_RANGE + (SABRE_ADDON_RANGE * self->myskills.weapons[WEAPON_SWORD].mods[2].current_level);

	//decino: special for decoys
	if (self->mtype == M_DECOY)
    swordrange = SABRE_INITIAL_RANGE + (SABRE_ADDON_RANGE * self->activator->myskills.weapons[WEAPON_SWORD].mods[2].current_level);

	//create starting and ending positions for trace
	VectorCopy(start, from);
	VectorMA(start, swordrange, dir, end);

//	gi.dprintf("Sword range: %d\n", swordrange);

	//taken from railgun fire code
	ignore = self;
	water = false;
	mask = MASK_SHOT;
	while (ignore)
	{
		tr = gi.trace (from, mins, maxs, end, ignore, mask);

		//ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client) ||
			(tr.ent->solid == SOLID_BBOX))
			ignore = NULL;//decino: no pierce
		else
			ignore = NULL;

		if ((tr.ent != self) && (tr.ent->takedamage))
		{
				if (self->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1)
					gi.sound (self, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav") , 1, ATTN_NORM, 0); 

				if (T_Damage (tr.ent, self, self, dir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_SWORD))
				{
					if (self->myskills.weapons[WEAPON_SWORD].mods[3].current_level >= 1)
						burn_person(tr.ent, self, (int)(SABRE_ADDON_HEATDAMAGE * self->myskills.weapons[WEAPON_SWORD].mods[3].current_level));
				}
		}

		VectorCopy (tr.endpos, from);

		if (++count >= MAX_EDICTS)
			break;
	}

	//trace forward and back to find water
	tr2 = gi.trace(tr.endpos, NULL, NULL, start, tr.ent, MASK_WATER);

	if (tr2.contents & MASK_WATER)
		water = true;
	else //water not found going backwards so try forwards
	{
		tr2 = gi.trace(start, NULL, NULL, tr.endpos, self, MASK_WATER);

		if (tr2.contents & MASK_WATER)
			water = true;
	}

	CreateLaser(self, start, tr.endpos, 0, color, 4, 4, 0.1);

	if (water) //create a laser in the reverse direction so it shows up in and out of water
		CreateLaser(self, tr.endpos, start, 0, color, 4, 4, 0.1);
}

// from newvrx. kept here for easier diffing
void lance_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t		origin;
	// int			n;

	if (!G_EntExists(ent->owner))
	{
		G_FreeEdict(ent);
		return;
	}

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	if (other->takedamage){
		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_SWORD);

		if (ent->owner->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1)
			gi.sound (other, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav") , 1, ATTN_NORM, 0);
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_EXPLOSION);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	G_FreeEdict (ent);
}

void lance_think (edict_t *self)
{
	trace_t tr;
	vec3_t forward, end;

	if (!G_EntIsAlive(self->owner) || level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	vectoangles(self->velocity, self->s.angles);
	ValidateAngles(self->s.angles);
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, self->dmg_radius, forward, end);
	tr = gi.trace (self->s.origin, NULL, NULL, end, self->owner, MASK_SHOT);

	/*gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_LASER);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PHS);*/

	// if we hit something, damage it and burn if it's upgraded
	if (tr.ent && tr.ent->takedamage && T_Damage(tr.ent, self, self->owner, forward, tr.endpos, tr.plane.normal, self->dmg, self->dmg, DAMAGE_ENERGY, MOD_SWORD)
		&& self->owner->myskills.weapons[WEAPON_SWORD].mods[3].current_level >= 1)
	{
		gi.sound (self, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav") , 1, ATTN_NORM, 0); 
		burn_person(tr.ent, self->owner, self->radius_dmg);
	}

	// if we hit anything at all, remove ent
	if (tr.fraction < 1)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_EXPLOSION);
		gi.WritePosition (tr.endpos);
		gi.multicast (tr.endpos, MULTICAST_PVS);

		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void fire_lance (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int burn_damage, float speed, float length)
{
	edict_t	*lance;


	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;

	// spawn grenade entity
	lance = G_Spawn();
	VectorCopy (start, lance->s.origin);
	lance->movetype = MOVETYPE_TOSS;
	lance->s.modelindex = 1;
    // az: we got a model!! thanks moesh
	// lance->svflags |= SVF_NOCLIENT;
	lance->owner = self;
	lance->think = lance_think;
	lance->dmg_radius = length;
	lance->dmg = damage;
	lance->radius_dmg = burn_damage;
	lance->classname = "lance";
	lance->delay = level.time + 10.0;
    gi.setmodel(lance, "models/objects/javelin/tris.md2");
	// lance->s.skinnum = 1;

	gi.linkentity (lance);
	lance->nextthink = level.time + FRAMETIME;
	vectoangles(aimdir, lance->s.angles);

	// adjust velocity
	VectorScale (aimdir, speed, lance->velocity);
	lance->velocity[2] += 300;
}

void sword_attack (edict_t *ent, vec3_t g_offset, int damage)
{
	vec3_t  forward, right;
	vec3_t  start;
	vec3_t  offset;

	int swordrange = SABRE_INITIAL_RANGE + SABRE_ADDON_RANGE * ent->myskills.weapons[WEAPON_SWORD].mods[2].current_level;

	if (is_quad)
		damage *= 4;

	AngleVectors (ent->client->v_angle, forward, right, NULL); 
	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	// az: newvrx lance code. unused for now
	/*
    if (ent->client->weapon_mode && ent->swordtimer <= level.time)
	{
		swordrange *= 10;
		damage *= 1.5;
		fire_lance (ent, start, forward, damage, swordrange);
		ent->swordtimer = level.time + 1.4;
		return;
	}
	*/

	if (ent->swordtimer >= level.time)
		return;

    fire_sword (
		ent, 
		start, 
		forward, 
		damage, 
		SABRE_INITIAL_KICK + SABRE_ADDON_KICK * ent->myskills.weapons[WEAPON_SWORD].mods[0].current_level,
		0xd3d3d3d3 /* scolor */
	); 
}

void Weapon_Sword_Fire (edict_t *ent) {
    float sword_bonus = 1;
    int damage;
    float temp;

    // special rules; flag carrier can't use weapons
    if (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
        return;

    if (ent->myskills.class_num == CLASS_KNIGHT)
        sword_bonus = 1.5;
    damage = SABRE_INITIAL_DAMAGE +
             (SABRE_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_SWORD].mods[0].current_level * sword_bonus);

    // sword forging reduces the per-frame damage penalty
    temp = 0.8 + 0.007 * ent->myskills.weapons[WEAPON_SWORD].mods[1].current_level;

    if ((temp < 1.0) && (ent->client->ps.gunframe - 5 > 0))
        damage *= pow(temp, ent->client->ps.gunframe - 5);

    //gi.dprintf("damage=%d\n", damage);

	 if ((ent->client->ps.gunframe == 5) && (ent->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1))
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("misc/power1.wav") , 1, ATTN_NORM, 0);

     if ( ent->client->buttons & BUTTON_ATTACK )
		sword_attack (ent, vec3_origin, damage);
     ent->client->ps.gunframe++;
}

void Weapon_Lance_Fire (edict_t *ent) {
    int sword_bonus = 1.0;
    int damage, burn_damage;
    float speed;
    vec3_t forward, start, right, offset;

    // special rules; flag carrier can't use weapons
    if (ctf->value && ctf_enable_balanced_fc->value && vrx_has_flag(ent))
        return;

	// calculate knight bonus
    if (ent->myskills.class_num == CLASS_KNIGHT)
        sword_bonus = 1.5;

    damage = SABRE_INITIAL_DAMAGE +
             (SABRE_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_SWORD].mods[0].current_level * sword_bonus);
    burn_damage = SABRE_ADDON_HEATDAMAGE * ent->myskills.weapons[WEAPON_SWORD].mods[3].current_level * sword_bonus;
    speed = 850 + (15 * ent->myskills.weapons[WEAPON_SWORD].mods[2].current_level * sword_bonus);

    // lance modifier
    damage *= 2;

    if (ent->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1) 
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/lancethrow.wav"), 1, ATTN_NORM, 0);

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	// apply view kick
	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// fire lance
	fire_lance(ent, start, forward, damage, burn_damage, speed, 64);

	// show muzzle flare
	if (ent->myskills.weapons[WEAPON_SWORD].mods[4].current_level < 1)
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent-g_edicts);
		gi.WriteByte(MZ_IONRIPPER|MZ_SILENCED);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	// advance weapon frame
    ent->client->ps.gunframe++;
}

void Weapon_Sword (edict_t *ent)
{
	static int      pause_frames[] = {19, 32, 0};
	static int      sword_frames[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 0};
	static int		lance_frames[] = {5, 0};
	
	if (ent->client->weapon_mode)
		Weapon_Generic (ent, 4, 20, 52, 55, pause_frames, lance_frames, Weapon_Lance_Fire);
	else
		Weapon_Generic (ent, 4, 20, 52, 55, pause_frames, sword_frames, Weapon_Sword_Fire);
}