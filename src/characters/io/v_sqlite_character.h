#ifndef VORTEXQUAKE2_V_SQLITE_CHARACTER_H
#define VORTEXQUAKE2_V_SQLITE_CHARACTER_H

// v_sqlite_character.c
qboolean VSF_SavePlayer(edict_t *player, char *path, qboolean fileexists, char *playername);

qboolean VSF_LoadPlayer(edict_t *player, char *path);

#endif //VORTEXQUAKE2_V_SQLITE_CHARACTER_H
