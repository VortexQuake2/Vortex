#include "g_local.h"
#include <gamemodes/invasion.h>

int vrx_get_joined_players(qboolean include_bots) {
	edict_t* player;
	int i, clients = 0;

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse)
			continue;
		if (G_IsSpectator(player))
			continue;
		if (!include_bots && player->ai.is_bot)
			continue;

		clients++;
	}

	if (clients < 1)
		return 0;

	return clients;
}

int vrx_get_alive_players(void) {
	edict_t* player;
	int i, clients = 0;

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse)
			continue;
		if (G_IsSpectator(player))
			continue;
		if (player->myskills.boss)
			continue;
		if (!G_EntIsAlive(player))
			continue;
		if (player->ai.is_bot)
			continue;

		clients++;
	}

	if (clients < 1)
		return 0;

	return clients;
}

int PvMAveragePlayerLevel(void) {
	edict_t* player;
	int players = 0, levels = 0, average, i;

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse)
			continue;
		if (G_IsSpectator(player))
			continue;
		if (player->myskills.boss)
			continue;
		players++;
		levels += player->myskills.level;
	}

	if (players < 1)
		return 0;
	if (levels < 1)
		levels = 1;


	average = levels / players;

	if (average < 1)
		average = 1;

	if (debuginfo->value)
		gi.dprintf("DEBUG: PvM Average player level %d\n", average);
	return average;
}

int AveragePlayerLevel(void) {
	edict_t* player;
	int players = 0, levels = 0, average, i;

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse)
			continue;
		if (G_IsSpectator(player))
			continue;
		if (player->myskills.boss)
			continue;

		//if (player->ai.is_bot) // az: heheh
		//	continue;

		players++;
		levels += player->myskills.level;
		//	gi.dprintf("%s level %d added, total %d\n", player->client->pers.netname,
		//		player->myskills.level, players);
	}

	if (players < 1)
		return 0;
	if (levels < 1)
		levels = 1;

	average = levels / players;

	if (average < 1)
		average = 1;

	/* if (debuginfo->value)
		gi.dprintf("DEBUG: Average player level %d\n", average);
		*/
	return average;
}

int PVM_TotalMonsterLevels(edict_t* monster_owner)
{
	int		monsters = 0;
	float	mult = 1.0;
	edict_t* scan = NULL;

	while ((scan = G_Find(scan, FOFS(classname), "drone")) != NULL)
	{
		// found a live monster that we own
		if (G_EntIsAlive(scan) && scan->activator && (scan->activator == monster_owner))
		{
			//4.5 monster bonus flags
			if (scan->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
				|| scan->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING)
				mult = 25;
			else if (scan->monsterinfo.bonus_flags & BF_CHAMPION)
				mult = 3.0;
			else if (scan->monsterinfo.bonus_flags & BF_BERSERKER)
				mult = 1.5;
			else if (scan->monsterinfo.bonus_flags & BF_POSESSED)
				mult = 4.0;

			monsters += scan->monsterinfo.level * scan->monsterinfo.control_cost * mult;
		}
	}

	return monsters;
}

int PVM_TotalMonstersValue(edict_t* monster_owner)
{
	int         i = 0;
	float       mult = 1.0, temp = 0;
	edict_t* e = NULL;

	// gets the first monster in the list
	e = DroneList_Iterate();

	while (e) {
		// found a live monster that we own
		if (G_EntIsAlive(e) && (e->activator == monster_owner)) {
			// unique monsters are especially hard
			if (e->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
				|| e->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING)
				mult = 5.0;
			// champion monsters are harder than normal monsters
			else if (e->monsterinfo.bonus_flags & BF_CHAMPION)
				mult = 3.0;
			else if (e->monsterinfo.bonus_flags & BF_BERSERKER)
				mult = 1.5;
			else if (e->monsterinfo.bonus_flags & BF_POSESSED)
				mult = 4.0;
			// add this monster's value to the subtotal
			temp += e->monsterinfo.level * e->monsterinfo.control_cost * mult;
		}
		// iterate to the next monster in the list
		e = DroneList_Next(e);
	}

	return (int)temp;
}

int vrx_pvm_update_total_owned_monsters(edict_t *monster_owner, qboolean update) {
    int i = 0;
    edict_t *e = NULL;

	if (update) {
		e = DroneList_Iterate();

		while (e) {
			// found a live monster that we own
			if (G_EntIsAlive(e) && (e->activator == monster_owner)) {
				i++;
			}
			e = DroneList_Next(e);
		}
		monster_owner->num_monsters_real = i;
		return monster_owner->num_monsters_real;
	}
	return monster_owner->num_monsters_real; // will this work?
}

int vrx_remove_all_monsters(edict_t *monster_owner) {
    int i = 0;
    edict_t *e = NULL;

	e = DroneList_Iterate();

	while (e) {
		if (G_EntExists(e) && e->activator && (e->activator == monster_owner)) {
			M_Remove(e, false, false);
			i++;
		}
		e = DroneList_Next(e);
	}
	return i;
}


void vrx_pvm_spawn_monsters(edict_t* self, int max_monsters, int total_monsters)
{
	int max_spawn_this_cycle = GetRandom(0, max_monsters - total_monsters);

	if (self->count == 0) // first time
		max_spawn_this_cycle = max_monsters;
	else
		max_spawn_this_cycle = max(max_spawn_this_cycle, 3);

	while (total_monsters < max_monsters && max_spawn_this_cycle > 0) {
		int rnd;
		do {
			rnd = GetRandom(1, 15); // az: don't spawn soldiers
		} while (rnd == 10);

		edict_t* scan;
		if ((scan = vrx_create_new_drone(self, rnd, true, true, self->monsterinfo.scale)) != NULL) {
			if (scan->monsterinfo.walk && random() > 0.5)
				scan->monsterinfo.walk(scan);
			total_monsters++;
			max_spawn_this_cycle--;
			self->count++;
		}
	}
}

void vrx_pvm_try_spawn_boss(edict_t* self, int players)
{
	if (!BossExists() && (self->num_sentries < 1) && (players > 0.6 * maxclients->value)) {
		int chance = 10;

		if (ffa->value)
			chance *= 2;

		if (GetRandom(1, 200) <= chance) // oh yeah!
			CreateRandomPlayerBoss(true);
	}
}

//qboolean SpawnWorldMonster(edict_t *ent, int mtype);
void vrx_pvm_spawn_world_monsters(edict_t* self) {
	int players = vrx_get_joined_players(true);
	total_monsters = vrx_pvm_update_total_owned_monsters(self, false);

	// dm_monsters cvar sets the default number of monsters in a given map
	int max_monsters = level.r_monsters;
	if (level.r_monsters <= 0)
		max_monsters = dm_monsters->value;

	int threshold = 0.66f * max_monsters;
	int levelup_threshold = 0.4f * max_monsters;



	if (level.time > self->delay) {
		int total_monsters = vrx_pvm_update_total_owned_monsters(self, true);
		// adjust spawning delay based on efficiency of player monster kills
		if (total_monsters < max_monsters) {
			// if a minimum of monsters has been killed consider scaling monsters up
			if (total_monsters < threshold) {
				// WriteServerMsg(va("** %d monsters remain **", total_monsters), "Info", true, false);

				// if the players are too efficient then level monsters up
				if (total_monsters <= levelup_threshold && self->last_move_time < level.time && self->count) {
					self->monsterinfo.scale += max(1, self->monsterinfo.scale);
					self->last_move_time = level.time + 15.0;
				}

				// reduce delay between monster spawning
				self->random *= 0.8;

				// minimum delay
				if (self->random < 3.0)
					self->random = 3.0;
			} else {
				// if not enough monsters have been killed scale down
				// decrease spawn frequency
				self->random += 1.0;

				// scale monster level down
				if (self->last_move_time < level.time && self->monsterinfo.scale > 0) {
					self->monsterinfo.scale *= 0.7f;
					self->last_move_time = level.time + 15.0;
				}

				// maximum delay
				if (self->random > ffa_respawntime->value)
					self->random = ffa_respawntime->value;
			}
		}

		// spawn monsters until we reach the limit
		if (total_monsters < max_monsters) {
			vrx_pvm_spawn_monsters(self, max_monsters, total_monsters);
		}

		WriteServerMsg(va("World has %d/%d monsters. Next update in %.1f seconds. levelup threshold: %d",
			total_monsters, max_monsters,
			self->random, levelup_threshold), "Info", true, false);

		// wait awhile before trying to spawn monsters again
		self->delay = level.time + self->random;
	}

	// there is a 10-20% chance that a boss will spawn
	// if the server is more than 1/6 full

	vrx_pvm_try_spawn_boss(self, players);

	level.pvm.level_bonus = self->monsterinfo.scale;
	level.pvm.time_to_next_respawn = self->delay - level.time;
	self->nextthink = level.time + FRAMETIME;
}

void SpawnRandomBoss(edict_t* self) {
	// 3% chance for a boss to spawn a boss if there isn't already one spawned
	if (!SPREE_WAR && vrx_get_alive_players() >= 8 && self->num_sentries < 1) {
		int chance = GetRandom(1, 100);

		if ((chance >= 97) && vrx_create_new_drone(self, GetRandom(30, 31), true, true, 0)) {
			//gi.dprintf("Spawning a boss monster (chance = %d) at %.1f. Waiting 300 seconds to try again.\n",
			//	chance, level.time);
			// 5 minutes until we can try to spawn another boss
			self->nextthink = level.time + 300.0;
			return;
		}
	}

	self->nextthink = level.time + 60.0;// try again in 1 minute
}

edict_t *InitMonsterEntity(qboolean manual_spawn) {
    edict_t *monster;

	if (ctf->value || domination->value || ptr->value || tbi->value || trading->value || V_IsPVP())
		return NULL;//4.4

	//if (!pvm->value && !ffa->value)
	//	return NULL;

	monster = G_Spawn();

	if (manual_spawn)
		monster->style = 1;

	monster->classname = "MonsterSpawn";
	monster->svflags |= SVF_NOCLIENT;
	monster->mtype = M_WORLDSPAWN;
	monster->num_monsters_real = 0;
	monster->monsterinfo.level = 0;
	monster->last_move_time = level.time;

    /*if (V_IsPVP())
    monster->think = SpawnRandomBoss;//4.4
    else */
    // most of the time people don't like having their pvp interrupted by a boss
    if (INVASION_OTHERSPAWNS_REMOVED) {
        monster->think = vrx_inv_spawn_monsters;
        monster->notify_drone_death = vrx_inv_notify_monster_death;
    } else
        monster->think = vrx_pvm_spawn_world_monsters;

	monster->nextthink = pregame_time->value + FRAMETIME;
	monster->random = ffa_respawntime->value;//4.52 default delay to begin respawning monsters
	gi.linkentity(monster);

	return monster;
}

/*
=============
vrx_remove_player_summonables

Removes all edicts that the player owns
=============
*/

void RemoveMagmines(edict_t* ent);

void RemoveProxyGrenades(edict_t* ent);

void RemoveNapalmGrenades(edict_t* ent);

void sentrygun_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);

void RemoveExplodingArmor(edict_t* ent);

void RemoveExplodingBarrels(edict_t* ent, qboolean refund);

void RemoveAutoCannons(edict_t* ent);

void caltrops_removeall(edict_t* ent);

void spikegren_removeall(edict_t* ent);

void detector_removeall(edict_t* ent);

void minisentry_remove(edict_t* self);

void organ_removeall(edict_t* self, char* classname, qboolean refund);

void RemoveMiniSentries(edict_t* ent);

void RemoveAllLaserPlatforms(edict_t* ent);//4.5
void holyground_remove(edict_t* ent, edict_t* self);

void depot_remove(edict_t* self, edict_t* owner, qboolean effect);

void lasertrap_removeall(edict_t* ent, qboolean effect);
void RemoveFirewalls(edict_t* ent);

void vrx_remove_player_summonables(edict_t* self) {
	edict_t* from;

	//	gi.dprintf("vrx_remove_player_summonables()\n");

	lasertrap_removeall(self, true);
	holyground_remove(self, self->holyground);
	detector_removeall(self);
	caltrops_removeall(self);
	organ_removeall(self, "box", false);
	organ_removeall(self, "fmedi", false);
	organ_removeall(self, "gasser", false);
	organ_removeall(self, "obstacle", false);
	organ_removeall(self, "spiker", false);
	organ_removeall(self, "spikeball", false);
	organ_remove(self->healer, false);
	organ_remove(self->cocoon, false);
	spikegren_removeall(self);
	RemoveTotem(self->totem1);
	RemoveTotem(self->totem2);
	AuraRemove(self, 0);
	RemoveLasers(self);
	RemoveProxyGrenades(self);
	RemoveMagmines(self);
	RemoveNapalmGrenades(self);
	RemoveExplodingArmor(self);
	RemoveExplodingBarrels(self, false);
	RemoveAutoCannons(self);
	RemoveMiniSentries(self);
	RemoveAllLaserPlatforms(self);
	RemoveFirewalls(self);

	// scan for edicts that we own
	for (from = g_edicts; from < &g_edicts[globals.num_edicts]; from++) {
		edict_t* cl_ent;

		if (!from->inuse)
			continue;
		// remove sentry
		if ((from->creator) && (from->creator == self)
			&& !Q_stricmp(from->classname, "Sentry_Gun") && !RestorePreviousOwner(from)) {
			sentrygun_die(from, NULL, from->creator, from->health, from->s.origin);
			continue;
		}

		// remove monsters
		if ((from->activator) && (from->activator == self)
			&& !Q_stricmp(from->classname, "drone") && !RestorePreviousOwner(from)) {
			DroneRemoveSelected(self, from);
			M_Remove(from, false, true);
			self->num_monsters = 0;
			self->num_monsters_real = 0;
			self->num_skeletons = 0;
			self->num_golems = 0;
			continue;
		}

		// remove force wall
		if ((from->activator) && (from->activator == self)
			&& !Q_stricmp(from->classname, "Forcewall")) {
			from->think = BecomeTE;
			from->takedamage = DAMAGE_NO;
			from->deadflag = DEAD_DEAD;
			from->nextthink = level.time + FRAMETIME;
			continue;
		}

		// remove grenades
		if ((from->owner) && (from->owner == self)
			&& !Q_stricmp(from->classname, "grenade")) {
			self->max_pipes = 0;
			G_FreeEdict(from);
			continue;
		}

		// remove bolts
		if ((from->owner) && (from->owner == self)
			&& !Q_stricmp(from->classname, "bolt")) {
			G_FreeEdict(from);
		}

		// remove hammers
		cl_ent = G_GetClient(from);
		if (cl_ent && cl_ent->inuse && cl_ent == self
			&& !Q_stricmp(from->classname, "hammer")) {
			self->num_hammers = 0;
			G_FreeEdict(from);
			continue;
		}
	}
	// remove everything else
	if (self->lasersight) {
		G_FreeEdict(self->lasersight);
		self->lasersight = NULL;
	}
	if (self->flashlight) {
		G_FreeEdict(self->flashlight);
		self->flashlight = NULL;
	}

	if (self->supplystation) {
		depot_remove(self->supplystation, self, true);
	}
	if (self->skull && !RestorePreviousOwner(self->skull)) {
		//BecomeExplosion1(self->skull);
		self->skull->think = BecomeExplosion1;
		self->skull->takedamage = DAMAGE_NO;
		self->skull->deadflag = DEAD_DEAD;
		self->skull->nextthink = level.time + FRAMETIME;
		self->skull = NULL;
	}
	if (self->spirit) {
		self->spirit->think = G_FreeEdict;
		self->spirit->deadflag = DEAD_DEAD;
		self->spirit->nextthink = level.time + FRAMETIME;
		self->spirit = NULL;
	}
	hook_reset(self->client->hook);
}

int vrx_GetMonsterCost(int mtype) {
	int cost;

    switch (mtype) {
        case M_FLYER:
            cost = M_FLYER_COST;
            break;
        case M_INSANE:
            cost = M_INSANE_COST;
            break;
        case M_SOLDIERLT:
            cost = M_SOLDIERLT_COST;
            break;
        case M_SOLDIER:
            cost = M_SOLDIER_COST;
            break;
        case M_INFANTRY:
            cost = M_ENFORCER_COST;
            break;
        case M_GUNNER:
            cost = M_GUNNER_COST;
            break;
        case M_CHICK:
            cost = M_CHICK_COST;
            break;
        case M_PARASITE:
            cost = M_PARASITE_COST;
            break;
        case M_MEDIC:
            cost = M_MEDIC_COST;
            break;
        case M_BRAIN:
            cost = M_BRAIN_COST;
            break;
        case M_TANK:
            cost = M_TANK_COST; 
            break;
        case M_HOVER:
            cost = M_HOVER_COST;
            break;
        case M_SHAMBLER:
            cost = M_TANK_COST; //using tank atm
            break;
        case M_SUPERTANK:
            cost = M_SUPERTANK_COST;
            break;
        case M_COMMANDER:
            cost = M_COMMANDER_COST;
            break;
        default:
            cost = M_DEFAULT_COST;
            break;
    }
    return cost;
}

int vrx_GetMonsterControlCost(int mtype) {
	int cost;

    switch (mtype) {
        case M_FLYER:
            cost = M_FLYER_CONTROL_COST;
            break;
        case M_INSANE:
            cost = M_INSANE_CONTROL_COST;
            break;
        case M_SOLDIERLT:
            cost = M_SOLDIERLT_CONTROL_COST;
            break;
        case M_SOLDIER:
            cost = M_SOLDIER_CONTROL_COST;
            break;
        case M_INFANTRY:
            cost = M_ENFORCER_CONTROL_COST;
            break;
        case M_GUNNER:
            cost = M_GUNNER_CONTROL_COST;
            break;
        case M_CHICK:
            cost = M_CHICK_CONTROL_COST;
            break;
        case M_PARASITE:
            cost = M_PARASITE_CONTROL_COST;
            break;
        case M_MEDIC:
            cost = M_MEDIC_CONTROL_COST;
            break;
        case M_BRAIN:
            cost = M_BRAIN_CONTROL_COST;
            break;
        case M_TANK:
            cost = M_TANK_CONTROL_COST;
            break;
        case M_SHAMBLER:
            cost = M_TANK_CONTROL_COST; //using tank atm
            break;
        case M_HOVER:
            cost = M_HOVER_CONTROL_COST;
            break;
        case M_SUPERTANK:
            cost = M_SUPERTANK_CONTROL_COST;
            break;
        case M_COMMANDER:
            cost = M_COMMANDER_CONTROL_COST;
            break;
        default:
            cost = M_DEFAULT_CONTROL_COST;
            break;
    }
    return cost;
}

void shrapnel_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if ((surf && (surf->flags & SURF_SKY)) || !self->creator || !self->creator->inuse)
	{
		G_FreeEdict(self);
		return;
	}

	if (G_ValidTarget(self->creator, other, false, true))
	{
		T_Damage(other, self, self->creator, self->velocity, self->s.origin, plane->normal, self->dmg, 0, 0, self->style);

		gi.sound(other, CHAN_WEAPON, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
		G_FreeEdict(self);
	}
}

void shrapnel_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowShrapnel(edict_t* self, char* modelname, float speed, vec3_t origin, int dmg, int mod)
{
	edict_t* cl, * chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy(origin, chunk->s.origin);
	//gi.dprintf("%s: origin: %.0f %.0f %.0f\n", __func__, origin[0], origin[1], origin[2]);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 150 * random();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	VectorSet(chunk->mins, -4, -4, -2);
	VectorSet(chunk->maxs, 4, 4, 2);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_BBOX;
	chunk->clipmask = MASK_SHOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random() * 5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "shrapnel";
	//chunk->takedamage = DAMAGE_YES;
	chunk->dmg = dmg;
	chunk->die = shrapnel_die;
	chunk->touch = shrapnel_touch;
	chunk->style = mod; // means-of-death
	if (cl = G_GetClient(self))
		chunk->creator = cl; // owner-creator of shrapnel
	else
		chunk->creator = self;
	gi.linkentity(chunk);

	// cloak player-owned shrapnel in PvM if there are too many entities nearby
	if (pvm->value && cl && !vrx_spawn_nonessential_ent(chunk->s.origin))
		chunk->svflags |= SVF_NOCLIENT;
}

// finds a vector parallel to a wall plane pointing in the direction of lookDir
void projectOntoWall(vec3_t lookDir, vec3_t wallNormal, vec3_t parallelVector) {
	vec3_t projectionOntoNormal;
	// Calculate dot product L � N
	float dot = DotProduct(lookDir, wallNormal);

	// Project L onto N: (L � N) * N
	VectorScale(wallNormal, dot, projectionOntoNormal);

	// Subtract this from L to get the parallel vector
	VectorSubtract(lookDir, projectionOntoNormal, parallelVector);

	// Normalize the parallel vector to get a unit direction vector
	VectorNormalize(parallelVector);
}

// positions the picked-up entity in-front of the player, constantly checking for obstructions
void V_PickUpEntity(edict_t* ent)
{
	float	dist;
	edict_t* e = ent, * other = ent->client->pickup;
	vec3_t forward, right, offset, start, org;
	trace_t tr;

	if (!G_EntIsAlive(ent) || !ent->client)
	{
		//gi.dprintf("V_PickUpEntity aborted: ent isn't alive\n");
		return;
	}
	// is the pickup entity still valid?
	if (!other || !other->inuse || other->solid == SOLID_NOT || (other->max_health && other->health < 1))
	{
		//gi.dprintf("V_PickUpEntity aborted: pickup entity is dead\n");
		// drop it
		ent->client->pickup = NULL;
		return;
	}
	// is this a player-tank?
	if (PM_PlayerHasMonster(ent))
	{
		//gi.dprintf("V_PickUpEntity: player tank\n");
		e = ent->owner;
	}

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	// calculate point in-front of us, leaving just enough room so that our bounding boxes don't collide
	dist = e->maxs[0] + other->maxs[0] + 0.1 * VectorLength(ent->velocity) + 8;
	VectorMA(start, dist, forward, start);
	// begin trace at calling entity's origin, adjusted to fit the height of the entity we're carrying
	VectorCopy(e->s.origin, org);
	org[2] = e->absmin[2] + fabs(other->mins[2]) + 1;
	// check for obstructions at our starting position - this will cover situations where the player can squeeze under a staircase
	tr = gi.trace(org, other->mins, other->maxs, org, other, MASK_SHOT);
	if (tr.allsolid || tr.startsolid || tr.fraction < 1)
	{
		dist = other->maxs[0];
		// attempt to reposition the pickup entity so its origin is clear of obstructions
		if (!V_PushBackWalls(other, org, dist, MASK_MONSTERSOLID, true))
		{
			//gi.dprintf("obstruction at start: %s at %.0f distance and %.0f height fraction %.1f allsolid %d startsolid %d\n", tr.ent->classname, dist, tr.endpos[2], tr.fraction, tr.allsolid, tr.startsolid);
			return;
		}
		VectorCopy(other->s.origin, org);
		// check for obstructions between our starting and ending positions
		tr = gi.trace(org, NULL, NULL, start, other, MASK_SHOT);
		if (tr.fraction < 1)
		{
			//gi.dprintf("clipped wall\n");
			VectorCopy(tr.endpos, start);
		}
		else
		{
			//gi.dprintf("clipped nothing\n");
		}
		if (!V_GetCorrectedOrigin(other, start, dist, MASK_MONSTERSOLID, start, true))
		{
			//gi.dprintf("couldn't correct ending position\n");
			//return;
		}

	}
	// check for obstructions between our starting and ending positions
	tr = gi.trace(org, other->mins, other->maxs, start, other, MASK_SHOT);
	if (tr.allsolid)// || tr.fraction < 1) 
	{ //FIXME the tr.startsolid check causes the entity to get stuck on walls :(
		// the issue is that entities with bboxes bigger than the player cause this problem
		// might need to do a little more work to adjust the position of the entity to account for picked up ents larger than the player
		//gi.dprintf("obstruction between: %s at %.0f distance and %.0f height fraction %.1f allsolid %d startsolid %d\n", tr.ent->classname, dist, tr.endpos[2], tr.fraction, tr.allsolid, tr.startsolid);
		//ent->client->pickup = NULL;
		return;
	}
	//DEBUG
	//if (tr.fraction < 1)
	//{
		//gi.dprintf("allowed obstruction %s at %.0f distance and %.0f height fraction %.1f allsolid %d startsolid %d\n", tr.ent->classname, dist, tr.endpos[2], tr.fraction, tr.allsolid, tr.startsolid);
	//}
	// move into position
	VectorCopy(tr.endpos, other->s.origin);
	VectorCopy(tr.endpos, other->s.old_origin);
	gi.linkentity(other);
	VectorClear(other->velocity);
}

// move entity into position in front of the player, remove the entity and return false if we can't
qboolean vrx_position_player_summonable(edict_t* ent, edict_t* other, float dist)
{
	vec3_t forward, right, start, offset;

	// get view origin
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	//FIXME: this function modifies start and returns the tr.endpos, so the next bit of code may push the starting position into a wall!
	if (!G_GetSpawnLocation(ent, dist, other->mins, other->maxs, start, NULL, PROJECT_HITBOX_FAR, false))
	{
		G_FreeEdict(other);
		return false;
	}

	// move entity into position
	VectorCopy(start, other->s.origin);
	VectorCopy(start, other->s.old_origin);
	//VectorMA(start, 48, forward, other->s.origin);
	gi.linkentity(other);

	VectorCopy(ent->s.angles, other->s.angles);
	other->s.angles[PITCH] = 0;
	other->s.angles[ROLL] = 0;
	return true;
}

// return true if there isn't a previously dropped entity that would prevent us from dropping another
qboolean CanDropPickupEnt(edict_t* ent)
{
	edict_t* pickup_prev = ent->client->pickup_prev;
	// don't have a pickup entity to drop
	if (!ent->client->pickup || !ent->client->pickup->inuse)
		return false;
	// previous solid pickup entity might get in the way
	if (pickup_prev && pickup_prev->inuse && pickup_prev->solid == SOLID_BBOX 
		&& pickup_prev->owner && pickup_prev->owner->inuse && pickup_prev->owner == ent)
		return false;
	return true;

}

// sets owner when this entity is picked up, making it non-solid to the player
void vrx_set_pickup_owner(edict_t* self)
{
	edict_t	*cl = NULL, *owner = NULL;

	// get preferred owner of barrel
	cl = owner = G_GetClient(self);

	// entity is owned by a non-client (e.g. world-spawned medic)
	if (!cl)
	{
		if (self->owner)
			self->owner = NULL;
		return;
	}

	// if the player is piloting another entity (e.g. morphed tank), set owner to the other entity
	if (PM_PlayerHasMonster(owner))
		owner = owner->owner;

	// is a player holding this barrel?
	if (cl->client && cl->client->pickup && cl->client->pickup == self)
	{
		// if owner isn't set or invalid, set it to the preferred owner
		// this will make barrel non-solid to the player (or player-monster)
		if (!self->owner || !self->owner->inuse || self->owner != owner)
		{
			self->owner = owner;
			self->flags |= FL_PICKUP;
			if (self->movetype == MOVETYPE_NONE)
				self->movetype = MOVETYPE_STEP; // detach entity from ceilings and other surfaces
		}
	}
	// player is no longer holding barrel--is owner set?
	else if (self->owner)
	{
		//gi.dprintf("let go!\n");
		// need to make this null first so that the trace works
		self->owner = NULL;
		// barrel isn't being held, so make it solid again to player if it's clear of obstructions
		trace_t tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_PLAYERSOLID);
		//FIXME: the owner won't be cleared if the player managed to stick the barrel in a bad spot
		if (tr.allsolid || tr.startsolid || tr.fraction < 1)
		{
			//gi.dprintf("bad spot!\n");
			self->owner = owner;
		}
		else
			self->flags &= ~FL_PICKUP;
	}
}

// clears player's pointer to picked-up entity
void vrx_clear_pickup_ent(gclient_t* player, edict_t *other)
{
	// stop tracking this previously picked up entity
	if (player->pickup_prev && player->pickup_prev->inuse && player->pickup_prev == other)
		player->pickup_prev = NULL;
	// drop it
	if (player->pickup && player->pickup->inuse && player->pickup == other)
		player->pickup = NULL;
}

// search for entity to pick up, or drop the one we're holding
// return false if there is nothing to do, otherwise return true
qboolean vrx_toggle_pickup(edict_t* ent, int mtype, float dist)
{
	edict_t* e = NULL;
	vec3_t forward;

	// already picked up an entity?
	if (ent->client->pickup && ent->client->pickup->inuse)
	{
		//gi.dprintf("have pickup\n");
		if (CanDropPickupEnt(ent))//FIXME: entities can clip on nearby walls, maybe do a final trace here before we allow it to drop?
		{
			//gi.dprintf("dropping it\n");
			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			// toss it forward if it's airborne
			if (!ent->client->pickup->groundentity)
			{
				VectorScale(forward, 400, ent->client->pickup->velocity);
				ent->client->pickup->velocity[2] += 200;
			}
			// save a pointer to dropped entity
			ent->client->pickup_prev = ent->client->pickup;
			// let go
			ent->client->pickup = NULL;
		}
		//else
			//gi.dprintf("can't drop it\n");
		return true;
	}
	//gi.dprintf("no pickup\n");

	// find an entity close to the player's aiming reticle
	while ((e = findclosestreticle(e, ent, dist)) != NULL)
	{
		edict_t* e_owner = NULL;
		//if (!G_EntIsAlive(e))
		//	continue;
		if (e && e->inuse && e->solid == SOLID_NOT && !e->deadflag)
		{
			//if (e && e->inuse)
			//	gi.dprintf("rejected %s\n", e->classname);
			continue;
		}
		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;
		e_owner = G_GetClient(e);
		if (!e_owner || e_owner != ent)
			continue;
		if (e->mtype != mtype)
			continue;
		// found one--pick it up!
		ent->client->pickup = e;
		// clear pickup_prev if we've picked up the same entity we previously dropped
		if (ent->client->pickup_prev && ent->client->pickup_prev->inuse && ent->client->pickup_prev == e)
			ent->client->pickup_prev = NULL;
		//gi.dprintf("found pickup\n");
		return true;
	}
	return false; // nothing to drop, nothing to pick up either
}

void vrx_stun(edict_t* self, edict_t* other, float time)
{
	// force monsters into idle mode
	if (!strcmp(other->classname, "drone"))
	{
		// bosses can't be stunned easily
		if (other->monsterinfo.control_cost >= M_COMMANDER_CONTROL_COST || other->monsterinfo.bonus_flags & BF_UNIQUE_FIRE
			|| other->monsterinfo.bonus_flags & BF_UNIQUE_LIGHTNING)
			time *= 0.2;

		other->empeffect_time = level.time + time;
		other->empeffect_owner = self->owner;
		other->monsterinfo.pausetime = level.time + time;
		other->monsterinfo.stand(other);
	}
	// stun anything else
	else
	{
		other->empeffect_time = level.time + time;
		other->holdtime = level.time + time;
	}
}

// returns false if a non-essential entity (e.g. visual effects, gibs) should be spawned at org
qboolean vrx_spawn_nonessential_ent(vec3_t org)
{
	int ents = 0; // counter for ents being sent to clients at org

	for (edict_t* e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		// freed entities don't (or shouldn't ) count
		if (!e->inuse)
			continue;
		// entities with this flag aren't sent to clients
		if (e->svflags & SVF_NOCLIENT)
			continue;
		// entities outside of current PVS shouldn't be sent to clients
		// note: entities outside the map or in a solid will always fail this check
		if (!gi.inPVS(org, e->s.origin))
			continue;
		// other possible checks could include visiblity and range (>1024 units)
		//if (distance(org, e->s.origin) > rad)
		//	continue;
		//if (is_visible && !G_IsClearPath(NULL, MASK_SOLID, org, e->s.origin))
		//	continue;
		ents++;
	}

	//gi.dprintf("%s: %d entities are potentially being sent to clients\n", __func__, ents);
	//if (!ents)
		//gi.dprintf("%s: WARNING: org is in a solid or outside the map!\n");
	if (ents && ents < NEARBY_ENTITIES_MAX)
		return true;
	//gi.dprintf("don't spawn non-essential entity!\n");
	return false;
}