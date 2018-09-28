
#ifndef VORTEXQUAKE2_V_SQLITE_UNIDB_H
#define VORTEXQUAKE2_V_SQLITE_UNIDB_H

void VSFU_SaveRunes(edict_t *player);

int VSFU_GetID(char *playername);

// v_sqlite_unidb.c
void VSFU_SavePlayer(edict_t *player);

qboolean VSFU_LoadPlayer(edict_t *player);

void V_VSFU_StartConn();

void V_VSFU_Cleanup();

#endif //VORTEXQUAKE2_V_SQLITE_UNIDB_H
