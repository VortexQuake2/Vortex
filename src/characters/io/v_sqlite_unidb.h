
#ifndef VORTEXQUAKE2_V_SQLITE_UNIDB_H
#define VORTEXQUAKE2_V_SQLITE_UNIDB_H

qboolean cdb_save_runes(edict_t *player);

int cdb_get_id(char *playername);

// v_sqlite_unidb.c
qboolean cdb_save_player(edict_t *player);

qboolean cdb_saveclose_player(edict_t* player);

qboolean cdb_load_player(edict_t *player);

void cdb_start_connection();

void cdb_end_connection();

qboolean cdb_stash_store(edict_t* ent, int itemindex);

qboolean cdb_stash_open(edict_t* ent);

qboolean cdb_stash_close(edict_t* ent);

qboolean cdb_stash_close_id(int owner_id);

qboolean cdb_stash_take(edict_t* ent, int stash_index);

qboolean cdb_stash_get_page(edict_t* ent, int page, int numitems);

void cdb_set_owner(edict_t* ent, char* owner_name, char* masterpw);

#endif //VORTEXQUAKE2_V_SQLITE_UNIDB_H
