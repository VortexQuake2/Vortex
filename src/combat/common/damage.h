#define D_PHYSICAL	1
#define D_MAGICAL	2
#define D_BULLET	4
#define D_ENERGY	8
#define D_EXPLOSIVE	16
#define D_PIERCING	32
#define D_SHELL		64
#define D_WORLD		128

int		G_DamageType (int mod, int dflags);
float	vrx_increase_damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t point, float damage, int dflags, int mod);

float vrx_resist_damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, float damage, int dflags, int mod);
int vrx_apply_pierce(const edict_t *targ, const edict_t *attacker, float damage, int dflags, int mod);