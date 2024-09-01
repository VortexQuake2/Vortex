#define MONSTERSPAWN_STATUS_WORKING		0 // spawning a wave of monsters
#define MONSTERSPAWN_STATUS_IDLE		1 // waiting for current wave to be killed

struct invdata_s
{
	qboolean printedmessage;
	int mspawned;
	// working monster set
	const int* monster_set;
	int monster_set_count;
	int remaining_monsters;
	// default monster set - may be overridden by spawn
	const int* default_monster_set;
	int default_set_count;
	float limitframe;
	edict_t* boss;
};

extern struct invdata_s invasion_data;

int G_GetEntityIndex(edict_t *ent);
void INV_SpawnMonsters(edict_t *self);
void INV_NotifyMonsterDeath(edict_t * edict);
