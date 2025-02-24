#define MONSTERSPAWN_STATUS_WORKING		0 // spawning a wave of monsters
#define MONSTERSPAWN_STATUS_IDLE		1 // waiting for current wave to be killed

struct invdata_s
{
	qboolean wave_triggered;
	qboolean started;
	int wave_spawned;
	// working monster set
	const int* monster_set;
	int monster_set_count;
	int wave_remaining;
	// default monster set - may be overridden by spawn
	const int* default_monster_set;
	int default_set_count;
	float limitframe;
	edict_t* boss;
	int wave_max_spawn;
	int wave;
};

extern struct invdata_s invasion_data;

int G_GetEntityIndex(edict_t *ent);
void vrx_inv_spawn_monsters(edict_t *self);
void vrx_inv_notify_monster_death(edict_t * edict);
