#include "g_local.h"
#include "invasion.h"

//FIXME: need queue that holds all players that are waiting to respawn but all spawns are busy
edict_t		*INV_SpawnQue[MAX_CLIENTS];
int			invasion_max_playerspawns;
int			invasion_spawncount; // current spawns
int			invasion_monsterspawns;
int			invasion_navicount;
int			invasion_start_navicount;
edict_t		*INV_PlayerSpawns[64];
edict_t		*INV_Navi[64];
edict_t		*INV_StartNavi[64];

struct invdata_s invasion_data;

/* reference vrx_create_drone_from_ent on drone_misc.c */
// all monsters except medic
static const int SET_EASY_MODE_MONSTERS[] = {
	1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14
};
const int SET_EASY_MODE_MONSTERS_COUNT = sizeof(SET_EASY_MODE_MONSTERS) / sizeof(int);

// parasite, brain, medic, tank, mutant, gladiator, berserker, infantry, hover
static const int SET_HARD_MODE_MONSTERS[] = {
	2, 4, 5, 6, 7, 8, 9, 11, 14
};
const int SET_HARD_MODE_MONSTERS_COUNT = sizeof(SET_HARD_MODE_MONSTERS) / sizeof(int);

static const int SET_FLYING_MONSTERS[] = {
	12, 13, 14
};
const int SET_FLYING_MONSTERS_COUNT = sizeof(SET_FLYING_MONSTERS) / sizeof(int);

// parasite, brain, mutant, berserker
static const int SET_MELEE_MONSTERS[] = {
	2, 4, 7, 9
};
const int SET_MELEE_MONSTERS_COUNT = sizeof(SET_MELEE_MONSTERS) / sizeof(int);

// gladiator, berserker
static const int SET_RAGEQUIT_MONSTERS[] = {
	8, 9
};
const int SET_RAGEQUIT_MONSTERS_COUNT = sizeof(SET_RAGEQUIT_MONSTERS) / sizeof(int);

// tank, mutant, berserker
static const int SET_TANKY_MONSTERS[] = {
	6, 7, 9
};
const int SET_TANKY_MONSTERS_COUNT = sizeof(SET_TANKY_MONSTERS) / sizeof(int);

void INV_Init(void)
{
	int i;
	if (!pvm->value || !invasion->value)
		return;

	INV_InitSpawnQue();
	INVASION_OTHERSPAWNS_REMOVED = false;
	invasion_difficulty_level = 1;
	invasion_max_playerspawns = 0;
	invasion_spawncount = 0;
	invasion_navicount = 0;
	invasion_start_navicount = 0;

	for (i = 0; i < MAX_CLIENTS; i++)   // down from 32 to 24
		INV_PlayerSpawns[i] = NULL;

	for (i = 0; i < 64; i++) {
		INV_Navi[i] = NULL;
		INV_StartNavi[i] = NULL;
	}
}

// Ran after edicts have been initialized
void INV_InitPostEntities(void)
{
	if (!invasion->value)
		return;
	
	// find all navis without a navi pre-targeting them.
	// they are potential start navis
	for (int i = 0; i < invasion_navicount; i++)
	{
		edict_t* self = INV_Navi[i];
		edict_t* target;

		if (self->target) {
			target = G_Find(NULL, FOFS(targetname), self->target);
			if (target) {
                target->prev_navi = self;
                self->target_ent = target;
            }
		}
	}

	for (int i = 0; i < invasion_navicount; i++)
	{
		edict_t* self = INV_Navi[i];
		if (self->prev_navi == NULL) {
			INV_StartNavi[invasion_start_navicount++] = self;
		}
	}

	//gi.dprintf("invasion: found %d start navis to choose from. total: %d\n", invasion_start_navicount, invasion_navicount);
	gi.dprintf("Invasion: %d start navis, %d total. %d player spawns, %d monster spawns.\n", invasion_start_navicount, invasion_navicount, invasion_spawncount, invasion_monsterspawns);
}

// initialize array values to NULL
void INV_InitSpawnQue(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		INV_SpawnQue[i] = NULL;
}

edict_t *INV_GiveClosestPSpawn(edict_t *ent)
{
	float bestdist = 8192 * 8192; // using dist. sqr.
	vec3_t eorg;
	edict_t *ret = NULL;
	
	for (int i = 0; i < invasion_spawncount; i++)
	{
		float dist;
		edict_t* spawn = INV_PlayerSpawns[i];
		if (!spawn) continue;
		VectorSubtract(ent->s.origin, spawn->s.origin, eorg);
		dist = VectorLengthSqr(eorg);

		if (dist < bestdist) {
			bestdist = dist;
			ret = spawn;
		}
	}

	return ret; // can be null if no pspawns
}

edict_t *INV_GiveRandomPSpawn()
{
	if (invasion_spawncount > 1)
	{
		// pick a random active spawn
		int rand = GetRandom(1, invasion_spawncount) - 1;
		return INV_PlayerSpawns[rand];
	}
	else if (invasion_spawncount == 1)
	{
		return INV_PlayerSpawns[0];
	}

	return NULL;
}

//FIXME: should we do a visibility check?
// this function returns the closest START navi, i.e. a navi at the start of a chain
edict_t* INV_ClosestNavi(edict_t* self)
{
	vec3_t eorg;
	float best = 8192 * 8192;
	edict_t *ret = NULL;
	for (int i = 0; i < invasion_start_navicount; i++)
	{
		float len;
		VectorSubtract(self->s.origin, INV_StartNavi[i]->s.origin, eorg);
		len = VectorLengthSqr(eorg);
		if (len < best)
		{
			best = len;
			ret = INV_StartNavi[i];
		}
	}

	return ret;
}

// returns the nearest navi
edict_t* INV_ClosestNaviAny(edict_t* self) {
    vec3_t eorg;
    float best = 8192 * 8192;
    edict_t *ret = NULL;
    for (int i = 0; i < invasion_navicount; i++)
    {
        float len;
        VectorSubtract(self->s.origin, INV_Navi[i]->s.origin, eorg);
        len = VectorLengthSqr(eorg);
        if (len < best)
        {
            best = len;
            ret = INV_Navi[i];
        }
    }

    return ret;
}

void DrawNavi(edict_t* ent)
{
	float		dist, flrht;
	edict_t*	navi=NULL;
	vec3_t		start, end;
	trace_t		tr;

	if (((navi = INV_ClosestNaviAny(ent)) != NULL) && navi->target_ent)
	{
		VectorCopy(navi->s.origin, start);
		VectorCopy(navi->target_ent->s.origin, end);
		dist = distance(start, end);
		// spawn bfg laser trails between two chained navis
		G_Spawn_Trails(TE_BFG_LASER, start, end);

		// calculate distance from floor - start node
		VectorCopy(start, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);
		flrht = start[2] - tr.endpos[2];

		if (!(level.framenum % 20))
			safe_centerprintf(ent, "Navi %s --> navi %s, height: %.0f distance: %.0f\n", navi->targetname, navi->target, flrht, distance);
	}
}

edict_t *drone_findnavi(edict_t *self)
{
#if 0
	edict_t* visible_candidates[4];
	int num_visible_candidates = 0;
#endif

	if (G_GetClient(self)) // Client monsters don't look for navis
		return NULL;
// GHz FIX - commented out the if clause below, as we want monsters to find a path to the player spawns
// because since 4.63 we can use A* pathfinding to get a path rather than relying on INVASION_NAVI/navi_monster_invasion entities
// if we still want to use the navis, then we need to use the target property of the map entity to find the next navi in the chain
	if (!(self->monsterinfo.aiflags & AI_FIND_NAVI)) {
		if (!invasion->value)
			return NULL;
		// if we reached this point we're no longer looking for navis. we should be looking for spawns!
		return INV_GiveClosestPSpawn(self); 
	}

	/*if (invasion->value && level.pathfinding)
	{
		return INV_GiveClosestPSpawn(self);
	}*/

#if 0
	// seek up to 4 visible candidates
	for (int i = 0; i < invasion_start_navicount; i++)
	{
		if (visible(self, INV_StartNavi[i]))
		{
			visible_candidates[num_visible_candidates++] = INV_StartNavi[i];
			if (num_visible_candidates == 4) break;
		}
	}

	// if there's visible candidates, use them
	if (num_visible_candidates > 0) {
		int i = GetRandom(1, num_visible_candidates) - 1;
		return visible_candidates[i];
	}

	// there aren't, then use any start navi
	if (invasion_start_navicount > 0)
		return INV_StartNavi[GetRandom(1, invasion_start_navicount) - 1];
#endif

	if (invasion_start_navicount > 0) {
		return INV_ClosestNaviAny(self);// INV_ClosestNavi(self);
	}
	
	// no navis, return a player spawn...
	return INV_GiveRandomPSpawn();
}


qboolean INV_IsSpawnQueEmpty()
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (INV_SpawnQue[i] != NULL)
			return false;
	}
	return true;
}

qboolean INV_InSpawnQue(edict_t *ent)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (INV_SpawnQue[i] && INV_SpawnQue[i] == ent)
			return true;
	return false;
}

// add player to the spawn queue
qboolean INV_AddSpawnQue(edict_t *ent)
{
	int i;

	if (!ent || !ent->inuse || !ent->client)
		return false;

	// don't add them if they are already in the queue
	if (INV_InSpawnQue(ent))
		return false;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!INV_SpawnQue[i])
		{
			INV_SpawnQue[i] = ent;
			//gi.dprintf("added %s to list\n", ent->client->pers.netname);
			return true;
		}
	}
	return false;
}

// remove player from the queue
qboolean INV_RemoveSpawnQue(edict_t *ent)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (INV_SpawnQue[i] && (INV_SpawnQue[i] == ent))
		{
			//gi.dprintf("removed %s from list\n", ent->client->pers.netname);
			INV_SpawnQue[i] = NULL;
			return true;
		}
	}
	return false;
}

// return player that is waiting to respawn
edict_t *INV_GetSpawnPlayer(void)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (INV_SpawnQue[i] && INV_SpawnQue[i]->inuse && INV_SpawnQue[i]->client)
		{
			//gi.dprintf("found %s in list\n", INV_SpawnQue[i]->client->pers.netname);
			return INV_SpawnQue[i];
		}
	}
	return NULL;
}

edict_t *INV_GetMonsterSpawn(edict_t *from)
{
	if (from)
		from++;
	else
		from = g_edicts;

	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (from && from->inuse && (from->mtype == INVASION_MONSTERSPAWN)
			&& (level.time > from->wait))
			return from;
	}

	return NULL;
}

void INV_AwardPlayers(void)
{
	int		i, points, credits, num_spawns = INV_GetNumPlayerSpawns(), num_winners = 0;
	edict_t *player;

	// we're not in invasion mode
	if (!INVASION_OTHERSPAWNS_REMOVED)
		return;

	// if map didn't end normally, don't award points
	if (level.time < timelimit->value * 60)
		return;

	// no award if the humans were unable to defend their spawns
	if (num_spawns < 1)
		return;

	for (i = 0; i<game.maxclients; i++)
	{
		player = g_edicts + 1 + i;
		if (!player->inuse)
			continue;

		if (invasion->value == 2)
			points = 2.0f / 5.0f * player->client->resp.score*((float)num_spawns / invasion_max_playerspawns) + 300 * sqrt(invasion_difficulty_level);
		else
			points = 1.5f / 10.0f * player->client->resp.score*((float)num_spawns / invasion_max_playerspawns);

		if (invasion->value == 1 && points > INVASION_BONUS_EXP)
			points = INVASION_BONUS_EXP;
		//points = INVASION_BONUS_EXP*((float)num_spawns/invasion_max_playerspawns);
		if (invasion->value < 2)
			credits = INVASION_BONUS_CREDITS*((float)num_spawns / invasion_max_playerspawns);
		else
			credits = 2.0f / 5.0f * INVASION_BONUS_CREDITS*((float)num_spawns / invasion_max_playerspawns) + 300 * sqrt(invasion_difficulty_level);

		//	gi.dprintf("points=%d credits=%d spawns=%d max=%d\n", 
		//		points, credits, num_spawns, invasion_max_playerspawns);

		if (!G_IsSpectator(player))
		{
			int fexp = vrx_apply_experience(player, points);
			player->myskills.credits += credits;
			safe_cprintf(player, PRINT_MEDIUM, "Earned %d exp and %d credits!\n", fexp, credits);

			if (player->client && player->client->pers.score) // we've been here for a while at least
				num_winners++;
		}
	}

	if (num_winners)
		gi.bprintf(PRINT_HIGH, "Humans win! Players were awarded a bonus.\n");
}

edict_t* INV_SpawnDrone(edict_t* self, edict_t *spawn_point, int index)
{
	edict_t *monster;
	vec3_t	start;
	trace_t	tr;
	int mhealth = 1;

	if (!(monster = vrx_create_new_drone(self, index, true, false)))
	{
		//gi.dprintf("INV_SpawnDrone() failed to create a new type %d drone\n", index);
		return NULL;
	}

	// calculate starting position
	VectorCopy(spawn_point->s.origin, start);
	start[2] = spawn_point->absmax[2] + 1 + fabsf(monster->mins[2]);

	tr = gi.trace(start, monster->mins, monster->maxs, start, NULL, MASK_SHOT);

	// starting point is occupied
	if (tr.fraction < 1)
	{
		// is this an entity (monster) that we own?
		if (tr.ent && tr.ent->inuse && tr.ent->activator && tr.ent->activator->inuse && (tr.ent->activator == self))
		{
			//gi.dprintf("another monster occupies this space\n");
			// if we're trying to spawn a non-boss monster, remove the monster and try again later
			if (index < 30)
			{
				M_Remove(monster, false, false);
				return NULL;
			}
			else
			{
				//gi.dprintf("tried to make a boss and remove another ent\n");
				// if we are trying to spawn a boss, remove the monster currently occupying this space
				M_Remove(tr.ent, false, false);
			}
		}
		else
		{
			//gi.dprintf("spawn area occupied by something else\n");
			// we've hit something else, remove the monster and try again later
			M_Remove(monster, false, false);
			return NULL;
		}
	}

	spawn_point->wait = level.time + 1.0; // time until spawn is available again

	monster->monsterinfo.aiflags |= AI_FIND_NAVI; // search for navi
	monster->s.angles[YAW] = spawn_point->s.angles[YAW];
	monster->prev_navi = NULL;


	// az: we modify the monsters' health lightly
	if (invasion->value == 1) // easy mode
	{
		if (invasion_difficulty_level < 5)
			mhealth = 1;
		else if (invasion_difficulty_level >= 5)
			mhealth = 1 + 0.05 * log2(invasion_difficulty_level - 4);
	}
	else if (invasion->value == 2) // hard mode
	{
		float plog = log2(vrx_get_joined_players() + 1) / log2(4);
		mhealth = 1 + 0.1 * invasion_difficulty_level * max(plog, 0);
	}

	monster->max_health = monster->health = monster->max_health*mhealth;

	// move the monster onto the spawn pad if it's not a boss
	//if (index < 30)
	//{
		VectorCopy(start, monster->s.origin);
		VectorCopy(start, monster->s.old_origin);
	//}

	monster->s.event = EV_OTHER_TELEPORT;

	// az: non-bosses get quad/inven for preventing spawn camp
	// bosses don't get it because it makes it inevitable for the players
	// to lose spawns sometimes.
	if (index < 30) {
		if (spawn_point->count)
			monster->monsterinfo.inv_framenum = level.framenum + spawn_point->count;
		else
		{
			if (invasion->value == 1)
				monster->monsterinfo.inv_framenum = level.framenum + (int)(6 / FRAMETIME); // give them quad/invuln to prevent spawn-camping
			else if (invasion->value == 2)
				monster->monsterinfo.inv_framenum = level.framenum + (int)(8 / FRAMETIME); // Hard mode invin
		}
	} else {
        // remove any existing invuln. from boss
        monster->monsterinfo.inv_framenum = level.framenum - 1;
	}

	gi.linkentity(monster);
	return monster;
}

float TimeFormula()
{
	int base = 4 * 60;
	int playeramt = vrx_get_alive_players() * 8;
	int levelamt = invasion_difficulty_level * 7;
	int cap = 60;
	int rval = base - playeramt - levelamt;

	if (invasion->value == 2) // hard mode
		cap = 52;

	if (rval < cap)
		rval = cap;

	return rval;
}

/*
// we'll override the other die functino to set our boss pointer to NULL.
void mytank_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void makron_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);

void invasion_boss_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (self->mtype == M_COMMANDER)
		mytank_die(self, inflictor, attacker, damage, point);
	else if (self->mtype == M_MAKRON)
		makron_die(self, inflictor, attacker, damage, point);
	invasion_data.boss = NULL;
}
*/

void INV_SpawnBoss(edict_t* self, int index)
{
	edict_t* spawn=NULL;

	if (invasion_data.boss)
		return;
	if (index < 30)
		return;

	while (spawn = INV_GetMonsterSpawn(spawn))
	{
		if (!(invasion_data.boss = INV_SpawnDrone(self, spawn, index)))
			continue;
		else
		{
			total_monsters++;
			G_PrintGreenText(va("A level %d %s has spawned!", invasion_data.boss->monsterinfo.level, V_GetMonsterName(invasion_data.boss)));
			return;
		}
	}
	gi.dprintf("Unable to spawn boss.\n");
}

void INV_BossCheck(edict_t *self)
{
	edict_t *e = NULL;

	if (!(invasion_difficulty_level % 5))// every 5 levels, spawn a boss
	{
		int bcount = 0;
		int iter = 0;
		while (((e = INV_GetMonsterSpawn(e)) != NULL) && (bcount < 1))
		{
			if (!invasion_data.boss)
			{
				if (!(invasion_data.boss = INV_SpawnDrone(self, e, GetRandom(30,32))))
				{
					iter++;

					if (iter < 256) // 256 tries before quitting with spawning this boss
						continue;
					else
						break;
				}
				bcount++;
				total_monsters++;
				//invasion_data.boss->die = invasion_boss_die;
				G_PrintGreenText(va("A level %d %s has spawned!", invasion_data.boss->monsterinfo.level, V_GetMonsterName(invasion_data.boss)));
				break;
			}
		}
	}
	else
		invasion_data.boss = NULL;
}

void INV_OnTimeout(edict_t *self) {
	qboolean was_boss = false;
	gi.bprintf(PRINT_HIGH, "Time's up!\n");
	if (invasion_data.boss && invasion_data.boss->deadflag != DEAD_DEAD) // out of time for the boss.
	{
		G_PrintGreenText(va("You failed to kill the %s soon enough!", V_GetMonsterName(invasion_data.boss)));
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BOSSTPORT);
		gi.WritePosition(invasion_data.boss->s.origin);
		gi.multicast(invasion_data.boss->s.origin, MULTICAST_PVS);
		//DroneList_Remove(invasion_data.boss); // az: WHY DID I FORGET THIS
		//G_FreeEdict(invasion_data.boss);
		invasion_data.boss = NULL;
		was_boss = true;
	}
	// remove monsters from the current wave before spawning the next
	if (self->num_monsters_real)
		PVM_RemoveAllMonsters(self);
	// restart the last wave
	if (!was_boss)
		invasion_difficulty_level -= 1;

	// increase the difficulty level for the next wave
	//if (invasion->value == 1)
	//	invasion_difficulty_level += 1;
	//else
	//	invasion_difficulty_level += 2; // Hard mode.
	invasion_data.printedmessage = 0;
	gi.sound(&g_edicts[0], CHAN_VOICE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);

	// start spawning
	self->nextthink = level.time + FRAMETIME;
	self->count = MONSTERSPAWN_STATUS_WORKING;
}

void INV_ShowLastWaveSummary() {
	// print exp summaries
	for (int i = 1; i <= maxclients->value; i++) {
		edict_t *player = &g_edicts[i];

		if (!player->inuse || G_IsSpectator(player))
			continue;

		if ( ( player->client->resp.wave_solo_exp < 1 ) &&
				( player->client->resp.wave_shared_exp < 1 ) &&
				( player->client->resp.wave_assist_exp < 1 ) ) {

			continue;
		}

		safe_cprintf(player, PRINT_MEDIUM, "Wave summary:\n" );

		if ( player->client->resp.wave_solo_exp > 0 ) {
			safe_cprintf(player, PRINT_MEDIUM, "  %d exp and %d credits from %d damage dealt to %d monsters\n",
							player->client->resp.wave_solo_exp,
							player->client->resp.wave_solo_credits,
							player->client->resp.wave_solo_dmgmod,
							player->client->resp.wave_solo_targets );
		}

		if ( player->client->resp.wave_assist_exp > 0 ) {
			safe_cprintf(player, PRINT_MEDIUM, "  %d exp and %d credits from assisting your team\n",
							player->client->resp.wave_assist_exp,
							player->client->resp.wave_assist_credits );
		}

		if ( player->client->resp.wave_shared_exp > 0 ) {
			safe_cprintf(player, PRINT_MEDIUM, "  %d exp and %d credits shared from your team\n",
							player->client->resp.wave_shared_exp,
							player->client->resp.wave_shared_credits );
		}

		player->client->resp.wave_solo_targets = 0;
		player->client->resp.wave_solo_dmgmod = 0;
		player->client->resp.wave_solo_exp = 0;
		player->client->resp.wave_solo_credits = 0;
		player->client->resp.wave_shared_exp = 0;
		player->client->resp.wave_shared_credits = 0;
		player->client->resp.wave_assist_exp = 0;
		player->client->resp.wave_assist_credits = 0;
	}
}

void INV_OnBeginWave(edict_t *self, int max_monsters) {
	if (invasion_difficulty_level == 1)
	{
		if (invasion->value == 1)
			gi.bprintf(PRINT_HIGH, "The invasion begins!\n");
		else
			gi.bprintf(PRINT_HIGH, "The invasion... begins.\n");
	} else {
		INV_ShowLastWaveSummary();
	}

	if (invasion_difficulty_level % 5)
		gi.bprintf(PRINT_HIGH, "Welcome to level %d. %d monsters incoming!\n", invasion_difficulty_level, max_monsters);
	else
		gi.bprintf(PRINT_HIGH, "Welcome to level %d.\n", invasion_difficulty_level, max_monsters);
	G_PrintGreenText(va("Timelimit: %dm %ds.\n", (int)TimeFormula() / 60, (int)TimeFormula() % 60));

	gi.sound(&g_edicts[0], CHAN_VOICE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);

	// check for a boss spawn
	INV_BossCheck(self);
}

void INV_SelectMonsterSet(const edict_t* self, int const * * monster_set, int* monster_set_count)
{
	int const **default_monster_set = &invasion_data.default_monster_set;
	int* default_monster_set_count = &invasion_data.default_set_count;

	//*monster_set = SET_EASY_MODE_MONSTERS;
	//*monster_set_count = SET_EASY_MODE_MONSTERS_COUNT;

	//****FIXME: either abort when boss spawns (since nothing else spawns), or don't print anything! ****
	// ALSO: fire baron will stay at range spamming meteor at a target, uninterrupted, rather than trying to close distance
	// set default - this is done at the start of a wave before checking individual spawnpoint values
	if (!self)
	{
		if (random() <= 0.5)// chance to spawn special wave
		{
			switch (GetRandom(1, 4))
			{
			case 1:
				*default_monster_set = SET_FLYING_MONSTERS;
				*default_monster_set_count = SET_FLYING_MONSTERS_COUNT;
				gi.bprintf(PRINT_HIGH, "Flying invaders are coming for you!\n");
				break;
			case 2:
				*default_monster_set = SET_MELEE_MONSTERS;
				*default_monster_set_count = SET_MELEE_MONSTERS_COUNT;
				gi.bprintf(PRINT_HIGH, "Prepare bayonets! The invaders are about to get up close and personal!\n");
				break;
			case 3:
				*default_monster_set = SET_RAGEQUIT_MONSTERS;
				*default_monster_set_count = SET_RAGEQUIT_MONSTERS_COUNT;
				gi.bprintf(PRINT_HIGH, "It's time to rage quit!\n");
				break;
			case 4:
				*default_monster_set = SET_TANKY_MONSTERS;
				*default_monster_set_count = SET_TANKY_MONSTERS_COUNT;
				gi.bprintf(PRINT_HIGH, "The heavyweights are coming for your base!\n");
				break;
			}
		}
		else if (invasion->value == 2)// hard mode
		{
			*default_monster_set = SET_HARD_MODE_MONSTERS;
			*default_monster_set_count = SET_HARD_MODE_MONSTERS_COUNT;
		}
		else// easy mode
		{
			*default_monster_set = SET_EASY_MODE_MONSTERS;
			*default_monster_set_count = SET_EASY_MODE_MONSTERS_COUNT;
		}
		return;
	}
	/*
	 * style 1 forces flying monsters
	 * style 2 forces easy mode monsters (no medic)
	 * style 3 forces hard mode monsters (include medic)
	 */

	// modify monster set based on map values (bypasses defaults)
	switch (self->style)
	{
	case 1:
		*monster_set = SET_FLYING_MONSTERS;
		*monster_set_count = SET_FLYING_MONSTERS_COUNT;
		break;
	case 2:
		*monster_set = SET_EASY_MODE_MONSTERS;
		*monster_set_count = SET_EASY_MODE_MONSTERS_COUNT;
		break;
	case 3:
		*monster_set = SET_HARD_MODE_MONSTERS;
		*monster_set_count = SET_HARD_MODE_MONSTERS_COUNT;
		break;
	default:
		*monster_set = *default_monster_set;
		*monster_set_count = *default_monster_set_count;
		break;
	}
}

void INV_SpawnMonsters(edict_t *self)
{
	const int players = vrx_get_joined_players();

	// How many monsters should we spawn?
	// 10 at lv 1, 30 by level 20. 41 by level 100
	int max_monsters = (int)round(10 + 4.6276 * log2f((float)invasion_difficulty_level));

	edict_t *e = NULL;
	int SpawnTries = 0, MaxTriesThisFrame = 32;

	// get the value of all of our monsters (BF flag mult * level * control_cost)
	// max_monsters_value = PVM_TotalMonstersValue(self);
	// update our drone count
	PVM_TotalMonsters(self, true);
	
	//max_monsters = 1;//GHz DEBUG - REMOVE ME!
	//self->nextthink = level.time + FRAMETIME;//GHz DEBUG - REMOVE ME!
	//return;//GHz DEBUG - REMOVE ME!

	if (!(invasion_difficulty_level % 5))
	{
		if (invasion->value == 1)
			max_monsters = 4 * (vrx_get_joined_players() - 1);
		else if (invasion->value == 2)
			max_monsters = 6 * (vrx_get_joined_players() - 1);
	}

	// Idle State
	// we're done spawning
	if (self->count == MONSTERSPAWN_STATUS_IDLE)
	{
		// if there's nobody playing, remove all monsters
		if (players < 1)
		{
		    // az: reset the current wave to avoid people skipping waves
		    if (self->num_monsters_real) {
                invasion_difficulty_level -= 1;
                invasion_data.printedmessage = false;
                PVM_RemoveAllMonsters(self);
            }
		}
		
		// the level ended, remove monsters
		if (level.intermissiontime)
		{
			if (self->num_monsters_real)
				PVM_RemoveAllMonsters(self);
			return;
		}

		// were all monsters eliminated?
		if (self->num_monsters_real == 0) {
			// start spawning
			self->nextthink = level.time + FRAMETIME;
			self->count = MONSTERSPAWN_STATUS_WORKING;
			return;
		}


		// Check for timeout
		if (invasion_data.limitframe > level.time) // we still got time?
		{
			self->nextthink = level.time + FRAMETIME;
			return;
		}
		else
		{
			// Timeout. We go straight to the working state.
			INV_OnTimeout(self);
			//self->count = MONSTERSPAWN_STATUS_WORKING;
			return;
		}
	}

	// Working State
	//gi.dprintf("%d: level: %d max_monsters: %d\n", (int)(level.framenum), invasion_difficulty_level, max_monsters);
	// if there's nobody playing, then wait until some join
	if (players < 1)
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	// print the message and set the timer the first frame we start working
	if (!invasion_data.printedmessage) {
		invasion_data.limitframe = level.time + TimeFormula();
		INV_OnBeginWave(self, max_monsters);
		invasion_data.printedmessage = true;
		INV_SelectMonsterSet(NULL, &invasion_data.monster_set, &invasion_data.monster_set_count);
	}

    self->nextthink = level.time + FRAMETIME;

	while ((e = INV_GetMonsterSpawn(e)) 
		&& invasion_data.mspawned < max_monsters
		&& SpawnTries < MaxTriesThisFrame)
	{
		//const int* monster_set;
		//int monster_set_count;
		int pick;
		int monster ;

		INV_SelectMonsterSet(e, &invasion_data.monster_set, &invasion_data.monster_set_count);
		pick = GetRandom(1, invasion_data.monster_set_count) - 1;
		monster = invasion_data.monster_set[pick];

		SpawnTries++;
		if (INV_SpawnDrone(self, e, monster)) // Wait for now
			invasion_data.mspawned++;
	}

	if (invasion_data.mspawned >= max_monsters)
	{
		// increase the difficulty level for the next wave
		invasion_difficulty_level += 1;
		invasion_data.printedmessage = 0;
		invasion_data.mspawned = 0;
		self->count = MONSTERSPAWN_STATUS_IDLE;
		//gi.dprintf("invasion level now: %d\n", invasion_difficulty_level);
	}
}

void INV_SpawnPlayers(void)
{
	edict_t *e, *cl_ent;
	vec3_t	start;
	trace_t	tr;

	// we shouldn't be here if this isn't invasion mode
	if (!INVASION_OTHERSPAWNS_REMOVED)
		return;

	// if there are no players waiting to be spawned, then we're done
	if (!(cl_ent = INV_GetSpawnPlayer()))
		return;

	for (int i = 0; i < invasion_spawncount; i++)
	{
		e = INV_PlayerSpawns[i];

		// find an available spawn point
		if (level.time > e->wait)
		{
			// get player starting position
			VectorCopy(e->s.origin, start);
			start[2] = e->absmax[2] + 1 + fabsf(cl_ent->mins[2]);

			tr = gi.trace(start, cl_ent->mins, cl_ent->maxs, start, NULL, MASK_SHOT);

			// don't spawn if another player is standing in the way
			if (tr.ent && tr.ent->inuse && tr.ent->client && tr.ent->deadflag != DEAD_DEAD)
			{
				e->wait = level.time + 1.0;
				continue;
			}

			e->wait = level.time + 2.0; // delay before other players can use this spawn
			cl_ent->spawn = e; // player may use this spawn
			respawn(cl_ent);

			// get another waiting player
			if (!(cl_ent = INV_GetSpawnPlayer()))
				return;
		}
	}
}

edict_t *INV_SelectPlayerSpawnPoint(edict_t *ent)
{
	if (!ent || !ent->inuse)
		return NULL;

	// spectators always get a spawn
	if (G_IsSpectator(ent))
		return INV_GiveRandomPSpawn();

	if (ent->spawn && ent->spawn->inuse)
		return ent->spawn;
	else // We requested a spawn point, but we don't have one. What now?
	{
		// Try to find one. But only if the spawn que is empty.
		if (INV_IsSpawnQueEmpty())
			return INV_GiveRandomPSpawn();
	}

	return NULL;
}

int INV_GetNumPlayerSpawns(void)
{
	return invasion_spawncount;
}

void INV_RemoveFromSpawnlist(edict_t *self)
{
	// az: move the last one to our current slot
	// this works out fine if list_index = invasion_spawncount
	invasion_spawncount--;
	INV_PlayerSpawns[self->list_index] = INV_PlayerSpawns[invasion_spawncount];

	if (INV_PlayerSpawns[self->list_index])
		INV_PlayerSpawns[self->list_index]->list_index = self->list_index;

	INV_PlayerSpawns[invasion_spawncount] = NULL;

	if (invasion_spawncount <= 0)
	{
		gi.bprintf(PRINT_HIGH, "Humans were unable to stop the invasion. Game over.\n");
		EndDMLevel();
		return;
	}

}

void info_player_invasion_death(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_UseTargets(self, self);
	self->think = BecomeExplosion1;
	self->nextthink = level.time + FRAMETIME;
	gi.bprintf(PRINT_HIGH, "A human spawn was destroyed by a %s!\n", GetMonsterKindString(attacker->mtype));
	INV_RemoveFromSpawnlist(self);
}

void info_player_invasion_think(edict_t *self)
{
	if (level.time > self->lasthurt + 1.0)
		M_Regenerate(self, PLAYERSPAWN_REGEN_FRAMES, PLAYERSPAWN_REGEN_DELAY, 1.0, true, false, false, &self->monsterinfo.regen_delay1);

	self->nextthink = level.time + FRAMETIME;
}

void SP_info_player_invasion(edict_t *self)
{
	//FIXME: change to invasion->value
	if (!pvm->value || !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}

	// remove deathmatch spawnpoints
	if (!INVASION_OTHERSPAWNS_REMOVED)
	{
		edict_t *e = NULL;

		gi.dprintf("PvM Invasion mode activated!\n");

		while ((e = G_Find(e, FOFS(classname), "info_player_deathmatch")) != NULL)
		{
			if (e && e->inuse)
				G_FreeEdict(e);
		}

		INVASION_OTHERSPAWNS_REMOVED = true;
	}

	// this entity should be killable and game should end if all of them die
	gi.setmodel(self, "models/objects/dmspot/tris.md2");
	self->s.skinnum = 0;
	self->mtype = INVASION_PLAYERSPAWN;

	if (invasion->value == 1)
		self->health = PLAYERSPAWN_HEALTH + 250 * AveragePlayerLevel();
	else if (invasion->value == 2)
		self->health = PLAYERSPAWN_HEALTH * 2 + 250 * AveragePlayerLevel();

	self->max_health = self->health;
	self->takedamage = DAMAGE_YES;
	self->die = info_player_invasion_death;
	self->think = info_player_invasion_think;
	self->nextthink = level.time + FRAMETIME;
	//self->touch = info_player_invasion_touch;
	self->solid = SOLID_BBOX;
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, -16);
	gi.linkentity(self);

	self->list_index = invasion_max_playerspawns;
	INV_PlayerSpawns[invasion_max_playerspawns] = self;

	invasion_max_playerspawns++;
	invasion_spawncount++;
}

void SP_info_monster_invasion(edict_t *self)
{
	//FIXME: change to invasion->value
	if (!pvm->value || !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->mtype = INVASION_MONSTERSPAWN;
	self->solid = SOLID_NOT;
	self->s.effects |= EF_BLASTER;
	gi.setmodel (self, "models/items/c_head/tris.md2");

	if (debuginfo->value == 0)
	    self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);

	invasion_monsterspawns++;
}

void SP_navi_monster_invasion(edict_t *self)
{
	//FIXME: change to invasion->value
	if (!pvm->value || !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}

	//gi.dprintf("navi point created!\n");

	self->solid = SOLID_NOT;
	self->mtype = INVASION_NAVI;
	gi.setmodel(self, "models/items/c_head/tris.md2");

	if (!debuginfo->value)
		self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);

	INV_Navi[invasion_navicount++] = self;
}

int G_GetEntityIndex(edict_t *ent)
{
	int		i = 0;
	edict_t *e;

	if (!ent || !ent->inuse)
		return 0;

	for (e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		if (e && e->inuse && (e == ent))
			return i;
		i++;
	}

	return 0;
}

void inv_defenderspawn_think(edict_t *self)
{
	//int		num=G_GetEntityIndex(self); // FOR DEBUGGING ONLY
	vec3_t	start;
	trace_t	tr;
	edict_t *monster = NULL;

	//FIXME: this isn't a good enough check if monster is dead or not
	// did our monster die?
	if (!G_EntIsAlive(self->enemy))
	{
		if (self->orders == MONSTERSPAWN_STATUS_IDLE)
		{
			// wait some time before spawning another monster
			self->wait = level.time + 30;
			self->orders = MONSTERSPAWN_STATUS_WORKING;
			//gi.dprintf("%d: monster died, waiting to build monster...\n", num);
		}

		// try to spawn another
		if ((level.time > self->wait)
			&& (monster = vrx_create_new_drone(self, self->sounds, true, false)) != NULL)
		{
			//gi.dprintf("%d: attempting to spawn a monster\n", num);
			// get starting position
			VectorCopy(self->s.origin, start);
			start[2] = self->absmax[2] + 1 + fabsf(monster->mins[2]);

			tr = gi.trace(start, monster->mins, monster->maxs, start, NULL, MASK_SHOT);

			// kill dead bodies
			if (tr.ent && tr.ent->takedamage && (tr.ent->deadflag == DEAD_DEAD || tr.ent->health < 1))
				T_Damage(tr.ent, self, self, vec3_origin, tr.ent->s.origin, vec3_origin, 10000, 0, 0, 0);
			// spawn is blocked, try again later
			else if (tr.fraction < 1)
			{
				//gi.dprintf("%d: spawn is blocked, will try again\n", num);
				DroneList_Remove(monster); // az 2020: this should've been a thing
				G_FreeEdict(monster);
				self->nextthink = level.time + 10.0;
				return;
			}

			// should this monster stand ground?
			if (self->style)
				monster->monsterinfo.aiflags |= AI_STAND_GROUND;

			monster->s.angles[YAW] = self->s.angles[YAW];
			// move the monster onto the spawn pad
			VectorCopy(start, monster->s.origin);
			VectorCopy(start, monster->s.old_origin);
			monster->s.event = EV_OTHER_TELEPORT;

			// give them quad/invuln to prevent spawn-camping
			if (self->count)
				monster->monsterinfo.inv_framenum = level.framenum + qf2sf(self->count);
			else
				monster->monsterinfo.inv_framenum = level.framenum + (int)(6 / FRAMETIME);

			gi.linkentity(monster);

			self->enemy = monster; // keep track of this monster
			//gi.dprintf("%d: spawned a monster successfully\n", num);
		}
		else
		{
			//if (level.time > self->wait)
			//	gi.dprintf("%d: spawndrone() failed to spawn a monster\n", num);
			//else if (!(level.framenum%10))
			//	gi.dprintf("%d: waiting...\n", num);
			self->orders = MONSTERSPAWN_STATUS_IDLE;
		}
	}
	else
	{
		//if (self->orders == MONSTERSPAWN_STATUS_WORKING)
		//	gi.dprintf("%d: spawn is now idle\n", num);
		self->orders = MONSTERSPAWN_STATUS_IDLE;
	}

	self->nextthink = level.time + FRAMETIME;
}

// map-editable fields - count (quad+invin), style (stand ground), angle, sounds (mtype to spawn)
void SP_inv_defenderspawn(edict_t *self)
{
	//FIXME: change to invasion->value
	if (!pvm->value || !invasion->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->mtype = INVASION_DEFENDERSPAWN;
	self->solid = SOLID_NOT;
	self->think = inv_defenderspawn_think;
	self->nextthink = level.time + pregame_time->value + FRAMETIME;
	self->s.effects |= EF_BLASTER;
	gi.setmodel (self, "models/items/c_head/tris.md2");
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}