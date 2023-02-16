#ifndef MYSQL_GDS
#define MYSQL_GDS

/* 
So back then there was a GDS 
and it had issues.

But now, we're doing it Right!<tm>

So, with a few pointers from KOTS2007
(ideas borrowed from kots. heheh)
we're having a MYSQL GDS.

						-az

*/
#ifndef NO_GDS

#define GDS_LOAD 1
#define GDS_SAVE 2
#define GDS_SAVECLOSE 3
#define GDS_SAVERUNES 4
#define GDS_STASH_OPEN 5
#define GDS_STASH_TAKE 6
#define GDS_STASH_STORE 7
#define GDS_STASH_CLOSE 8
#define GDS_STASH_GET_PAGE 9
#define GDS_SET_OWNER 10
#define GDS_EXITTHREAD 11


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
void gds_queue_add(edict_t *ent, int operation, int index);
void gds_queue_add_setowner(edict_t* ent, char* charname, char* masterpw);
qboolean gds_enabled();
#ifndef GDS_NOMULTITHREADING
void gds_finish_thread();
void gds_handle_status(edict_t *player);
#endif

void Mem_PrepareMutexes();

qboolean vrx_mysql_isloading(edict_t *ent);
qboolean vrx_mysql_saveclose_character(edict_t* player);

#endif //NO_GDS

// Wrapped, thread safe mem allocation.
void *V_Malloc(size_t Size, int Tag);
void V_Free (void* mem);

#endif // MYSQL_GDS