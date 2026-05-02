#include "g_local.h"


void InitTrigger (edict_t *self)
{
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel (self, self->model);
	self->svflags = SVF_NOCLIENT;
}


// the wait time has passed, so set back up for another activation
void multi_wait (edict_t *ent)
{
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger (edict_t *ent)
{
	if (ent->nextthink)
		return;		// already been triggered

	G_UseTargets (ent, ent->activator);

	if (ent->wait > 0)	
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + ent->wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEdict;
	}
}

void Use_Multi (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->activator = activator;
	multi_trigger (ent);
}

void Touch_Multi (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if(other->client || PM_MonsterHasPilot(other))
	{
		if (self->spawnflags & 2)
			return;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!(self->spawnflags & 1))
			return;
	}
	else
		return;

	if (self->teamnum)
	{
		if (other->teamnum != self->teamnum)
			return;
	}

	if (!VectorCompare(self->movedir, vec3_origin))
	{
		vec3_t	forward;

		AngleVectors(other->s.angles, forward, NULL, NULL);
		if (_DotProduct(forward, self->movedir) < 0)
			return;
	}

	self->activator = other;
	multi_trigger (self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
void trigger_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	gi.linkentity (self);
}

void SP_trigger_multiple (edict_t *ent)
{
	if (ent->sounds == 1)
		ent->noise_index = gi.soundindex ("misc/secret.wav");
	else if (ent->sounds == 2)
		ent->noise_index = gi.soundindex ("misc/talk.wav");
	else if (ent->sounds == 3)
		ent->noise_index = gi.soundindex ("misc/trigger1.wav");
	
	if (!ent->wait)
		ent->wait = 0.2;
	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->svflags |= SVF_NOCLIENT;


	if (ent->spawnflags & 4)
	{
		ent->solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	if (!VectorCompare(ent->s.angles, vec3_origin))
		G_SetMovedir (ent->s.angles, ent->movedir);

	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}


/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.

sounds
 1)	secret
 2)	beep beep
 3)	large switch
 4)

"message"	string to be displayed when triggered
*/

void SP_trigger_once(edict_t *ent)
{
	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (ent->spawnflags & 1)
	{
		vec3_t	v;

		VectorMA (ent->mins, 0.5, ent->size, v);
		ent->spawnflags &= ~1;
		ent->spawnflags |= 4;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", ent->classname, vtos(v));
	}

	ent->wait = -1;
	SP_trigger_multiple (ent);
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
void trigger_relay_use (edict_t *self, edict_t *other, edict_t *activator)
{
	G_UseTargets (self, activator);
}

void SP_trigger_relay (edict_t *self)
{
	self->use = trigger_relay_use;
}


/*
==============================================================================

trigger_key

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires it's targets if player has the proper key.
Use "item" to specify the required key, for example "key_data_cd"
*/
void trigger_key_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int			index;

	if (!self->item)
		return;
	if (!activator->client)
		return;

	index = ITEM_INDEX(self->item);
	if (!activator->client->pers.inventory[index])
	{
		if (level.time < self->touch_debounce_time)
			return;
		self->touch_debounce_time = level.time + 5.0;
		if(!(activator->svflags & SVF_MONSTER))
				safe_centerprintf (activator, "You need the %s", self->item->pickup_name);
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keytry.wav"), 1, ATTN_NORM, 0);
		return;
	}

	gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keyuse.wav"), 1, ATTN_NORM, 0);
	if (coop->value)
	{
		int		player;
		edict_t	*ent;

		if (strcmp(self->item->classname, "key_power_cube") == 0)
		{
			int	cube;

			for (cube = 0; cube < 8; cube++)
				if (activator->client->pers.power_cubes & (1 << cube))
					break;
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				if (ent->client->pers.power_cubes & (1 << cube))
				{
					ent->client->pers.inventory[index]--;
					ent->client->pers.power_cubes &= ~(1 << cube);
				}
			}
		}
		else
		{
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				ent->client->pers.inventory[index] = 0;
			}
		}
	}
	else
	{
		activator->client->pers.inventory[index]--;
	}

	G_UseTargets (self, activator);

	self->use = NULL;
}

void SP_trigger_key (edict_t *self)
{
	if (!st.item)
	{
		gi.dprintf("no key item for trigger_key at %s\n", vtos(self->s.origin));
		return;
	}
	self->item = FindItemByClassname (st.item);

	if (!self->item)
	{
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item, vtos(self->s.origin));
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s at %s has no target\n", self->classname, vtos(self->s.origin));
		return;
	}

	gi.soundindex ("misc/keytry.wav");
	gi.soundindex ("misc/keyuse.wav");

	self->use = trigger_key_use;
}


/*
==============================================================================

trigger_counter

==============================================================================
*/

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/

void trigger_counter_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->count == 0)
		return;
	
	self->count--;

	if (self->count)
	{
		if (! (self->spawnflags & 1) && !(self->svflags & SVF_MONSTER))
		{
			safe_centerprintf(activator, "%i more to go...", self->count);
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	
	if (! (self->spawnflags & 1) && !(self->svflags & SVF_MONSTER))
	{
		safe_centerprintf(activator, "Sequence completed!");
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
	self->activator = activator;
	multi_trigger (self);
}

void SP_trigger_counter (edict_t *self)
{
	self->wait = -1;
	if (!self->count)
		self->count = 2;

	self->use = trigger_counter_use;
}


/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (edict_t *ent)
{
	// we must have some delay to make sure our use targets are present
	if (ent->delay < 0.2)
		ent->delay = 0.2;
	G_UseTargets(ent, ent);
}


/*
==============================================================================

trigger_push

==============================================================================
*/

#if 0
#define PUSH_ONCE		1

static int windsound;

void trigger_push_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (strcmp(other->classname, "grenade") == 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);
	}
	else if (other->health > 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);

		if (other->client)
		{
			// don't take falling damage immediately from this
			VectorCopy (other->velocity, other->client->oldvelocity);
			if (other->fly_sound_debounce_time < level.time)
			{
				other->fly_sound_debounce_time = level.time + 1.5;
				gi.sound (other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}
	if (self->spawnflags & PUSH_ONCE)
		G_FreeEdict (self);
}

void SP_trigger_push (edict_t *self)
{
	InitTrigger (self);
	windsound = gi.soundindex ("misc/windfly.wav");
	self->touch = trigger_push_touch;
	if (!self->speed)
		self->speed = 1000;
	gi.linkentity (self);
}
#endif

// RAFAEL
#define PUSH_ONCE  1

static int windsound;

void trigger_push_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (strcmp(other->classname, "grenade") == 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);
	}
	else if (other->health > 0)
	{	
		VectorScale (self->movedir, self->speed * 10, other->velocity);

		if (other->client)
		{
			// don't take falling damage immediately from this
			VectorCopy (other->velocity, other->client->oldvelocity);
			if (other->fly_sound_debounce_time < level.time)
			{
				other->fly_sound_debounce_time = level.time + 1.5;
				gi.sound (other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}
	if (self->spawnflags & PUSH_ONCE)
	G_FreeEdict (self);
}


/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
Pushes the player
"speed"		defaults to 1000
*/
void trigger_push_active (edict_t *self);

void trigger_effect (edict_t *self)
{
	vec3_t	origin;
	vec3_t	size;
	int		i;
	
	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	
	for (i=0; i<10; i++)
	{
		origin[2] += (self->speed * 0.01) * (i + random());
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_TUNNEL_SPARKS);
		gi.WriteByte (1);
		gi.WritePosition (origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte (0x74 + (randomMT()&7));
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

}

void trigger_push_inactive (edict_t *self)
{
	if (self->delay > level.time)
	{
		self->nextthink = level.time + 0.1;
	}
	else
	{
		self->touch = trigger_push_touch;
		self->think = trigger_push_active;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;  
	}
}

void trigger_push_active (edict_t *self)
{
	if (self->delay > level.time)
	{
		self->nextthink = level.time + 0.1;
		trigger_effect (self);
	}
	else
	{
		self->touch = NULL;
		self->think = trigger_push_inactive;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;  
	}
}

void SP_trigger_push (edict_t *self)
{
	InitTrigger (self);
	windsound = gi.soundindex ("misc/windfly.wav");
	self->touch = trigger_push_touch;
	
	if (self->spawnflags & 2)
	{
		if (!self->wait)
			self->wait = 10;
  
		self->think = trigger_push_active;
		self->nextthink = level.time + 0.1;
		self->delay = self->nextthink + self->wait;
	}

	if (!self->speed)
		self->speed = 1000;
	
	gi.linkentity (self);

}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.

It does dmg points of damage each server frame

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)

*/
void hurt_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity (self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void V_RespawnItems(edict_t* ent);//GHz
void hurt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		dflags;

	V_RespawnItems(other);

	if (!other->takedamage)
		return;

	if (self->teamnum)
	{
		if (other->teamnum != self->teamnum)
			return;
	}

	if (self->timestamp > level.time)
		return;

	if (self->spawnflags & 16)
		self->timestamp = level.time + 1;
	else
		self->timestamp = level.time + FRAMETIME;

	if (!(self->spawnflags & 4))
	{
		if ((level.framenum % (int)(1 / FRAMETIME)) == 0)
			gi.sound (other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
	}

	if (self->spawnflags & 8)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, self->dmg, dflags, MOD_TRIGGER_HURT);
}

void SP_trigger_hurt (edict_t *self)
{
	InitTrigger (self);

	self->noise_index = gi.soundindex ("world/electro.wav");
	self->touch = hurt_touch;

	if (!self->dmg)
		self->dmg = 5;

	if (self->spawnflags & 1)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & 2)
		self->use = hurt_use;

	gi.linkentity (self);
}


/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entites gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/

void trigger_gravity_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	other->gravity = self->gravity;
}

void SP_trigger_gravity (edict_t *self)
{
	if (st.gravity == 0 )
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n", vtos(self->s.origin));
		G_FreeEdict  (self);
		return;
	}

	InitTrigger (self);
	self->gravity = atof(st.gravity);
	self->touch = trigger_gravity_touch;
}


/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/

void trigger_monsterjump_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client) return;

	if (other->flags & (FL_FLY | FL_SWIM) )
		return;
	if (other->svflags & SVF_DEADMONSTER)
		return;
	if ( !(other->svflags & SVF_MONSTER))
		return;

// set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;
	
	if (!other->groundentity)
		return;
	
	other->groundentity = NULL;
	other->velocity[2] = self->movedir[2];
}

void SP_trigger_monsterjump (edict_t *self)
{
	if (!self->speed)
		self->speed = 200;
	if (!st.height)
		st.height = 200;
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;
	InitTrigger (self);
	self->touch = trigger_monsterjump_touch;
	self->movedir[2] = st.height;
}

#ifdef VRX_REPRO

/*
==============================================================================

trigger_fog

==============================================================================
*/

/*QUAKED trigger_fog (.5 .5 .5) ? AFFECT_FOG AFFECT_HEIGHTFOG INSTANTANEOUS FORCE BLEND
Players moving against this trigger will have their fog settings changed.
Fog/heightfog will be adjusted if the spawnflags are set. Instantaneous
ignores any delays. Force causes it to ignore movement dir and always use
the "on" values. Blend causes it to change towards how far you are into the trigger
with respect to angles.
"target" can target an info_notnull to pull the keys below from.
"delay" default to 0.5; time in seconds a change in fog will occur over
"wait" default to 0.0; time in seconds before a re-trigger can be executed

"fog_density"; density value of fog, 0-1
"fog_color"; color value of fog, 3d vector with values between 0-1 (r g b)
"fog_density_off"; transition density value of fog, 0-1
"fog_color_off"; transition color value of fog, 3d vector with values between 0-1 (r g b)
"fog_sky_factor"; sky factor value of fog, 0-1
"fog_sky_factor_off"; transition sky factor value of fog, 0-1

"heightfog_falloff"; falloff value of heightfog, 0-1
"heightfog_density"; density value of heightfog, 0-1
"heightfog_start_color"; the start color for the fog (r g b, 0-1)
"heightfog_start_dist"; the start distance for the fog (units)
"heightfog_end_color"; the start color for the fog (r g b, 0-1)
"heightfog_end_dist"; the end distance for the fog (units)

"heightfog_falloff_off"; transition falloff value of heightfog, 0-1
"heightfog_density_off"; transition density value of heightfog, 0-1
"heightfog_start_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_start_dist_off"; transition the start distance for the fog (units)
"heightfog_end_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_end_dist_off"; transition the end distance for the fog (units)
*/

#define SPAWNFLAG_FOG_AFFECT_FOG        1
#define SPAWNFLAG_FOG_AFFECT_HEIGHTFOG  2
#define SPAWNFLAG_FOG_INSTANTANEOUS     4
#define SPAWNFLAG_FOG_FORCE             8
#define SPAWNFLAG_FOG_BLEND             16

static float fog_lerp(float from, float to, float fraction)
{
	return from + (to - from) * fraction;
}

static float fog_clamp01(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static void trigger_fog_set_global(edict_t *player, const edict_t *fog_value_storage, qboolean use_on)
{
	player->client->pers.wanted_fog[0] = use_on ? fog_value_storage->fog.density : fog_value_storage->fog.density_off;
	player->client->pers.wanted_fog[1] = use_on ? fog_value_storage->fog.color[0] : fog_value_storage->fog.color_off[0];
	player->client->pers.wanted_fog[2] = use_on ? fog_value_storage->fog.color[1] : fog_value_storage->fog.color_off[1];
	player->client->pers.wanted_fog[3] = use_on ? fog_value_storage->fog.color[2] : fog_value_storage->fog.color_off[2];
	player->client->pers.wanted_fog[4] = use_on ? fog_value_storage->fog.sky_factor : fog_value_storage->fog.sky_factor_off;
}

static void trigger_fog_set_height(edict_t *player, const edict_t *fog_value_storage, qboolean use_on)
{
	height_fog_t *wanted = &player->client->pers.wanted_heightfog;

	wanted->start[0] = use_on ? fog_value_storage->heightfog.start_color[0] : fog_value_storage->heightfog.start_color_off[0];
	wanted->start[1] = use_on ? fog_value_storage->heightfog.start_color[1] : fog_value_storage->heightfog.start_color_off[1];
	wanted->start[2] = use_on ? fog_value_storage->heightfog.start_color[2] : fog_value_storage->heightfog.start_color_off[2];
	wanted->start[3] = use_on ? fog_value_storage->heightfog.start_dist : fog_value_storage->heightfog.start_dist_off;

	wanted->end[0] = use_on ? fog_value_storage->heightfog.end_color[0] : fog_value_storage->heightfog.end_color_off[0];
	wanted->end[1] = use_on ? fog_value_storage->heightfog.end_color[1] : fog_value_storage->heightfog.end_color_off[1];
	wanted->end[2] = use_on ? fog_value_storage->heightfog.end_color[2] : fog_value_storage->heightfog.end_color_off[2];
	wanted->end[3] = use_on ? fog_value_storage->heightfog.end_dist : fog_value_storage->heightfog.end_dist_off;

	wanted->falloff = use_on ? fog_value_storage->heightfog.falloff : fog_value_storage->heightfog.falloff_off;
	wanted->density = use_on ? fog_value_storage->heightfog.density : fog_value_storage->heightfog.density_off;
}

static void trigger_fog_blend_global(edict_t *player, const edict_t *fog_value_storage, float fraction)
{
	player->client->pers.wanted_fog[0] = fog_lerp(fog_value_storage->fog.density_off, fog_value_storage->fog.density, fraction);
	player->client->pers.wanted_fog[1] = fog_lerp(fog_value_storage->fog.color_off[0], fog_value_storage->fog.color[0], fraction);
	player->client->pers.wanted_fog[2] = fog_lerp(fog_value_storage->fog.color_off[1], fog_value_storage->fog.color[1], fraction);
	player->client->pers.wanted_fog[3] = fog_lerp(fog_value_storage->fog.color_off[2], fog_value_storage->fog.color[2], fraction);
	player->client->pers.wanted_fog[4] = fog_lerp(fog_value_storage->fog.sky_factor_off, fog_value_storage->fog.sky_factor, fraction);
}

static void trigger_fog_blend_height(edict_t *player, const edict_t *fog_value_storage, float fraction)
{
	height_fog_t *wanted = &player->client->pers.wanted_heightfog;

	wanted->start[0] = fog_lerp(fog_value_storage->heightfog.start_color_off[0], fog_value_storage->heightfog.start_color[0], fraction);
	wanted->start[1] = fog_lerp(fog_value_storage->heightfog.start_color_off[1], fog_value_storage->heightfog.start_color[1], fraction);
	wanted->start[2] = fog_lerp(fog_value_storage->heightfog.start_color_off[2], fog_value_storage->heightfog.start_color[2], fraction);
	wanted->start[3] = fog_lerp(fog_value_storage->heightfog.start_dist_off, fog_value_storage->heightfog.start_dist, fraction);

	wanted->end[0] = fog_lerp(fog_value_storage->heightfog.end_color_off[0], fog_value_storage->heightfog.end_color[0], fraction);
	wanted->end[1] = fog_lerp(fog_value_storage->heightfog.end_color_off[1], fog_value_storage->heightfog.end_color[1], fraction);
	wanted->end[2] = fog_lerp(fog_value_storage->heightfog.end_color_off[2], fog_value_storage->heightfog.end_color[2], fraction);
	wanted->end[3] = fog_lerp(fog_value_storage->heightfog.end_dist_off, fog_value_storage->heightfog.end_dist, fraction);

	wanted->falloff = fog_lerp(fog_value_storage->heightfog.falloff_off, fog_value_storage->heightfog.falloff, fraction);
	wanted->density = fog_lerp(fog_value_storage->heightfog.density_off, fog_value_storage->heightfog.density, fraction);
}

void trigger_fog_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *fog_value_storage;

	if (!other->client)
		return;

	if (self->timestamp > level.time)
		return;

	self->timestamp = level.time + self->wait;

	fog_value_storage = self;

	if (self->movetarget)
		fog_value_storage = self->movetarget;

	if (self->spawnflags & SPAWNFLAG_FOG_INSTANTANEOUS)
		other->client->pers.fog_transition_time = 0.0f;
	else
		other->client->pers.fog_transition_time = fog_value_storage->delay;

	if (self->spawnflags & SPAWNFLAG_FOG_BLEND)
	{
		vec3_t center;
		vec3_t half_size;
		vec3_t start;
		vec3_t end;
		vec3_t player_dist;
		vec3_t delta;
		float dist;
		float full_dist;

		VectorScale(self->size, 0.5f, half_size);
		VectorMA(half_size, 0.5f, other->size, half_size);
		VectorMA(self->absmin, 0.5f, self->size, center);

		for (int i = 0; i < 3; i++)
		{
			start[i] = -self->movedir[i] * half_size[i];
			end[i] = self->movedir[i] * half_size[i];
			player_dist[i] = (other->s.origin[i] - center[i]) * fabsf(self->movedir[i]);
		}

		VectorSubtract(player_dist, start, delta);
		dist = VectorLength(delta);
		VectorSubtract(start, end, delta);
		full_dist = VectorLength(delta);

		if (full_dist > 0.0f)
			dist /= full_dist;
		else
			dist = 0.0f;

		dist = fog_clamp01(dist);

		if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_FOG)
			trigger_fog_blend_global(other, fog_value_storage, dist);

		if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)
			trigger_fog_blend_height(other, fog_value_storage, dist);

		return;
	}

	qboolean use_on = true;

	if (!(self->spawnflags & SPAWNFLAG_FOG_FORCE))
	{
		float len;
		vec3_t forward;

		VectorCopy(other->velocity, forward);
		len = VectorNormalize(forward);

		// Not moving enough to trip; this avoids tripping the wrong direction.
		if (len <= 0.0001f)
			return;

		use_on = DotProduct(forward, self->movedir) > 0.0f;
	}

	if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_FOG)
		trigger_fog_set_global(other, fog_value_storage, use_on);

	if (self->spawnflags & SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)
		trigger_fog_set_height(other, fog_value_storage, use_on);
}

void SP_trigger_fog(edict_t *self)
{
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;

	InitTrigger(self);

	if (!(self->spawnflags & (SPAWNFLAG_FOG_AFFECT_FOG | SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)))
		gi.dprintf("WARNING: trigger_fog at %s with no fog spawnflags set\n", vtos(self->s.origin));

	if (self->target)
	{
		self->movetarget = G_PickTarget(self->target);

		if (self->movetarget && !self->movetarget->delay)
			self->movetarget->delay = 0.5f;
	}

	if (!self->delay)
		self->delay = 0.5f;

	self->touch = trigger_fog_touch;
}

#endif

