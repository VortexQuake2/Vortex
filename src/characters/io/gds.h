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
#define GDS_EXITTHREAD 5

typedef enum threadstatus_s {
    GDS_STATUS_OK,
    GDS_STATUS_CHARACTER_LOADED = 1,
    GDS_STATUS_CHARACTER_DOES_NOT_EXIST = 2,
    GDS_STATUS_CHARACTER_SAVED = 3,
    GDS_STATUS_ALREADY_PLAYING = 4,
    GDS_STATUS_CHARACTER_LOADING = 5,
} threadstatus_t;

// For Everyone
// void V_GDS_Load(edict_t *ent);
qboolean V_GDS_StartConn();
void V_GDS_Queue_Add(edict_t *ent, int operation);
qboolean GDS_IsEnabled();
#ifndef GDS_NOMULTITHREADING
void GDS_FinishThread();
void GDS_HandleStatus(edict_t *player);
#endif

void Mem_PrepareMutexes();

qboolean vrx_mysql_isloading(edict_t *ent);
qboolean vrx_mysql_saveclose_character(edict_t* player);

#endif //NO_GDS

// Wrapped, thread safe mem allocation.
void *V_Malloc(size_t Size, int Tag);
void V_Free (void* mem);

#endif // MYSQL_GDS