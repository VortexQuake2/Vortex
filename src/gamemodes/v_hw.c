#include "g_local.h"
#include "v_hw.h"

// dom but without teams. vrxchile 3.0 - az

#define HW_POINTS 150
#define HW_CREDITS 100
#define HW_AWARD_FRAMES		50
#define HW_MINIMUM_PLAYERS	3
#define HW_FRAG_POINTS		45

int halo_index;

int vrx_award_exp (edict_t *attacker, edict_t *targ, edict_t *targetclient);

float hw_getdamagefactor(edict_t *targ, edict_t* attacker)
{
	if (targ && G_GetClient(targ))
	{
		if (G_GetClient(targ)->client->pers.inventory[halo_index]) // increment damage to halo bearer by every kill
			return 1 + (float)targ->myskills.streak/8.0;
	}
	// attacker has the halo, normal damage
	return 1;
}

void hw_deathcleanup(edict_t *targ, edict_t *attacker)
{
	edict_t *clienttarg, *clientattacker, *player;
	int i;

	clienttarg = G_GetClient(targ);
	clientattacker = G_GetClient(attacker);

	// we got a client target, the target is the same that got killed, and it has the halo
	if (clienttarg && clienttarg == targ && clienttarg->client->pers.inventory[halo_index])
	{
		hw_dropflag(clienttarg, &itemlist[halo_index]); // DROP IT WUB WUB

				gi.bprintf(PRINT_HIGH, "%s killed the saint!\n", 
			clientattacker ? clientattacker->client->pers.netname : "the world");

		if (clientattacker)
		{
			clientattacker->myskills.credits += 250; // prize
			vrx_apply_experience(clientattacker, HW_FRAG_POINTS * clienttarg->myskills.streak);
		}
	}

	if (clientattacker && clientattacker->client->pers.inventory[halo_index])
	{
		// oh no saint be killan (bonus)
		vrx_apply_experience(clientattacker, HW_FRAG_POINTS * pow(clientattacker->myskills.streak, 1.2)); // increase experience significantly
	}

	for (i=0; i<game.maxclients; i++) 
	{
		player = g_edicts+1+i;

		// award experience and credits to non-spectator clients
		if (!player->inuse || G_IsSpectator(player) || player == targ || player->flags & FL_CHATPROTECT)
			continue;
		
		vrx_award_exp(player, targ, targ);
	}
}

void hw_checkflag(edict_t *ent)
{
	int index;

	if (!ent || !ent->inuse)
		return;

	if (!hw->value)
		return;

	index = ITEM_INDEX(ent->item);

	if (index != halo_index)
		return;

	hw_spawnflag();
	gi.bprintf(PRINT_HIGH, "The halo respawns.");
}

edict_t *hw_flagcarrier()
{
	int i;
	edict_t *cl;

	for (i = 0; i < game.maxclients; i++)
	{
		cl = &g_edicts[i+1];

		if (cl->client && cl->client->pers.inventory[halo_index])
		{
			return cl; // got our carrier
		}
	}
	return NULL;
}

void hw_awardpoints (void)
{
	int		points, credits;
	edict_t	*carrier;

	if (!(carrier = hw_flagcarrier()))
	{
		// no flag carrier lol
		return;
	}

	if (level.framenum % qf2sf(HW_AWARD_FRAMES))
		return;

	// not enough players
	if (vrx_get_joined_players() < HW_MINIMUM_PLAYERS)
		return;

	points = HW_POINTS;
	credits = HW_CREDITS;
	carrier->myskills.credits += credits;
	vrx_apply_experience(carrier, points);
}

void hw_laserthink (edict_t *self)
{
	// must have an owner
	if (!self->owner || !self->owner->inuse)
	{
		G_FreeEdict(self);
		return;
	}
	
	if (self->owner->style)
		self->s.skinnum = 0xf2f2f0f0;
	else
		self->s.skinnum = 0xf3f3f1f1;

	// update position
	VectorCopy(self->pos2, self->s.origin);
	VectorCopy(self->pos1, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}

qboolean hw_pickupflag (edict_t *ent, edict_t *other)
{
	if (!other || !other->inuse || !other->client || G_IsSpectator(other))
		return false;

	// disable movement abilities
	if (other->client)
	{
		//jetpack
		other->client->thrusting = 0;
		//grapple hook
		other->client->hook_state = HOOK_READY;
	}
	// super speed
	other->superspeed = false;
	// antigrav
	other->antigrav = false;

    vrx_remove_player_summonables(other);

	// disable scanner
	if (other->client->pers.scanner_active & 1)
		other->client->pers.scanner_active = 0;

	// reset their velocity
	VectorClear(other->velocity);

	// alert everyone
	gi.bprintf(PRINT_HIGH, "%s got the halo!\n", other->client->pers.netname);
	
	gi.sound(world, CHAN_VOICE, gi.soundindex("hw/hw_saint.wav"), 1, ATTN_NORM, 0);
	gi.sound(other, CHAN_ITEM, gi.soundindex( va("hw/hw_pick%d.wav", GetRandom(1,5))), 1, ATTN_NORM, 0);
	other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
	return true;
}

edict_t *hw_spawnlaser (edict_t *ent, vec3_t v1, vec3_t v2)
{
	edict_t *laser;

	laser = G_Spawn();
	laser->movetype	= MOVETYPE_NONE;
	laser->solid = SOLID_NOT;
	laser->s.renderfx = RF_BEAM|RF_TRANSLUCENT;
	laser->s.modelindex = 1; // must be non-zero
	//laser->s.sound = gi.soundindex ("world/laser.wav");
	laser->classname = "laser";
	laser->s.frame = 8; // beam diameter
    laser->owner = ent;
	// set laser color
	laser->s.skinnum = 0xf2f2f0f0; // red
    laser->think = hw_laserthink;
	VectorCopy(v2, laser->s.origin);
	VectorCopy(v1, laser->s.old_origin);
	VectorCopy(v2, laser->pos2);
	VectorCopy(v1, laser->pos1);
	gi.linkentity(laser);
	laser->nextthink = level.time + FRAMETIME;
	return laser;
}


// this a literal copypaste job
void hw_flagthink (edict_t *self)
{
	vec3_t	end;
	trace_t	tr;

	//3.0 allow anyone to pick up the flag
	if (self->owner) self->owner = NULL;

	//3.0 destroy idling flags so they can respawn somewhere else
	if (self->count >= 300)	//30 seconds of idling time;
	{
		BecomeTE(self);
		gi.bprintf(PRINT_HIGH, "The halo respawns.\n");
		hw_spawnflag();
		return;
	}
	self->count++;

	if (!self->other && !VectorLength(self->velocity))
	{
		VectorCopy(self->s.origin, end);
		end[2] += 8192;
		tr = gi.trace (self->s.origin, NULL, NULL, end, self, MASK_SOLID);
		VectorCopy(tr.endpos, end);
		self->other = hw_spawnlaser(self, self->s.origin, end);
	}
	self->s.effects = 0;

	if ((self->count % 10) == 0) {
		if (self->style)
			self->style = 0;
		else
			self->style = 1;
	}

	self->s.effects |= (EF_COLOR_SHELL);
	if (self->style)
		self->s.renderfx = RF_SHELL_RED;
	else
		self->s.renderfx = RF_SHELL_BLUE;

	self->s.angles[PITCH] = -45;
	self->s.angles[YAW] += FRAMETIME * 120;

	if (self->s.angles[YAW] > 360)
		self->s.angles[YAW] -= 360;

	self->nextthink = level.time + 0.1;
}

void hw_touch_flag(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf) {
	if (!other->takedamage && (level.time - ent->touch_debounce_time) > 0.2)
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex(va("hw/hw_ding%d.wav", GetRandom(1, 5))), 1, ATTN_NORM, 0);
		ent->touch_debounce_time = level.time;
	}

	Touch_Item(ent, other, plane, surf);
}

void hw_dropflag (edict_t *ent, gitem_t *item)
{
	edict_t *flag;
	vec3_t dir = { 0, 0, 0 };

	float speed = 600;

	//if (!G_EntExists(ent))
	//	return;
	if (!ent || !ent->inuse || G_IsSpectator(ent))
		return;

	if (!hw->value || !ent->client->pers.inventory[halo_index])
		return;

	gi.bprintf(PRINT_HIGH, "%s dropped the halo!\n", ent->client->pers.netname);
	flag = Drop_Item (ent, item);
	flag->think = hw_flagthink;
	flag->touch = hw_touch_flag;
	flag->count = 0;
	//Wait a second before starting to think
	flag->nextthink = level.time + 1.0;
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
	ValidateSelectedItem (ent);
	gi.sound(flag, CHAN_ITEM, gi.soundindex( va("hw/hw_ding%d.wav", GetRandom(1, 5)) ), 1, ATTN_NORM, 0);

	// toss this flag
	dir[YAW] = GetRandom(0, 360);
	dir[PITCH] = -60;
	AngleVectors(dir, dir, NULL, NULL);
	VectorScale(dir, speed, dir);

	VectorNormalize(dir);
	VectorScale(dir, speed, dir);
	VectorCopy(dir, flag->velocity);
}

void hw_find_spawn_point(edict_t* flag)
{
	vec3_t dir = {0, 0, 0};
	float speed = 600;
	edict_t* it = NULL;
	edict_t* candidates[32];
	int cnt = 0;
	int retries = 32;

	while ((it = G_Find(it, FOFS(classname), "info_player_deathmatch")) && cnt < 32)
	{
		candidates[cnt] = it;
		cnt++;
	}

	// throw in a direction away from a wall
	while (retries > 0) {
		trace_t tr;
		vec3_t end;

		it = candidates[GetRandom(1, cnt) - 1];
		dir[YAW] = GetRandom(0, 360);
		dir[PITCH] = -45;
		AngleVectors(dir, dir, NULL, NULL);
		VectorScale(dir, speed, dir);

		VectorSet(flag->s.origin, it->s.origin[0], it->s.origin[1], it->s.origin[2] + 16);
		VectorAdd(flag->s.origin, dir, end);

		tr = gi.trace(flag->s.origin, end, flag->mins, flag->maxs, flag, MASK_SOLID);

		if (tr.fraction != 1.0)
			break;

		flag->speed = speed;
		VectorNormalize(dir);
		VectorScale(dir, speed, dir);
		VectorCopy(dir, flag->velocity);
		// vectoangles(dir, flag->s.angles);
		retries--;
	}
}


void hw_spawnflag (void)
{
	edict_t *flag;

	flag = Spawn_Item(FindItemByClassname("item_flaghw"));
	flag->think = hw_flagthink;
	flag->nextthink = level.time + FRAMETIME;
	flag->takedamage = DAMAGE_NO; // this should fix a nasty bug. should.
	flag->touch = hw_touch_flag;
	flag->movetype = MOVETYPE_BOUNCE;

	hw_find_spawn_point(flag);

	FLAG_FRAMES = 0;
	flag->s.effects |= EF_BLASTER;
	flag->s.renderfx = RF_GLOW;

	flag->s.angles[PITCH] = 90;                     // Set the right angles
	flag->s.angles[YAW] = 0;                       //
	flag->s.angles[ROLL] = 45;                       //
	flag->touch_debounce_time = level.time;

	gi.sound(flag, CHAN_ITEM, gi.soundindex( va("hw/hw_ding%d.wav", GetRandom(1, 5)) ), 1, ATTN_NORM, 0);
	gi.linkentity(flag);
}

void hw_init()
{
	halo_index = ITEM_INDEX(FindItem("Halo"));

	hw_spawnflag();
	gi.bprintf(PRINT_HIGH, "Find the halo!\n");
}