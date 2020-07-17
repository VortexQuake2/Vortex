#include "g_local.h"

void wormhole_in (edict_t *self, edict_t *other)
{
	
}

void wormhole_out (edict_t *self, edict_t *other)
{

}

void wormhole_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (!other || !other->inuse)
		return;

	// point to monster's pilot
	if (PM_MonsterHasPilot(other))
		other = other->activator;

	if ((other == self->creator) || IsAlly(self->creator, other)) {
        float time;

        // can't enter wormhole with the flag
        if (vrx_has_flag(other))
            return;

        // can't enter wormhole while being hurt
        if (other->lasthurt + DAMAGE_ESCAPE_DELAY > level.time)
            return;

        // can't enter wormhole while cursed (this includes healing, bless)
        if (que_typeexists(other->curses, -1))
            return;

		// can't stay in wormhole long if we're warring
		if (SPREE_WAR == true && SPREE_DUDE == other)
			time = 10.0;
		else
			time = BLACKHOLE_EXIT_TIME;

		// reset railgun sniper frames
		other->client->refire_frames = 0;

        vrx_remove_player_summonables(other);
		V_RestoreMorphed(other, 50); // un-morph

		other->flags |= FL_WORMHOLE;
		other->movetype = MOVETYPE_NOCLIP;
		other->svflags |= SVF_NOCLIENT;
		other->client->wormhole_time = level.time + BLACKHOLE_EXIT_TIME; // must exit wormhole by this time

		self->nextthink = level.time + FRAMETIME; // close immediately
	}
}

void wormhole_close (edict_t *self)
{
	if (self->s.frame > 0)
	{
		self->s.frame--;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	G_FreeEdict(self);
}

void wormhole_think (edict_t *self)
{

}

void wormhole_out_ready (edict_t *self)
{
	vec3_t	mins, maxs;
	trace_t tr;

	self->nextthink = level.time + FRAMETIME;

	// wormhole begins to open up
	if (self->s.frame == 0)
	{
        gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/portalcast.wav"), 1, ATTN_NORM, 0);

		// delay before we can exit the wormhole
		self->monsterinfo.lefty = (int)(level.framenum + 1.0 / FRAMETIME);
	}

	// wormhole not fully opened
	if (level.framenum < self->monsterinfo.lefty)
	{
		// hold creator in-place until wormhole is fully opened
		self->creator->holdtime = level.time + FRAMETIME;

		if (self->s.frame < 4)
			self->s.frame++;
		return;
	}

	// hold creator in-place
	self->creator->holdtime = level.time + 0.5;

	// don't get stuck in a solid
	VectorSet (mins, -16, -16, -24);
	VectorSet (maxs, 16, 16, 32);
	tr = gi.trace(self->s.origin, mins, maxs, self->s.origin, self->creator, MASK_SHOT);
	if (tr.fraction < 1 || gi.pointcontents(self->s.origin) & CONTENTS_SOLID 
		|| !ValidTeleportSpot(self, self->s.origin))
	{
		self->think = wormhole_close;
		return;
	}

    gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/portalenter.wav"), 1, ATTN_NORM, 0);

	//  entity made a sound, used to alert monsters
	self->creator->lastsound = level.framenum;

	// spawn player at wormhole location
	VectorCopy(self->s.origin, self->creator->s.origin);
	gi.linkentity(self->creator);
	VectorClear(self->creator->velocity);
	self->creator->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->creator->client->ps.pmove.pm_time = 14;

	self->creator->movetype = MOVETYPE_WALK;
	self->creator->flags &= ~FL_WORMHOLE;
	self->creator->svflags &= ~SVF_NOCLIENT;

	self->think = wormhole_close;

	self->creator->client->ability_delay = level.time + (ctf->value?2.0:1.0);
	self->creator->client->pers.inventory[power_cube_index] -= BLACKHOLE_COST;
}

void wormhole_in_ready (edict_t *self)
{
	if (self->s.frame < 4)
	{
		if (self->s.frame == 0)
            gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/portalcast.wav"), 1, ATTN_NORM, 0);

		self->s.frame++;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	// make wormhole semi-solid, ready for entry
	VectorSet(self->mins, -16, -16, -16);
	VectorSet(self->maxs, 16, 16, 16);
	self->solid = SOLID_TRIGGER;
	self->touch = wormhole_touch;
	self->think = wormhole_close;
	self->nextthink = level.time + 1.0;
	gi.linkentity(self);
}

void SpawnWormhole (edict_t *ent, int type)
{
	edict_t	*hole;
	vec3_t	offset, forward, right, start;
    
//	if (!ent->myskills.administrator)
//		return;

	if (ent->flags & FL_WORMHOLE)
	{
		type = 0;
		ent->holdtime = level.time + 1.0;
	}

	// set-up firing parameters
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorScale (forward, -3, ent->client->kick_origin);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	// create wormhole
	hole = G_Spawn();
	hole->s.modelindex = gi.modelindex ("models/mad/hole/tris.md2");
	VectorCopy (start, hole->s.origin);
	VectorCopy (start, hole->s.old_origin);
	hole->movetype = MOVETYPE_NONE;
	//hole->s.sound = gi.soundindex ("misc/lasfly.wav");
	hole->creator = ent;

	if (type)	// wormhole is an entrance
		hole->think = wormhole_in_ready;
	else		// wormhole is an exit
		hole->think = wormhole_out_ready;

	hole->nextthink = level.time + 0.6;
	hole->classname = "wormhole";
	gi.linkentity (hole);

	// write a nice effect so everyone knows we've cast a spell
	if (type)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_TELEPORT_EFFECT);
		gi.WritePosition (ent->s.origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		//  entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}

    //gi.sound (ent, CHAN_WEAPON, gi.soundindex("abilities/portalcast.wav"), 1, ATTN_NORM, 0);
//	ent->client->pers.inventory[power_cube_index] -= BLACKHOLE_COST;
//	ent->client->ability_delay = level.time + BLACKHOLE_DELAY;
}

void Cmd_WormHole_f (edict_t *ent)
{
	// allow wormhole exit
	if (ent->flags & FL_WORMHOLE)
		SpawnWormhole(ent, 0);

	if (!V_CanUseAbilities(ent, BLACKHOLE, BLACKHOLE_COST, true))
		return;
	if (ent->myskills.abilities[BLACKHOLE].disable)
		return;

	SpawnWormhole(ent, 1);
}
