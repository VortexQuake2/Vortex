
#ifndef VORTEXQUAKE2_V_SQLITE_UNIDB_H
#define VORTEXQUAKE2_V_SQLITE_UNIDB_H

qboolean VSFU_SaveRunes(edict_t *player);

int VSFU_GetID(char *playername);

// v_sqlite_unidb.c
qboolean VSFU_SavePlayer(edict_t *player);

qboolean VSFU_LoadPlayer(edict_t *player);

void vrx_sqlite_start_connection();

void vrx_sqlite_end_connection();

#endif //VORTEXQUAKE2_V_SQLITE_UNIDB_H
