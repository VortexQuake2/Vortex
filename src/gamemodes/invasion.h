#define MONSTERSPAWN_STATUS_WORKING		0 // spawning a wave of monsters
#define MONSTERSPAWN_STATUS_IDLE		1 // waiting for current wave to be killed

struct invdata_s
{
	qboolean printedmessage;
	int mspawned;
	float limitframe;
	edict_t* boss;
};

extern struct invdata_s invasion_data;
