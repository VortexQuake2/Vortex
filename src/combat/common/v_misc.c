#include "g_local.h"
#include <gamemodes/invasion.h>

int vrx_get_joined_players() {
	edict_t* player;
	int i, clients = 0;

	for (i = 1; i <= maxclients->value; i++) {
		player = &g_edicts[i];

		if (!player->inuse)
			continue;
		if (G_IsSpectator(player))
			continue;
		if (player->ai.is_bot)
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

		if (player->ai.is_bot) // az: heheh
			continue;

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
	int players = vrx_get_joined_players();
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
        monster->think = INV_SpawnMonsters;
        monster->notify_drone_death = INV_NotifyMonsterDeath;
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

void organ_remove(edict_t* self, qboolean refund);

void organ_removeall(edict_t* self, char* classname, qboolean refund);

void RemoveMiniSentries(edict_t* ent);

void RemoveAllLaserPlatforms(edict_t* ent);//4.5
void holyground_remove(edict_t* ent, edict_t* self);

void depot_remove(edict_t* self, edict_t* owner, qboolean effect);

void lasertrap_removeall(edict_t* ent, qboolean effect);

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

	if (other->takedamage)
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
}
