#include "g_local.h"
#include "../../gamemodes/ctf.h"

#define SENTRY_IDLE_LEFT		1
#define SENTRY_IDLE_RIGHT		2
#define SENTRY_TARGET_RANGE		764		// target aquisition distance
#define SENTRY_ATTACK_RANGE		1024	// maximum attacking distance
#define SENTRY_BUILD_TIME		2.0		// time it takes to build a sentry
#define SENTRY_FOV				60		// field of vision, in degrees
#define SENTRY_REGEN_TIME		30		// seconds it takes sentry to regenerate to full
//#define SENTRY_MAX_HEALTH		(50+5*self->creator->myskills.abilities[BUILD_SENTRY].current_level)
//#define SENTRY_MAX_ARMOR		(50+15*self->creator->myskills.abilities[BUILD_SENTRY].current_level)
#define SENTRY_MAX_AMMO			(MINISENTRY_INITIAL_AMMO+MINISENTRY_ADDON_AMMO*self->creator->myskills.abilities[BUILD_SENTRY].current_level)

void minisentry_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (!self->enemy && G_ValidTarget(self, other, true) 
		&& (entdist(self, other)<SENTRY_ATTACK_RANGE))
		self->enemy = other;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 2;
	gi.sound (self, CHAN_VOICE, gi.soundindex ("tank/tnkpain2.wav"), 1, ATTN_NORM, 0);	
}

void minisentry_remove (edict_t *self)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	if (self->creator && self->creator->inuse)
		self->creator->num_sentries--;

	// remove base immediately
	if (self->owner && self->owner->inuse)
		G_FreeEdict(self->owner);

	if (self->creator && self->creator->client)
		layout_remove_tracked_entity(&self->creator->client->layout, self);

	// prep sentry for removal
	self->think = BecomeExplosion1;
	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->svflags |= SVF_NOCLIENT;
	self->deadflag = DEAD_DEAD;
	self->nextthink = level.time + FRAMETIME;
	gi.unlinkentity(self);
}

void minisentry_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	//FIXME: it is unsafe to remove an entity immediately!
	//gi.dprintf("minisentry_die()\n");

	if (self->creator && self->creator->inuse)
		safe_cprintf(self->creator, PRINT_HIGH, "Your mini-sentry was destroyed.\n");

	minisentry_remove(self);
}

void drone_heal(edict_t* self, edict_t* other, qboolean heal_while_being_damaged);
void minisentry_reload(edict_t* self, edict_t *other)
{
	int	player_ammo;
	edict_t* player;

	// entity must be alive
	if (!G_EntIsAlive(other))
		return;
	// must be a player entity
	if (other->client)
		player = other;
	else if (PM_MonsterHasPilot(other))
		player = other->owner;
	else
		return;

	// must be in need of ammo
	if (self->light_level < self->monsterinfo.jumpdn)
	{
		player_ammo = player->client->pers.inventory[self->num_hammers];

		//If player has more bullets than needed to fill up the gun
		if (self->light_level + 4 * player_ammo > self->monsterinfo.jumpdn)
		{
			player_ammo -= 0.25 * (self->monsterinfo.jumpdn - self->light_level);
			self->light_level = self->monsterinfo.jumpdn;
		}
		else	//Player loads all their bullets into the gun
		{
			self->light_level += 4 * player_ammo;
			player_ammo = 0;
		}

		// has player's inventory been modified?
		if (player->client->pers.inventory[self->num_hammers] != player_ammo)
		{
			player->client->pers.inventory[self->num_hammers] = player_ammo; // update player's ammo
			gi.sound(self, CHAN_ITEM, gi.soundindex("misc/w_pkup.wav"), 1, ATTN_STATIC, 0);
		}
	}
}

void minisentry_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(self, other, plane, surf);

	// limit how often sentry can be repaired/reloaded
	if (self->sentrydelay > level.time)
		return;
	// other must be valid and a client
	if (!G_EntIsAlive(other) || !other->client)
		return;
	// only teammates can repair/reload
	if (!OnSameTeam(self, other))
		return;

	minisentry_reload(self, other);
	if (self->health < self->max_health || self->monsterinfo.power_armor_power < self->monsterinfo.max_armor)
	{
		drone_heal(self, other, true);
		gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
	}

	safe_cprintf(other, PRINT_HIGH, "Sentry Health: %d/%d Armor: %d/%d Ammo: %d/%d\n", self->health, self->max_health, self->monsterinfo.power_armor_power, self->monsterinfo.max_armor, self->light_level, self->monsterinfo.jumpdn);
	self->sentrydelay = level.time + 2.5;// SENTRY_RELOAD_DELAY

	/*
	//4.2 if this is a player-monster (morph), then we should be looking at the client/player
	if (PM_MonsterHasPilot(other))
		other = other->activator;
	
	if (other && other->inuse && (other == self->creator) && (level.time > self->random))
	{
		safe_cprintf(self->creator, PRINT_HIGH, "Rotating sentry gun 45 degrees.\n");

		VectorCopy(self->move_angles, self->s.angles); // reset angles to default
		self->s.angles[YAW]+=45;
		ValidateAngles(self->s.angles);
		VectorCopy(self->s.angles, self->move_angles); // update default angles
		self->random = level.time + 1;
		self->nextthink = level.time + 2;
	}
	*/
}

qboolean minisentry_checkposition (edict_t *self)
{
	vec3_t	end;
	trace_t tr;
	
	// make sure there is still ground below or above us
	if (self->style == SENTRY_UPRIGHT)
	{
		VectorCopy(self->s.origin, end);
		end[2]--;// -= fabsf(self->mins[2])+1;
		tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SHOT);//MASK_SOLID);
		if (tr.fraction == 1.0 || (tr.ent != world && tr.ent->mtype != M_LASERPLATFORM))
		{
			//gi.dprintf("%d %d %d\n", tr.fraction==1.0?1:0,tr.ent==world?1:0,tr.ent->mtype==M_LASERPLATFORM?1:0);
			return false;
		}
	}
	else
	{
		VectorCopy(self->s.origin, end);
		end[2]++;// += self->maxs[2]+1;
		tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_SHOT);//MASK_SOLID);
		if (tr.fraction == 1.0 || (tr.ent != world && tr.ent->mtype != M_LASERPLATFORM))
		{
			//gi.dprintf("%d %d %d\n", tr.fraction==1.0?1:0,tr.ent==world?1:0,tr.ent->mtype==M_LASERPLATFORM?1:0);
			return false;
		}
	}
	return true;
}

qboolean minisentry_findtarget (edict_t *self)
{
	edict_t	*target=NULL;

	// don't retarget too quickly
	if (self->last_move_time > level.time)
		return false;
	while ((target = findclosestradius_targets (target, self, self->monsterinfo.sight_range)) != NULL)
	{
		if (!G_ValidTarget_Lite(self, target, true))
			continue;
		if (!infov(self, target, SENTRY_FOV))
			continue;
		self->enemy = target;
		self->last_move_time = level.time + 1.0;
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/turrspot.wav"), 1, ATTN_NORM, 0);
		return true;
	}
	return false;
}

void minisentry_lockon (edict_t *self)
{
	float	max, temp, deg;
	vec3_t	angles, end, v;

	// curse causes minisentry to fail to lock-on to enemy
	if ((que_typeexists(self->curses, CURSE)) && random() <= 0.8)
		return;

	// rotate quickly
	temp = self->yaw_speed;
	self->yaw_speed *= 3;

	// aim at target
	G_EntMidPoint(self->enemy, end);
	VectorSubtract(end, self->s.origin, v);
	VectorNormalize(v);
	vectoangles(v, angles);
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw(self);

	// restore original yaw speed
	self->yaw_speed = temp; 
	// vectoangles returns angles between (-) 0-360, this restores the "normal" range of (+/-) 180
	if (angles[PITCH] < -180)
		angles[PITCH] += 360;
	self->s.angles[PITCH] = angles[PITCH];
	//ValidateAngles(self->s.angles);

	//gi.dprintf("ideal yaw %.0f pitch %.0f\n", angles[YAW], angles[PITCH]);
	//gi.dprintf("actual yaw %.0f pitch %.0f\n", self->s.angles[YAW], self->s.angles[PITCH]);

	// maximum pitch in either direction
	if (self->mtype == M_BEAMSENTRY)
		deg = 30;
	else
		deg = 5;

	if (self->enemy->s.origin[2] > self->s.origin[2]) // if the enemy is above
	{	
		//note: negative PITCH tilts up 0 - (-)180
		max = -deg;
		//if (self->owner && self->owner->style == SENTRY_FLIPPED)
			//max = 360-deg; // allow 5 degrees up
		//else
		//	max = 315; // allow 45 degrees up
			//gi.dprintf("enemy ABOVE! max %.0f\n", max);
		if (self->s.angles[PITCH] < max)
			self->s.angles[PITCH] = max;
	}
	else
	{
		//note: positive PITCH tilts down 0 - 180

		//if (self->owner && self->owner->style == SENTRY_FLIPPED)
		//	max = 45; // allow 45 degrees down
		//else
			max = deg; // allow 5 degrees down
			//gi.dprintf("enemy BELOW! max %.0f\n", max);
		if (self->s.angles[PITCH] > max)
			self->s.angles[PITCH] = max;
	}
}

void minisentry_rocket_attack(edict_t* self, vec3_t start)
{
	float		speed, dmg_radius;
	vec3_t		aim;

	if (level.time > self->wait)
	{
		speed = 650 + 35 * self->creator->myskills.abilities[BUILD_SENTRY].current_level;
		MonsterAim(self, -1, speed, true, -1, aim, start);
		dmg_radius = self->radius_dmg;
		if (dmg_radius > 150)
			dmg_radius = 150;
		fire_rocket(self, start, aim, self->radius_dmg, speed, dmg_radius, self->radius_dmg);
		if (que_typeexists(self->curses, AURA_HOLYFREEZE))
			self->wait = level.time + 2.0;
		else if (self->chill_time > level.time)
			self->wait = level.time + (1.0 * (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level));
		else
			self->wait = level.time + 1.0;
	}
}

void minisentry_bullet_attack(edict_t* self, vec3_t start)
{
	float	chance;
	vec3_t	aim;

	if (que_typeexists(self->curses, AURA_HOLYFREEZE) && !(sf2qf(level.framenum) % 2))
		return;

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	MonsterAim(self, -1, 0, true, -1, aim, start);
	fire_bullet(self, start, aim, self->dmg, 2 * self->dmg, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_SENTRY);
	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/plaser.wav"), 1, ATTN_NORM, 0);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
	gi.multicast(start, MULTICAST_PVS);

	self->light_level--; // decrease ammo
}

void brain_beam_sparks(vec3_t start);

void minisentry_beam_attack(edict_t* self, vec3_t start)
{
	float	chance;
	vec3_t	forward, end;
	trace_t	tr;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	if (que_typeexists(self->curses, AURA_HOLYFREEZE) && !(sf2qf(level.framenum) % 2))
		return;

	// chill effect reduces attack rate/refire
	if (self->chill_time > level.time)
	{
		chance = 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);
		if (random() > chance)
			return;
	}

	if (!nearfov(self, self->enemy, 20, 10))
		return;
	//AngleVectors(self->s.angles, forward, NULL, NULL);

	// aim at target
	G_EntMidPoint(self->enemy, end);
	VectorSubtract(end, start, forward);
	VectorNormalize(forward);

	VectorMA(start, 8192, forward, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	brain_beam_sparks(tr.endpos);

	T_Damage(tr.ent, self, self, forward, tr.endpos, tr.plane.normal, self->dmg, 0, DAMAGE_ENERGY, MOD_SENTRY_BEAM);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_LASER);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PHS);

	if (level.time > self->wait)
	{
		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/phrwea.wav"), 1, ATTN_NORM, 0);
		self->wait = level.time + 2.5;
	}
	else if (level.time == self->wait)
	{
		// delay next attack
		self->monsterinfo.attack_finished = level.time + 1.0;
	}
	self->light_level--; // decrease ammo
}

qboolean infov (edict_t *self, edict_t *other, int degrees);
void minisentry_attack (edict_t *self)
{
	vec3_t		forward, start;

	minisentry_lockon(self);

	if (!nearfov(self, self->enemy, 45, 30))
		return;

	// out of ammo?
	if (self->light_level < 1)
	{
		if (self->mtype == M_MINISENTRY)
			self->s.frame = 7; //idle
		return;
	}

	// calculate muzzle location
	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorCopy(self->s.origin, start);
	if (self->owner && self->owner->style == SENTRY_FLIPPED)
		start[2] = self->absmin[2] + 16;// -= fabsf(self->mins[2]);
	else
		start[2] = self->absmax[2] - 16;// += self->maxs[2];
	VectorMA(start, (self->maxs[0] + 1), forward, start);

	if (self->mtype == M_MINISENTRY)
	{
		// cycle attack frames
		if (self->s.frame == 8)
			self->s.frame = 7; //idle
		else
			self->s.frame = 8; //firing
		minisentry_rocket_attack(self, start);
		minisentry_bullet_attack(self, start);

	}
	else
	{
		minisentry_beam_attack(self, start);
	}
}

void minisentry_idle (edict_t *self)
{
	float	min_yaw, max_yaw;

	if (self->delay > level.time)
		return;

	if (self->mtype == M_MINISENTRY)
		self->s.frame = 7;//idle

	if (self->delay == level.time)
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/gunidle1.wav"), 0.5, ATTN_NORM, 0);

//	self->yaw_speed = 5;
	max_yaw = self->move_angles[YAW] + 45;
	min_yaw = self->move_angles[YAW] - 45;
	AngleCheck(&max_yaw);
	AngleCheck(&min_yaw);

	//gi.dprintf("current %f default %f min %f max %f dir %d\n", self->s.angles[YAW], self->move_angles[YAW], min_yaw, max_yaw, self->style);

	// sentry scans side-to-side looking for targets
	if (self->style == SENTRY_IDLE_RIGHT)
	{
		self->ideal_yaw = max_yaw;
		M_ChangeYaw(self);
		if ((int)self->s.angles[YAW] == (int)self->ideal_yaw)
		{
			self->style = SENTRY_IDLE_LEFT;
			self->delay = level.time + 1.0;
		}
	}
	else
	{	
		self->ideal_yaw = min_yaw;
		M_ChangeYaw(self);
		if ((int)self->s.angles[YAW] == (int)self->ideal_yaw)
		{
			self->style = SENTRY_IDLE_RIGHT;
			self->delay = level.time + 1.0;
		}
	}	
}

void minisentry_regenerate (edict_t *self)
{
	if (!(level.framenum % (int)(1 / FRAMETIME)))
	{
		// regenerate health
		if (self->health < self->max_health)
			self->health += self->max_health/SENTRY_REGEN_TIME;
		if (self->health > self->max_health)
			self->health = self->max_health;
		// regenerate armor
		if (self->monsterinfo.power_armor_power < self->monsterinfo.max_armor)
			self->monsterinfo.power_armor_power += self->monsterinfo.max_armor/SENTRY_REGEN_TIME;
		if (self->monsterinfo.power_armor_power > self->monsterinfo.max_armor)
			self->monsterinfo.power_armor_power = self->monsterinfo.max_armor;
		// regenerate ammo
		if (self->light_level < SENTRY_MAX_AMMO)
			self->light_level += SENTRY_MAX_AMMO/SENTRY_REGEN_TIME;
		if (self->light_level > SENTRY_MAX_AMMO)
			self->light_level = SENTRY_MAX_AMMO;
	}
//	gi.dprintf("%d/%d armor\n", self->monsterinfo.power_armor_power, self->monsterinfo.max_armor);
}

void minisentry_sparks(edict_t* self)
{
	float	min, max, dist;
	vec3_t start, forward, up,angle;

	VectorCopy(self->s.angles, angle);
	
	angle[YAW] = GetRandom(0, 360);
	AngleVectors(self->s.angles, NULL, NULL, up);
	vectoangles(up, up);
	AngleCheck(&up[PITCH]);
	min = up[PITCH] - 45;
	max = up[PITCH] + 45;
	angle[PITCH] = GetRandom(min, max);

	VectorCopy(self->s.origin, start);
	
	if (self->style == SENTRY_FLIPPED)
		dist = fabs(self->mins[2]);
	else
		dist = self->maxs[2];
	//gi.dprintf("pitch %.0f min %.0f max %0.f\n", up[PITCH], min, max);
	//angle[PITCH] = 270;
	//AngleCheck(&angle[ROLL]);
	//AngleCheck(&angle[PITCH]);
	ValidateAngles(angle);

	AngleVectors(angle, forward, NULL, NULL);
	VectorMA(start, dist, forward, start);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(20);
	gi.WritePosition(start);
	gi.WriteDir(forward);
	gi.WriteByte(0xdad0dcd2);//(0x000000FF);
	gi.multicast(start, MULTICAST_PVS);

	gi.sound(self, CHAN_VOICE, gi.soundindex("world/spark1.wav"), 1, ATTN_STATIC, 0);
}

void minisentry_checkstatus(edict_t* self)
{
	qboolean warn = false;

	if (self->health < (int)(0.3 * self->max_health))
	{
		if (level.time > self->shield_activate_time)
		{
			minisentry_sparks(self);
			self->shield_activate_time = level.time + GetRandom(5, 30) * FRAMETIME;
		}
		warn = true;
		//safe_cprintf(self->creator, PRINT_HIGH, "Your sentry needs repairs!\n");
	}

	if (self->light_level < 0.2 * self->monsterinfo.jumpdn)
	{
		//safe_cprintf(self->creator, PRINT_HIGH, "Your sentry needs ammo!\n");
		warn = true;
	}

	if (warn && level.time > self->msg_time)
	{
		safe_cprintf(self->creator, PRINT_HIGH, "Your sentry needs your attention!\n");
		self->monsterinfo.selected_time = level.time + 1.0; // blink
		self->msg_time = level.time + 10;
	}
}

void minisentry_think (edict_t *self)
{
	float	modifier, temp;
	que_t	*slot=NULL;

	if (!self->owner || !self->owner->inuse)
	{
		minisentry_remove(self);
		return;
	}
	else if (self->removetime > 0)
	{
		qboolean converted=false;

		if (self->flags & FL_CONVERTED)
			converted = true;

		if (level.time > self->removetime)
		{
			// if we were converted, try to convert back to previous owner
			if (converted && self->prev_owner && self->prev_owner->inuse)
			{
				if (!ConvertOwner(self->prev_owner, self, 0, false))
				{
					minisentry_remove(self);
					return;
				}
			}
			else
			{
				minisentry_remove(self);
				return;
			}
		}
		// warn the converted monster's current owner
		else if (converted && self->creator && self->creator->inuse && self->creator->client 
			&& (level.time > self->removetime-5) && !(level.framenum % (int)(1 / FRAMETIME)))
				safe_cprintf(self->creator, PRINT_HIGH, "%s conversion will expire in %.0f seconds\n", 
					V_GetMonsterName(self), self->removetime-level.time);	
	}

	V_HealthCache(self, (int)(0.2 * self->max_health), 1);
	V_ArmorCache(self, (int)(0.2 * self->monsterinfo.max_armor), 1);

	// sentry is stunned
	if (self->holdtime > level.time)
	{
		M_SetEffects(self);
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	minisentry_checkstatus(self);

	// toggle sentry spotlight
	if (level.daytime && self->flashlight)
		FL_make(self);
	else if (!level.daytime && !self->flashlight)
		FL_make(self);

	// is the sentry slowed by holy freeze?
	temp = self->yaw_speed;
	slot = que_findtype(self->curses, slot, AURA_HOLYFREEZE);
	if (slot)
	{
		modifier = 1 / (1 + 0.1 * slot->ent->owner->myskills.abilities[HOLY_FREEZE].current_level);
		if (modifier < 0.25) modifier = 0.25;
		self->yaw_speed *= modifier;
	}

	// chill effect slows sentry rotation speed
	if(self->chill_time > level.time)
		self->yaw_speed *= 1 / (1 + CHILL_DEFAULT_BASE + CHILL_DEFAULT_ADDON * self->chill_level);

	if (!self->enemy)
	{
		//minisentry_regenerate(self);
		if (minisentry_findtarget(self))
			minisentry_attack(self);
		else
			minisentry_idle(self);
	}
	else
	{
		if (G_ValidTarget(self, self->enemy, true) 
			&& (entdist(self, self->enemy)<=SENTRY_ATTACK_RANGE))
		{
			minisentry_attack(self);
		}
		else
		{
			self->enemy = NULL;
			if (minisentry_findtarget(self))
			{
				minisentry_attack(self);
			}
			else
			{
				minisentry_idle(self);
				VectorCopy(self->move_angles, self->s.angles);
			}
		}
	}

	self->yaw_speed = temp; // restore original yaw speed
	M_SetEffects(self);
	self->nextthink = level.time + FRAMETIME;
}

void base_think (edict_t *self)
{
	// make sure sentry has settled down
	if (!minisentry_checkposition(self))
	{
		BecomeExplosion1(self);
		return;
	}
	self->nextthink = level.time + FRAMETIME;
}

void base_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->creator && self->creator->inuse)
	{
		self->creator->num_sentries--;
		if (self->creator->num_sentries < 0)
			self->creator->num_sentries = 0;
	}
	BecomeExplosion1(self);
}

void base_createturret (edict_t *self)
{
	edict_t		*sentry;
	vec3_t		end;
	trace_t		tr;
	int			casterlevel, talentLevel;
	float		ammo_mult=1.0;

	// make sure sentry has settled down
	if (!G_EntIsAlive(self->creator) || !minisentry_checkposition(self))
	{
		if (self->creator)
		{
			// refund player's power cubes
			if (self->creator->client)
				self->creator->client->pers.inventory[power_cube_index] += self->monsterinfo.cost;
			self->creator->num_sentries--;
			if (self->creator->num_sentries < 0)
				self->creator->num_sentries = 0;
		}
		BecomeExplosion1(self);
		return;
	}
	self->movetype = MOVETYPE_NONE; // lock down base
	self->takedamage = DAMAGE_NO; // the base is invulnerable

	// 3.8 base bbox no longer necessary, turret takes over
	VectorClear(self->mins);
	VectorClear(self->maxs);
	self->solid = SOLID_NOT;

	// create basic ent for sentry
	sentry = G_Spawn();
	sentry->creator = self->creator;
	sentry->owner = self; // the base becomes the owner
	VectorCopy(self->s.angles, sentry->s.angles);
	sentry->think = minisentry_think;
	sentry->nextthink = level.time + FRAMETIME;
	sentry->s.renderfx |= RF_IR_VISIBLE;
	// who really wanted to chase sentries anyway
	// sentry->flags |= FL_CHASEABLE; // 3.65 indicates entity can be chase cammed
	sentry->solid = SOLID_BBOX;
	sentry->movetype = MOVETYPE_NONE;
	sentry->clipmask = MASK_MONSTERSOLID;
	sentry->mass = 100;
	sentry->classname = "msentrygun";
	//sentry->viewheight = 16;
	sentry->takedamage = DAMAGE_AIM;
	sentry->mtype = self->orders; // copys mtype from base to sentry
	sentry->touch = minisentry_touch;
	
	sentry->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;

	//Talent: Storage Upgrade
    talentLevel = vrx_get_talent_level(self->creator, TALENT_STORAGE_UPGRADE);
	ammo_mult += 0.2 * talentLevel;

	sentry->monsterinfo.sight_range = SENTRY_TARGET_RANGE; // az

	// set ammo
	sentry->monsterinfo.jumpdn = SENTRY_MAX_AMMO * ammo_mult; // max ammo
	sentry->light_level = sentry->monsterinfo.jumpdn; // current ammo

	//get player's current_level for BUILD_SENTRY
	casterlevel = self->monsterinfo.level;
	sentry->monsterinfo.level = casterlevel; // used for adding monster exp
	sentry->monsterinfo.control_cost = 2 * M_CONTROL_COST_SCALE; // used for adding monster exp

	//set health
	sentry->health = MINISENTRY_INITIAL_HEALTH + (MINISENTRY_ADDON_HEALTH * casterlevel);
	sentry->monsterinfo.power_armor_power = MINISENTRY_INITIAL_ARMOR + (MINISENTRY_ADDON_ARMOR * casterlevel);
//	if (sentry->health > MINISENTRY_MAX_HEALTH)
//		sentry->health = MINISENTRY_MAX_HEALTH;
	//if (sentry->monsterinfo.power_armor_power > MINISENTRY_MAX_ARMOR)
	//	sentry->monsterinfo.power_armor_power = MINISENTRY_MAX_ARMOR;
	sentry->max_health = sentry->health;
	sentry->monsterinfo.max_armor = sentry->monsterinfo.power_armor_power;

	sentry->die = minisentry_die;
	sentry->pain = minisentry_pain;
	sentry->yaw_speed = 5;

	VectorCopy(self->s.origin, end);

	if (sentry->mtype == M_BEAMSENTRY)
	{
		//sentry->s.modelindex = gi.modelindex("models/weapons/g_bfg/tris.md2");
		sentry->s.modelindex = gi.modelindex("models/objects/turret/x12bfgcannon.md2");
		sentry->dmg = 10 + 5 * casterlevel;
		sentry->num_hammers = cell_index; // this sentry uses cells for ammo
		if (self->style == SENTRY_UPRIGHT)
		{
			VectorSet(sentry->mins, -25, -25, -24);
			VectorSet(sentry->maxs, 25, 25, 24);
			end[2] += fabsf(sentry->mins[2]) + 1;
		}
		else
		{
			VectorSet(sentry->mins, -25, -25, -24);
			VectorSet(sentry->maxs, 25, 25, 24);
			end[2] -= sentry->maxs[2] + 1;
		}	
	}
	else
	{
		sentry->num_hammers = bullet_index; // this sentry uses bullets for ammo
		sentry->s.modelindex = gi.modelindex("models/sentry/turret/tris.md2");
		sentry->s.frame = 7; //idle frame
		//set damage
		sentry->dmg = MINISENTRY_INITIAL_BULLET + (MINISENTRY_ADDON_BULLET * casterlevel);// bullet damage
		sentry->radius_dmg = MINISENTRY_INITIAL_ROCKET + (MINISENTRY_ADDON_ROCKET * casterlevel); // rocket damage
		if (sentry->dmg > MINISENTRY_MAX_BULLET)
			sentry->dmg = MINISENTRY_MAX_BULLET;
		if (sentry->radius_dmg > MINISENTRY_MAX_ROCKET)
			sentry->radius_dmg = MINISENTRY_MAX_ROCKET;
		if (self->style == SENTRY_UPRIGHT)
		{
			VectorSet(sentry->mins, -25, -25, -8);
			VectorSet(sentry->maxs, 25, 25, 45);
			end[2] += fabsf(sentry->mins[2]) + 1;
		}
		else
		{
			VectorSet(sentry->mins, -25, -25, -45);
			VectorSet(sentry->maxs, 25, 25, 8);
			VectorCopy(self->s.origin, end);
			end[2] -= sentry->maxs[2] + 1;
		}
	}

	// make sure position is valid
	tr = gi.trace(end, sentry->mins, sentry->maxs, end, sentry, MASK_SHOT);
	if (tr.contents & MASK_SHOT)
	{
		if (self->creator)
		{
			// refund player's power cubes
			if (self->creator->client)
				self->creator->client->pers.inventory[power_cube_index] += self->monsterinfo.cost;
			self->creator->num_sentries--;
			if (self->creator->num_sentries < 0)
				self->creator->num_sentries = 0;
		}
		//gi.dprintf("%s\n", tr.ent?tr.ent->classname:"null");
		BecomeExplosion1(self);
		BecomeExplosion1(sentry);
		return;
	}

	if (self->creator && self->creator->client)
		layout_add_tracked_entity(&self->creator->client->layout, sentry);

	VectorCopy(tr.endpos, sentry->s.origin);
	VectorCopy(sentry->s.angles, sentry->move_angles);// save for idle animation
	gi.linkentity(sentry);
	
	gi.sound(sentry, CHAN_VOICE, gi.soundindex("weapons/turrset.wav"), 1, ATTN_NORM, 0);
	self->think = base_think; // base is done creating gun
	self->nextthink = level.time + FRAMETIME;
}

void SpawnMiniSentry (edict_t *ent, int cost, int mtype, float skill_mult, float delay_mult)
{
	int			sentries=0;//3.9
	qboolean	failed=false;
	vec3_t		offset, forward, right, start, end;
	edict_t		*base;
	trace_t		tr;

	if (debuginfo->value)
		gi.dprintf("DEBUG: SpawnMiniSentry()\n");

	// create basic ent for sentry base
	base = G_Spawn();
	base->orders = mtype; // mtype to be transferred to the gun entity
	base->activator = base->creator = ent;
	VectorCopy(ent->s.angles, base->s.angles);
	base->s.angles[PITCH] = 0;
	base->s.angles[ROLL] = 0;
	base->think = base_createturret;
	base->nextthink = level.time + SENTRY_BUILD_TIME * delay_mult;
	base->solid = SOLID_BBOX;
	base->movetype = MOVETYPE_TOSS;
	base->clipmask = MASK_MONSTERSOLID;
	base->mass = 300;
	base->health = 500;
	base->die = base_die;
	base->classname = "msentrybase";
	base->monsterinfo.level = ent->myskills.abilities[BUILD_SENTRY].current_level * skill_mult;
	base->takedamage = DAMAGE_YES; // need this to let sentry get knocked around
	base->monsterinfo.sight_range = SENTRY_TARGET_RANGE; // az

	// calculate starting position
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorMA(start, 128, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

	// base spawned in mid-air
	if (tr.fraction == 1.0)
	{	
		base->style = SENTRY_UPRIGHT;
	}
	// base spawned below us
	else if(tr.endpos[2] <= ent->s.origin[2])
	{
		// add the height of the base
		VectorCopy(tr.endpos, end);
		end[2] += fabsf(base->mins[2]);
		//base->movetype = MOVETYPE_NONE;
		base->style = SENTRY_UPRIGHT;
	}
	// base spawned above us
	else if (tr.ent && (tr.ent == world || tr.ent->mtype == M_LASERPLATFORM))
	{
		// flip base upside-down
		base->movetype = MOVETYPE_NONE;
		base->s.angles[ROLL] += 180;
		if (base->s.angles[ROLL] < 0)
			base->s.angles[ROLL] += 360;
		else if (base->s.angles[ROLL] > 360)
			base->s.angles[ROLL] -= 360;
		//VectorSet(base->mins, -32, -32, -12);
		//VectorSet(base->maxs, 32, 32, 0);
		// subtract the height of the base
		VectorCopy(tr.endpos, end);
		end[2] -= base->maxs[2];
		base->style = SENTRY_FLIPPED;
	}
	
	if (mtype == M_BEAMSENTRY)
	{
		base->s.modelindex = gi.modelindex("models/objects/turret/turret_heavy.md2");
		if (base->style == SENTRY_UPRIGHT)
		{
			VectorSet(base->mins, -32, -32, 0);
			VectorSet(base->maxs, 32, 32, 12);
		}
		else
		{
			VectorSet(base->mins, -32, -32, -12);
			VectorSet(base->maxs, 32, 32, 0);
		}
	}
	else
	{
		base->s.modelindex = gi.modelindex("models/sentry/base/tris.md2");
		if (base->style == SENTRY_UPRIGHT)
		{
			VectorSet(base->mins, -12, -12, 0);
			VectorSet(base->maxs, 12, 12, 36);
		}
		else
		{
			VectorSet(base->mins, -12, -12, -36);
			VectorSet(base->maxs, 12, 12, 0);
		}
	}

	// make sure the position is valid
	tr = gi.trace(end, base->mins, base->maxs, end, NULL, MASK_SHOT);
	if (tr.contents & MASK_SHOT)
	{
		G_FreeEdict(base);
		return;
	}
	VectorCopy(tr.endpos, base->s.origin);
	gi.linkentity(base);
	gi.sound(base, CHAN_VOICE, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);

	// 3.9 double sentry cost if there are too many sentries in CTF
	if (ctf->value)
	{
		sentries += 2*CTF_GetNumSummonable("Sentry_Gun", ent->teamnum);
		sentries += CTF_GetNumSummonable("msentrygun", ent->teamnum);

		if (sentries > MAX_MINISENTRIES)
			cost *= 2;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	base->monsterinfo.cost = cost;
	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->client->ability_delay = level.time + SENTRY_BUILD_TIME * delay_mult;
	ent->holdtime = level.time + SENTRY_BUILD_TIME * delay_mult;
	ent->num_sentries++;
	ent->lastsound = level.framenum;
}

void RemoveMiniSentries (edict_t *ent)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), "msentrygun")) != NULL) 
	{
		if (e && e->creator && (e->creator == ent) && !RestorePreviousOwner(e))
			minisentry_remove(e);
	}

	ent->num_sentries = 0;
}

#define MINISENTRY_AIM_NEAREST	0
#define MINISENTRY_AIM_ALL		1

void minisentry_setaim(edict_t* ent, edict_t* minisentry)
{
	vec3_t	forward, right, start, offset, end;
	trace_t	tr;

	// get start position for trace
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// get end position for trace
	VectorMA(start, 8192, forward, end);

	// get vector to the point client is aiming at and convert to angles
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);
	VectorSubtract(tr.endpos, minisentry->s.origin, forward);

	vectoangles(forward, forward);
	ValidateAngles(forward);

	// copy angles to minisentry
	minisentry->s.angles[YAW] = minisentry->move_angles[YAW] = forward[YAW];

	//minisentry->nextthink = level.time + 1.0;
	minisentry->delay = level.time + 1;
}

void Cmd_MinisentryAim_f(edict_t* ent, int option)
{
	edict_t* e = NULL;

	if (option == MINISENTRY_AIM_NEAREST)
	{
		while ((e = findclosestradius(e, ent->s.origin, 512)) != NULL)
		{
			if (!e->inuse)
				continue;
			if (e->mtype != M_MINISENTRY && e->mtype != M_BEAMSENTRY)
				continue;
			if (e->creator && e->creator->inuse && e->creator == ent)
			{
				minisentry_setaim(ent, e);
				break;
			}
		}
		safe_cprintf(ent, PRINT_HIGH, "Aiming sentry gun...\n");
	}
	else if (option == MINISENTRY_AIM_ALL)
	{
		while ((e = G_Find(e, FOFS(classname), "msentrygun")) != NULL)
		{
			if (e && e->inuse && e->creator && e->creator->client && (e->creator == ent))
				minisentry_setaim(ent, e);
		}
		safe_cprintf(ent, PRINT_HIGH, "Aiming sentry guns...\n");
	}
}

void Cmd_MiniSentry_f (edict_t *ent)
{
	int talentLevel, sentries=0, cost=SENTRY_COST;
	float skill_mult=1.0, cost_mult=1.0, delay_mult=1.0;//Talent: Rapid Assembly & Precision Tuning
	edict_t *scan=NULL;
	char* arg;

	arg = gi.args();

	if (debuginfo->value)
		gi.dprintf("%s just called Cmd_MiniSentry_f\n", ent->client->pers.netname);

	if (ent->myskills.abilities[BUILD_SENTRY].disable)
		return;

	if (!Q_strcasecmp(arg, "remove"))
	{
		RemoveMiniSentries(ent);
		return;
	}

	if (!Q_strcasecmp(arg, "aim"))
	{
		Cmd_MinisentryAim_f(ent, MINISENTRY_AIM_NEAREST);
		return;
	}

	if (!Q_strcasecmp(arg, "aimall"))
	{
		Cmd_MinisentryAim_f(ent, MINISENTRY_AIM_ALL);
		return;
	}

	// 3.9 double sentry cost if there are too many sentries in CTF
	if (ctf->value)
	{
		sentries += 2*CTF_GetNumSummonable("Sentry_Gun", ent->teamnum);
		sentries += CTF_GetNumSummonable("msentrygun", ent->teamnum);

		if (sentries > MAX_MINISENTRIES)
			cost *= 2;
	}

	// cost is doubled if you are a flyer or cacodemon below skill level 5
	if ((ent->mtype == MORPH_FLYER && ent->myskills.abilities[FLYER].current_level < 5) 
		|| (ent->mtype == MORPH_CACODEMON && ent->myskills.abilities[CACODEMON].current_level < 5))
		cost *= 2;

	//Talent: Rapid Assembly
    talentLevel = vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY);
	if (talentLevel > 0)
		delay_mult -= 0.1 * talentLevel;
	//Talent: Precision Tuning
    else if ((talentLevel = vrx_get_talent_level(ent, TALENT_PRECISION_TUNING)) > 0)
	{
		cost_mult += PRECISION_TUNING_COST_FACTOR * talentLevel;
		delay_mult += PRECISION_TUNING_DELAY_FACTOR * talentLevel;
		skill_mult += PRECISION_TUNING_SKILL_FACTOR * talentLevel;
	}
	cost *= cost_mult;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[BUILD_SENTRY].current_level, cost))
		return;

	if (ent->num_sentries >= MAX_MINISENTRIES)
	{
		safe_cprintf(ent, PRINT_HIGH, "You have reached the max of %d sentries\n", (int)MAX_MINISENTRIES);
		return;
	}

	if (ctf->value && (CTF_DistanceFromBase(ent, NULL, CTF_GetEnemyTeam(ent->teamnum)) < CTF_BASE_DEFEND_RANGE))
	{
		safe_cprintf(ent, PRINT_HIGH, "Can't build in enemy base!\n");
		return;
	}

	if (!Q_strcasecmp(gi.args(), "beam"))
		SpawnMiniSentry(ent, cost, M_BEAMSENTRY, skill_mult, delay_mult);
	else
		SpawnMiniSentry(ent, cost, M_MINISENTRY, skill_mult, delay_mult);
}
