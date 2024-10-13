#ifndef MYSQL_GDS
#define MYSQL_GDS

#ifndef NO_GDS

typedef enum {
 GDS_LOAD,
 GDS_SAVE,
 GDS_SAVECLOSE,
 GDS_SAVERUNES,
 GDS_STASH_OPEN,
 GDS_STASH_TAKE,
 GDS_STASH_STORE,
 GDS_STASH_CLOSE,
 GDS_STASH_GET_PAGE,
 GDS_SET_OWNER,
 GDS_DISCONNECT,
} gds_op;

typedef enum threadstatus_s {
    GDS_STATUS_OK,
    GDS_STATUS_CHARACTER_LOADED = 1,
    GDS_STATUS_CHARACTER_DOES_NOT_EXIST = 2,
    GDS_STATUS_CHARACTER_SAVED = 3,
    GDS_STATUS_ALREADY_PLAYING = 4,
    GDS_STATUS_CHARACTER_LOADING = 5,
} threadstatus_t;

// For Everyone
// void gds_op_load(edict_t *ent);
qboolean gds_connect();

// for STASH_TAKE/STORE index means what index
// of the stash or inventory to take from respectively.
void gds_queue_add(edict_t *ent, gds_op operation, int index);
void gds_queue_add_setowner(edict_t* ent, char* charname, char* masterpw, qboolean reset);
qboolean gds_enabled();
void gds_finish_thread();
void gds_handle_status(edict_t *player);

qboolean vrx_mysql_isloading(edict_t *ent);
qboolean vrx_mysql_saveclose_character(edict_t* player);

#endif //NO_GDS

void Mem_PrepareMutexes();

#endif // MYSQL_GDS