/*
 * v_stash.c
 *
 * instead of using the vrx_stash_io functions directly, use these.
 * these wrap around common behaviour that must be found around the stash_io calls.
 *
 */

typedef struct
{
	item_t page[10];
} stash_state_t;

void vrx_init_stash_io();
void vrx_close_stash_io();
void vrx_stash_open(edict_t* ent);
void vrx_stash_open_page(edict_t* ent, item_t* page, int item_count, int page_index);
void vrx_stash_close(edict_t* ent);
void vrx_stash_store(edict_t* ent, int itemindex);

/*
 * stash event notifications.
 * meant to be deferrable using defer_t.
 */

typedef struct
{
	edict_t* ent;
	// "connection ID"
	int gds_connection_id;
} stash_event_t;

typedef struct
{
	edict_t* ent;
	int gds_connection_id; // connection ID
	int gds_owner_id; // database ID
	item_t page[10];
	int pagenum;
} stash_page_event_t;

typedef struct
{
	edict_t* ent;
	int gds_connection_id; // connection ID
	item_t taken;
	char requester[16];
} stash_taken_event_t;

/*
 * the following functions free the memory of the args passed to them.
 */

/* stash_event_t */
void vrx_notify_stash_locked(void* args);
void vrx_notify_stash_no_owner(void* args);

/* stash_page_event_t */
void vrx_notify_open_stash(void* args);

/* stash_taken_event_t */
void vrx_notify_stash_taken(void* args);