#define RED_TEAM					1
#define BLUE_TEAM					2
#define BASE_FLAG_TAKEN				1
#define BASE_FLAG_SECURE			2

#define GROUP_ATTACKERS				1
#define GROUP_DEFENDERS				2

qboolean CTF_IsFlag(edict_t* ent);
qboolean CTF_PickupFlag (edict_t *ent, edict_t *other);
void CTF_DropFlag (edict_t *ent, gitem_t *item);
void CTF_SpawnFlag (int teamnum, vec3_t point);
char *CTF_GetTeamString (int teamnum);
char* CTF_GetShortTeam(int teamnum);
void CTF_SpawnFlagBase (int teamnum, vec3_t point);
void CTF_Init (void);
void CTF_ShutDown (void);
void CTF_WriteFlagPosition (edict_t *ent);
void CTF_OpenJoinMenu (edict_t *ent);
float PlayersRangeFromSpot (edict_t *spot);
edict_t *CTF_SelectSpawnPoint (edict_t *ent);
void CTF_SpawnPlayersInBase (int teamnum);
void CTF_AwardTeam (edict_t *ent, int teamnum, int points, int credits);
void CTF_AwardPlayer (edict_t *ent, int points, int credits);
void CTF_AwardFrag (edict_t *attacker, edict_t *target);
void CTF_AwardFlagCapture (edict_t *carrier, int teamnum);
edict_t *CTF_GetFlagBaseEnt (int teamnum);
float CTF_DistanceFromBase(edict_t *ent, vec3_t start, int base_teamnum);
void CTF_CheckFlag (edict_t *ent);
int CTF_GetGroupNum (edict_t *ent, edict_t *base);
int CTF_GetBaseStatus (int teamnum);
int CTF_GetSpecialFcHealth (edict_t *ent, qboolean max);
int CTF_GetTeamCaps (int teamnum);
int CTF_GetEnemyTeam (int teamnum);
void CTF_SpawnFlagAtBase (edict_t *base, int teamnum);
int CTF_GetNumSummonable (char *classname, int teamnum);
int CTF_GetNumClassPlayers (int classnum, int teamnum);
void CTF_RecoverFlag (edict_t *ent);
void CTF_RemovePlayerFlags (void);
int CTF_NumPlayerSpawns (int type, int teamnum);
void CTF_PlayerRespawnTime (edict_t *ent);

void OrganizeTeams (qboolean remove_summonables);
edict_t *SelectDeathmatchSpawnPoint (edict_t *ent);