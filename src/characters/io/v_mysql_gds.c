#include "g_local.h"
#include "v_characterio.h"

#ifndef NO_GDS

#include <mysql.h>
#include "gds.h"
#include "characters/class_limits.h"

#ifndef GDS_NOMULTITHREADING
#include <pthread.h>
#endif

#ifdef _WIN32
#pragma warning ( disable : 4090 ; disable : 4996 )
#endif

// *********************************
// Definitions
// *********************************

#define DEFAULT_DATABASE "127.0.0.1"
#define MYSQL_PW "123456"
#define MYSQL_USER "root"
#define MYSQL_DBNAME "vortex"

/* 
These are very similar to the sqlite version.
Most of the redundancy comes from not abstracting the surrounding context.
I think that's fine. -az
*/

// abilities

const char* MYSQL_INSERTABILITY = "INSERT INTO abilities VALUES (%d,%d,%d,%d,%d,%d,%d,%d);";

// runes

const char* MYSQL_INSERTRMETA = "INSERT INTO runes_meta VALUES (%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,%d,%d);";

const char* MYSQL_INSERTRMOD = "INSERT INTO runes_mods VALUES (%d,%d,%d,%d,%d,%d,%d);";

// update stuff

const char* MYSQL_UPDATECDATA = "UPDATE character_data SET respawns=%d, health=%d, maxhealth=%d, armour=%d, maxarmour=%d, nerfme=%d, adminlevel=%d, bosslevel=%d WHERE char_idx=%d;";

const char* MYSQL_UPDATESTATS = "UPDATE game_stats SET shots=%d, shots_hit=%d, frags=%d, fragged=%d, num_sprees=%d, max_streak=%d, spree_wars=%d, broken_sprees=%d, broken_spreewars=%d, suicides=%d, teleports=%d, num_2fers=%d WHERE char_idx=%d;";

const char* MYSQL_UPDATECTFSTATS = "UPDATE ctf_stats SET flag_pickups=%d, flag_captures=%d, flag_returns=%d, flag_kills=%d, offense_kills=%d, defense_kills=%d, assists=%d WHERE char_idx=%d;";

// MYSQL
MYSQL* GDS_MySQL = NULL;

// Queue. Make certain things unions because they're mutually exclusive.
typedef struct
{
	edict_t *ent; /* for most operations. */
	int operation; /* GDS_LOAD, GDS_SAVE, etc... */
	int connection_id; /* ent connection id */
	void *next; /* next node in the chain */

	/* these two are mutually exclusive */
	union {
		/* save, saveclose, saverunes... */
		skills_t myskills;

		/* for stash only. */
		item_t item;
	};

	union {
		/* for page event */
		int page_index;

		/* for take event */
		int stash_index;
	};

	/* used to not have to access client->pers.netname. it can change. */
	char char_name[16];

	/* for setowner operation. */
	char owner_name[24];
	char masterpw[64];

	/* database id */
	int gds_id;
} gds_queue_t;

static gds_queue_t *first;
static gds_queue_t *last;

static gds_queue_t *free_first = NULL;
static gds_queue_t *free_last = NULL;
// CVARS
// cvar_t *gds_debug; // Should I actually use this? -az
// if gds_singleserver is set then bypass the isplaying check.
static cvar_t *gds_singleserver;

// Threading
#ifndef GDS_NOMULTITHREADING

static pthread_t QueueThread;
static pthread_attr_t attr;
static pthread_mutex_t mutex_gds_queue;
static pthread_mutex_t mutex_player_status;
static pthread_mutex_t MemMutex_Free;
static pthread_mutex_t MemMutex_Malloc;
static pthread_mutex_t mutex_gds_thread_status;

static qboolean ThreadRunning;

#endif

// Prototypes
int gds_op_save(gds_queue_t *myskills, MYSQL* db);
qboolean gds_op_load(gds_queue_t *current, MYSQL *db);
void gds_op_saveclose(gds_queue_t *current, MYSQL *db);
int gds_op_save_runes(gds_queue_t *current, MYSQL* db);
void gds_create_process_queue();
qboolean gds_process_queue_thread_running();

// Utility Functions

// va used only in the thread.
char	*myva(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	return string;	
}

int FindRuneIndex_S(int index, skills_t *myskills)
{
	int count = 0;

	for (int i = 0; i < MAX_VRXITEMS; ++i)
	{
		if (myskills->items[i].itemtype != ITEM_NONE)
		{
			++count;
			if (count == index)
			{
				return i;
			}
		}
	}
	return -1;	//just in case something messes up
}

int V_WeaponUpgradeVal_S(skills_t *myskills, int weapnum)
{
    //Returns an integer value, usualy from 0-100. Used as a % of maximum upgrades statistic

    float iCount;

    float iMax = iCount = 0.0f;

	if ((weapnum >= MAX_WEAPONS) || (weapnum < 0))
		return -666;	//BAD weapon number

	for (int i = 0; i<MAX_WEAPONMODS;++i)
	{
		iCount += myskills->weapons[weapnum].mods[i].current_level;
		iMax += myskills->weapons[weapnum].mods[i].soft_max;
	}

	if (iMax == 0)
		return 0;

	float val = (iCount / iMax) * 100;
	
	return (int)val;
}

int CountWeapons_S(skills_t *myskills)
{
	int count = 0;
	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		if (V_WeaponUpgradeVal_S(myskills, i) > 0)
			count++;
	}
	return count;
}


int CountRunes_S(skills_t* myskills)
{
	int count = 0;

	for (int i = 0; i < MAX_VRXITEMS; ++i)
	{
		if (myskills->items[i].itemtype != ITEM_NONE)
			++count;
	}
	return count;
}

int CountAbilities_S(skills_t *myskills)
{
	int count = 0;
	for (int i = 0; i < MAX_ABILITIES; ++i)
	{
		if (!myskills->abilities[i].disable)
			++count;
	}
	return count;
}

int FindAbilityIndex_S(int index, skills_t *myskills)
{
	int count = 0;
	for (int i = 0; i < MAX_ABILITIES; ++i)
	{
		if (!myskills->abilities[i].disable)
		{
			++count;
			if (count == index)
				return i;
		}
	}
	return -1;	//just in case something messes up
}

int FindWeaponIndex_S(int index, skills_t *myskills)
{
	int count = 0;
	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		if (V_WeaponUpgradeVal_S(myskills, i) > 0)
		{
			count++;
			if (count == index)
				return i;
		}
	}
	return -1;	//just in case something messes up
}


// *********************************
// QUEUE functions
// *********************************
void gds_freequeue_add(gds_queue_t *current)
{
	if (current)
	{
		if (!free_first)
		{
			free_last = free_first = current;
		}
		else
		{
			free_last->next = current;
			free_last = current;
			free_last->next = NULL;
		}
	}
}

void gds_freequeue_free()
{
	gds_queue_t *next;
	
	while (free_first)
	{
		next = free_first->next;
		V_Free (free_first);
		free_first = next;
	}

	free_last = free_first = NULL;
}

void gds_queue_push(gds_queue_t *current, qboolean lock)
{
#ifndef GDS_NOMULTITHREADING
	if (lock)
		pthread_mutex_lock(&mutex_gds_queue);
#endif
	
	if (current)
	{
		last->next = current;
		last = current;
		last->next = NULL;
	}

#ifndef GDS_NOMULTITHREADING
	if (lock)
		pthread_mutex_unlock(&mutex_gds_queue);
#endif
}

/* queue mutex must be locked before calling this
 * will make 'last' into a new operation
 */
void gds_queue_make_operation()
{
	if (!first)
	{
		first = V_Malloc(sizeof(gds_queue_t), TAG_GAME);
		last = first;
		first->ent = NULL;
	}
	
	if (first->ent) // warning: we assume first is != NULL!
	{
		// Use this empty next node 
		// for a new node
		gds_queue_t* newest = V_Malloc(sizeof(gds_queue_t), TAG_GAME); 

		gds_queue_push(newest, false); // false since we're already locked
	}
}

void gds_queue_add(edict_t *ent, int operation, int index)
{
	if ((!ent || !ent->client) && operation != GDS_EXITTHREAD)
	{
		gi.dprintf("gds_queue_add: Null Entity or Client!\n");
	}

	if (!GDS_MySQL)
	{
		safe_cprintf(ent, PRINT_HIGH, "It seems that there's no connection to the database. Contact an admin ASAP.\n");
		return;
	}

	if (operation != GDS_SAVE && 
		operation != GDS_LOAD && 
		operation != GDS_SAVECLOSE &&
		operation != GDS_SAVERUNES &&
		operation != GDS_EXITTHREAD &&
		operation != GDS_STASH_OPEN &&
		operation != GDS_STASH_TAKE &&
		operation != GDS_STASH_STORE &&
		operation != GDS_STASH_CLOSE &&
		operation != GDS_STASH_GET_PAGE)
	{
		gi.dprintf("gds_queue_add: Called with wrong operation!\n");
		return;
	}

	if (operation == GDS_EXITTHREAD && ent)
	{
		//if (gds_debug->value)
			gi.dprintf("gds_queue_add: Called with an entity on an exit thread operation?\n");
	}

	if (operation == GDS_STASH_STORE)
	{
		if (index < 0 || index >= MAX_VRXITEMS)
		{
			gi.dprintf("called GDS_STASH_STORE with an invalid item index (%d)\n", index);
			return;
		}
	}

	if (operation == GDS_STASH_TAKE)
	{
		if (index < 0) {
			gi.dprintf("called GDS_STASH_STARE with negative index (%d)\n", index);
			return;
		}
	}

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_gds_queue);
#endif

	gds_queue_make_operation();

	last->ent = ent;
	last->next = NULL; // make sure last node has null pointer 
	last->operation = operation;
	last->connection_id = ent ? ent->gds_connection_id : 0;

	if (ent) {
		assert(sizeof last->char_name == sizeof ent->client->pers.netname);
		strcpy(last->char_name, ent->client->pers.netname);
	}

	if (operation == GDS_STASH_TAKE)
	{
		last->stash_index = index;
	}
	else if (operation == GDS_STASH_STORE)
	{
		// remove the item from the player.
		// there's a chance we could lose this data if the server crashes
		// before this data is committed
		// but oh well. -az
		last->item = ent->myskills.items[index];
		memset(&ent->myskills.items[index], 0, sizeof (item_t));
	}
	else if (operation == GDS_SAVE || 
		operation == GDS_SAVECLOSE ||
		operation == GDS_SAVERUNES)
	{
		memcpy(&last->myskills, &ent->myskills, sizeof(skills_t));
	} else if (operation == GDS_STASH_CLOSE)
	{
		if (ent)
			last->gds_id = -1;
		else
		{
			last->gds_id = index;
		}
	} else if (operation == GDS_STASH_GET_PAGE)
	{
		last->page_index = index;
	}

	// if it doesn't exist, create the process queue.
	if (!gds_process_queue_thread_running()) 
		gds_create_process_queue(); 
	
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&mutex_gds_queue);
#endif
}

void gds_queue_add_setowner(edict_t* ent, char* charname, char* masterpw)
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_gds_queue);
#endif
	gds_queue_make_operation();

	last->ent = ent;
	last->operation = GDS_SET_OWNER;
	last->connection_id = ent->gds_connection_id;
	strcpy(last->owner_name, charname);
	strcpy(last->char_name, ent->client->pers.netname);

	// these verifications should be done by Cmd_SetOwner_f
	// if (strlen(masterpw) < sizeof last->masterpw)
	strcpy(last->masterpw, masterpw);
	// else {}

	if (!gds_process_queue_thread_running())
		gds_create_process_queue();

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&mutex_gds_queue);
#endif
}

gds_queue_t *gds_queue_pop_first()
{
	gds_queue_t *node;

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_gds_queue);
#endif
	node = first; // save node on top to delete
	
	if (first && first->operation) // does our node have something to do?
		first = first->next; // set first to next node

	// give me the assgined entity of the first node
	// node must be freed by caller
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&mutex_gds_queue);
#endif
	return node; 
}

// *********************************
// Database Thread
// *********************************

qboolean gds_process_queue_thread_running()
{
#ifndef GDS_NOMULTITHREADING
	qboolean temp;
	pthread_mutex_lock(&mutex_gds_thread_status);

	temp = ThreadRunning;

	pthread_mutex_unlock(&mutex_gds_thread_status);
	return temp;
#else
	return true;
#endif
}

void gds_set_thread_running(qboolean running)
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_gds_thread_status);

	ThreadRunning = running;

	pthread_mutex_unlock(&mutex_gds_thread_status);
#endif
}


void gds_op_stash_open(gds_queue_t* current, MYSQL* db);

void gds_op_stash_store(gds_queue_t* current, MYSQL* db);

void gds_op_stash_take(gds_queue_t* current, MYSQL* db);

void gds_op_stash_close(gds_queue_t* current, MYSQL* db);

void gds_op_stash_get_page(gds_queue_t* current, MYSQL* db);


void gds_op_set_owner(gds_queue_t* current, MYSQL* db);

void *gds_process_queue(void *unused)
{
	gds_queue_t *current = gds_queue_pop_first();

	while (current)
	{
#ifndef GDS_NOMULTITHREADING
		if (!current)
		{
			gds_set_thread_running(false);
			pthread_exit(NULL);
			continue;
		}
#endif
		
		if (current->operation == GDS_LOAD)
		{
			gds_op_load(current, GDS_MySQL);
		}else if (current->operation == GDS_SAVE)
		{
			gds_op_save(current, GDS_MySQL);
		}else if (current->operation == GDS_SAVECLOSE)
		{
			gds_op_saveclose(current, GDS_MySQL);
		}else if (current->operation == GDS_SAVERUNES)
		{
			gds_op_save_runes(current, GDS_MySQL);
		}else if (current->operation == GDS_STASH_OPEN)
		{
			gds_op_stash_open(current, GDS_MySQL);
		}else if (current->operation == GDS_STASH_STORE)
		{
			gds_op_stash_store(current, GDS_MySQL);
		}else if (current->operation == GDS_STASH_TAKE)
		{
			gds_op_stash_take(current, GDS_MySQL);
		}else if (current->operation == GDS_STASH_CLOSE)
		{
			gds_op_stash_close(current, GDS_MySQL);
		}else if (current->operation == GDS_STASH_GET_PAGE)
		{
			gds_op_stash_get_page(current, GDS_MySQL);
		}else if (current->operation == GDS_SET_OWNER)
		{
			gds_op_set_owner(current, GDS_MySQL);
		}

		gds_freequeue_add(current);
		current = gds_queue_pop_first();
	}

#ifndef GDS_NOMULTITHREADING
	gds_set_thread_running(false);
	pthread_exit(NULL);
#endif
	return NULL;
}

// *********************************
// MYSQL functions
// *********************************

#define QUERY(a, ...) { char* format = strdup(myva(a, __VA_ARGS__));\
	if (mysql_query(db, format)) { gi.dprintf("DB: %s", mysql_error(db)); };\
	free (format); }

#define GET_RESULT result = mysql_store_result(db);\
	row = mysql_fetch_row(result);

#define FREE_RESULT mysql_free_result(result);

#define DECLARE_ESCAPE(varname, src) char varname[sizeof src * 2];\
	mysql_real_escape_string(db, varname, src, strlen(src));

#define DECLARE_ESCAPE_S(varname, len, src) char varname[len];\
	mysql_real_escape_string(db, varname, src, strlen(src));


int gds_get_id(const char* name, MYSQL *db)
{
	char escaped[32];

	mysql_real_escape_string(db, escaped, name, strlen(name));

	QUERY ("CALL GetCharID(\"%s\", @PID);", escaped)

	mysql_query (db, "SELECT @PID;"); 

	MYSQL_RES* result = mysql_store_result(db);
	
	if (result != NULL) {
		MYSQL_ROW row = mysql_fetch_row(result);
		int id = -1;
		if (row[0]) {
			id = atoi(row[0]);
		}

		return id;
	}

	return -1;
}


int gds_get_owner_id(char* charname, MYSQL* db)
{
	char escaped[64];

	mysql_real_escape_string(db, escaped, charname, strlen(charname));
	QUERY("select char_idx from userdata "
		"where playername = (select owner from userdata where playername = '%s') "
		"or (playername = '%s' and email is not null and LENGTH(email) > 0)",
		escaped, escaped)

	MYSQL_RES* result = mysql_store_result(db);
	MYSQL_ROW row = mysql_fetch_row(result);

	if (!row) {
		mysql_free_result(result);
		return -1;
	}

	int id = atoi(row[0]);
	mysql_free_result(result);
	return id;
}

int gds_subop_stash_get_page(int owner_id, item_t* page, int items_per_page, int page_index, MYSQL* db)
{
	int itemcount = 0;
	memset(page, 0, sizeof(item_t) * items_per_page);

	const int start_index = page_index * items_per_page;
	const int end_index = start_index + items_per_page;

	mysql_query(db, "BEGIN TRANSACTION");

	// reader notice: scoping like this prevents using the variables from the wrong context.
	{
		QUERY("select stash_index, itemtype, itemlevel, quantity, untradeable, "
			"id, name, nummods, setcode, classnum "
			"from stash_runes_meta where char_idx=%d "
			"and stash_index >= %d and stash_index <= %d",
			owner_id, start_index, end_index);

		MYSQL_RES* item_head_result = mysql_store_result(db);

		if (item_head_result == NULL) {
			mysql_query(db, "COMMIT");
			return 0;
		}

		MYSQL_ROW item_head_row = mysql_fetch_row(item_head_result);

		int safeguard_items = 0;
		while (item_head_row && safeguard_items < items_per_page)
		{
			const int page_row_index = atoi(item_head_row[0]) - start_index;
			assert(page_row_index >= 0 && page_row_index < items_per_page);

			item_t* item = &page[page_row_index];

			item->itemtype = atoi(item_head_row[1]);
			item->itemLevel = atoi(item_head_row[2]);
			item->quantity = atoi(item_head_row[3]);
			item->untradeable = atoi(item_head_row[4]);
			strncpy(item->id, item_head_row[5], sizeof item->id);
			strncpy(item->name, item_head_row[6], sizeof item->name);
			item->numMods = atoi(item_head_row[7]);
			item->setCode = atoi(item_head_row[8]);
			item->classNum = atoi(item_head_row[9]);

			safeguard_items++;
			item_head_row = mysql_fetch_row(item_head_result);
		}

		itemcount = safeguard_items;
		mysql_free_result(item_head_result);
	}

	// we fetch everything in one query.
	{
		QUERY("select stash_index, rune_mod_index, type, mod_index, value, rset "
			"from stash_runes_mods where char_idx = %d "
			"and stash_index >= %d and stash_index <= %d order by stash_index", owner_id, start_index, end_index);

		MYSQL_RES* item_modifier_result = mysql_store_result(db);
		MYSQL_ROW item_modifier_row = mysql_fetch_row(item_modifier_result);
		int last_row = 0;
		int mod_index = 0;
		while (item_modifier_row && mod_index < MAX_VRXITEMMODS)
		{
			const int page_row_index = atoi(item_modifier_row[0]) - start_index;
			assert(page_row_index >= 0 && page_row_index < items_per_page);

			if (last_row != page_row_index) {
				mod_index = 0;
				last_row = page_row_index;
			}

			/*const int modn = atoi(item_modifier_row[1]);
			assert(modn >= 0 && modn < MAX_VRXITEMMODS);*/

			imodifier_t* mod = &page[page_row_index].modifiers[mod_index];

			mod->type = atoi(item_modifier_row[2]);
			mod->index = atoi(item_modifier_row[3]);
			mod->value = atoi(item_modifier_row[4]);
			mod->set = atoi(item_modifier_row[5]);

			item_modifier_row = mysql_fetch_row(item_modifier_result);
			mod_index++;
		}
		mysql_free_result(item_modifier_result);
	}

	mysql_query(db, "COMMIT");
	return itemcount;
}



void gds_op_set_owner(gds_queue_t* current, MYSQL* db)
{
	int new_owner_id = gds_get_id(current->owner_name, db);

	event_owner_error_t* evt = V_Malloc(sizeof(event_owner_error_t), TAG_GAME);
	assert(sizeof evt->owner_name == sizeof current->owner_name);
	strcpy(evt->owner_name, current->owner_name);
	evt->ent = current->ent;
	evt->connection_id = current->connection_id;

	if (new_owner_id == -1)
	{
		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_owner_nonexistent,
			.data = evt
		});
		return;
	}

	int id = gds_get_id(current->char_name, db);

	DECLARE_ESCAPE(esc_ownername, current->owner_name);
	DECLARE_ESCAPE(esc_password, current->masterpw);

	QUERY(
		"update userdata target "
		"	join  userdata owner on "
		"		owner.email = \"%s\" and "
		"		owner.email is not null and "
		"		owner.char_idx=%d "
		"set target.owner = \"%s\""
		"where target.char_idx = %d",
		esc_password, 
		new_owner_id,
		esc_ownername, 
		id);

	int rows = mysql_affected_rows(db);
	if (rows == 0)
	{
		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_owner_bad_password,
				.data = evt
		});
	} else
	{
		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_owner_success,
				.data = evt
		});
	}
}

void gds_op_stash_get_page(gds_queue_t* current, MYSQL* db)
{
	int owner_id = gds_get_owner_id(current->char_name, db);

	if (owner_id == -1)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_stash_no_owner,
				.data = notif
		});

		return;
	}

	stash_page_event_t* evt = V_Malloc(sizeof(stash_page_event_t), TAG_GAME);
	evt->gds_connection_id = current->connection_id;
	evt->gds_owner_id = owner_id;
	evt->ent = current->ent;
	evt->pagenum = current->page_index;

	gds_subop_stash_get_page(
		owner_id, 
		evt->page, 
		sizeof evt->page / sizeof(item_t), 
		evt->pagenum, 
		db
	);

	defer_add(&current->ent->client->defers, (defer_t) {
		.function = vrx_notify_open_stash,
			.data = evt
	});
}

/* opening the stash is a surprisingly complicated transaction. */
void gds_op_stash_open(gds_queue_t* current, MYSQL* db)
{
	int owner_id = gds_get_owner_id(current->char_name, db);

	if (owner_id == -1)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_stash_no_owner,
				.data = notif
		});

		return;
	}

	mysql_autocommit(db, false);
	mysql_query(db, "START TRANSACTION");

	/* check if the character is locked */
	QUERY("select lock_char_id from stash where char_idx=%d for update", owner_id);

	MYSQL_RES* lock_status_result = mysql_store_result(db);
	qboolean locked = false;

	if (lock_status_result != NULL)
	{
		const MYSQL_ROW lock_status_row = mysql_fetch_row(lock_status_result);
		if (lock_status_row[0] != NULL)
			locked = true;
	} else
	{
		gi.dprintf("row for character %d does not exist.\n", owner_id);
		locked = true;
	}

	mysql_free_result(lock_status_result);

	if (locked)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_stash_locked,
				.data = notif
		});
	}
	else
	{
		/* lock the stash */
		int id = gds_get_id(current->char_name, db);
		QUERY("update stash set lock_char_id = %d where char_idx=%d", id, owner_id);

		/* fetch a page and open the stash */
		stash_page_event_t* notif = V_Malloc(sizeof(stash_page_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;
		notif->gds_owner_id = owner_id;
		notif->pagenum = 0;

		gds_subop_stash_get_page(
			owner_id, 
			notif->page, 
			sizeof notif->page / sizeof (item_t), 
			0,
			db
		);

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_open_stash,
				.data = notif
		});
	}

	mysql_query(db, "COMMIT");
	mysql_autocommit(db, true);
}

void gds_op_stash_close(gds_queue_t* current, MYSQL* db)
{
	const int owner_id = current->ent ? gds_get_owner_id(current->char_name, db) : current->gds_id;

	if (owner_id == -1 && current->ent)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_stash_no_owner,
				.data = notif
		});

		return;
	}

	QUERY("update stash set lock_char_id = NULL where char_idx=%d", owner_id);
}

void gds_op_stash_store(gds_queue_t* current, MYSQL* db)
{
	int owner_id = gds_get_owner_id(current->char_name, db);

	if (owner_id == -1)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = current->ent;
		notif->gds_connection_id = current->connection_id;

		defer_add(&current->ent->client->defers, (defer_t) {
			.function = vrx_notify_stash_no_owner,
				.data = notif
		});

		return;
	}

	mysql_autocommit(db, false);

	mysql_query(db, "START TRANSACTION");

	/* find a reasonable stash index for our rune */
	int index = 0;
	{
		QUERY("select stash_index from stash_runes_meta "
			"where char_idx=%d "
			"order by stash_index", owner_id)

		MYSQL_RES* res = mysql_store_result(db);
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(res)))
		{
			int index_result = atoi(row[0]);

			// we found a free slot
			if (index < index_result)
				break;

			// continue looking
			index++;
		}

		mysql_free_result(res);
	}

	item_t item = current->item;

	QUERY("INSERT INTO stash_runes_meta " 
		"VALUES (%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,%d,%d)", 
		owner_id,
		index,
		item.itemtype,
		item.itemLevel,
		item.quantity,
		item.untradeable,
		item.id,
		item.name,
		item.numMods,
		item.setCode,
		item.classNum)

	for (int j = 0; j < MAX_VRXITEMMODS; ++j)
	{
		// TYPE_NONE, so skip it
		if (item.modifiers[j].type == 0 ||
			item.modifiers[j].value == 0)
			continue;

		QUERY("INSERT INTO stash_runes_mods "
			"VALUES (%d,%d,%d,%d,%d,%d,%d)", 
			owner_id, index, j,
			item.modifiers[j].type,
			item.modifiers[j].index,
			item.modifiers[j].value,
			item.modifiers[j].set)
	}

	mysql_query(db, "COMMIT");
	mysql_autocommit(db, true);
}



void gds_op_stash_take(gds_queue_t* current, MYSQL* db)
{
	int owner_id = gds_get_owner_id(current->char_name, db);
	int id = gds_get_id(current->char_name, db);

	/* check if we're the owners of the stash before taking */
	{
		QUERY("select char_idx from stash where lock_char_id=%d", id);

		MYSQL_RES* res = mysql_store_result(db);
		MYSQL_ROW row = mysql_fetch_row(res);
		if (!row || atoi(row[0]) != owner_id)
		{
			stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
			notif->ent = current->ent;
			notif->gds_connection_id = current->connection_id;

			defer_add(&current->ent->client->defers, (defer_t) {
				.function = vrx_notify_stash_locked,
					.data = notif
			});

			mysql_free_result(res);
			return;
		}

		mysql_free_result(res);
	}

	/* take it */
	item_t item;
	V_ItemClear(&item);

	{
		QUERY("select stash_index, itemtype, itemlevel, quantity, untradeable, "
			"id, name, nummods, setcode, classnum "
			"from stash_runes_meta where char_idx=%d "
			"and stash_index = %d",
			owner_id, current->stash_index);

		MYSQL_RES* item_head_result = mysql_store_result(db);
		MYSQL_ROW item_head_row = mysql_fetch_row(item_head_result);

		if (item_head_row)
		{
			item.itemtype = atoi(item_head_row[1]);
			item.itemLevel = atoi(item_head_row[2]);
			item.quantity = atoi(item_head_row[3]);
			item.untradeable = atoi(item_head_row[4]);
			strncpy(item.id, item_head_row[5], sizeof item.id);
			strncpy(item.name, item_head_row[6], sizeof item.name);
			item.numMods = atoi(item_head_row[7]);
			item.setCode = atoi(item_head_row[8]);
			item.classNum = atoi(item_head_row[9]);
		} else
		{
			// TODO: defer "the item has been removed already" -- this shouldn't happen, however.
		}

		mysql_free_result(item_head_result);
	}

	{
		QUERY("select stash_index, rune_mod_index, type, mod_index, value, rset "
			"from stash_runes_mods where char_idx = %d "
			"and stash_index = %d", owner_id, current->stash_index);

		MYSQL_RES* item_modifier_result = mysql_store_result(db);
		MYSQL_ROW item_modifier_row = mysql_fetch_row(item_modifier_result);
		while (item_modifier_row)
		{
			const int modn = atoi(item_modifier_row[1]);
			assert(modn >= 0 && modn < MAX_VRXITEMMODS);

			imodifier_t* mod = &item.modifiers[modn];

			mod->type = atoi(item_modifier_row[2]);
			mod->index = atoi(item_modifier_row[3]);
			mod->value = atoi(item_modifier_row[4]);
			mod->set = atoi(item_modifier_row[5]);

			item_modifier_row = mysql_fetch_row(item_modifier_result);
		}

		mysql_free_result(item_modifier_result);
	}

	QUERY("delete from stash_runes_meta where stash_index=%d and char_idx=%d", current->stash_index, owner_id);
	QUERY("delete from stash_runes_mods where stash_index=%d and char_idx=%d", current->stash_index, owner_id);

	stash_taken_event_t* evt = V_Malloc(sizeof(stash_taken_event_t), TAG_GAME);
	evt->ent = current->ent;
	evt->gds_connection_id = current->connection_id;
	V_ItemCopy(&item, &evt->taken);

	assert(sizeof evt->requester == sizeof current->char_name);
	strcpy(evt->requester, current->char_name);

	defer_add(&current->ent->client->defers, (defer_t) {
		.function = vrx_notify_stash_taken,
		.data = evt
	});
}

void gds_subop_insert_rune(MYSQL* db, gds_queue_t* current, int id)
{
	//begin runes
	int numRunes = CountRunes_S(&current->myskills);
	for (int i = 0; i < numRunes; ++i)
	{
		int index = FindRuneIndex_S(i+1, &current->myskills);
		if (index != -1)
		{
			item_t item = current->myskills.items[index];

			QUERY (MYSQL_INSERTRMETA, id, 
			       index,
			       item.itemtype,
			       item.itemLevel,
			       item.quantity,
			       item.untradeable,
			       item.id,
			       item.name,
			       item.numMods,
			       item.setCode,
			       item.classNum)
			for (int j = 0; j < MAX_VRXITEMMODS; ++j)
			{
				// TYPE_NONE, so skip it
				if (item.modifiers[j].type == 0 || 
					item.modifiers[j].value == 0)
					continue;

				QUERY(MYSQL_INSERTRMOD, id, index, j,
				      item.modifiers[j].type,
				      item.modifiers[j].index,
				      item.modifiers[j].value,
				      item.modifiers[j].set)
			}
		}
	}
	//end runes
}

// GDS_Save except it only deals with runes.
int gds_op_save_runes(gds_queue_t *current, MYSQL* db)
{
	char* format;
	const int id = gds_get_id(current->myskills.player_name, db);
	mysql_autocommit(db, false);

	mysql_query(db, "START TRANSACTION");

	QUERY("DELETE FROM runes_meta WHERE char_idx=%d", id)
	QUERY("DELETE FROM runes_mods WHERE char_idx=%d", id)

	gds_subop_insert_rune(db, current, id);
	
	mysql_query(db, "COMMIT"); // hopefully this will avoid shit breaking
	mysql_autocommit(db, true);

	return 0;
}

int gds_op_save(gds_queue_t *current, MYSQL* db)
{
	int i;
	int id; // character ID
	const int numAbilities = CountAbilities_S(&current->myskills);
	const int numWeapons = CountWeapons_S(&current->myskills);
	MYSQL_ROW row;
	MYSQL_RES *result;

	if (!db)
	{
/*		if (gds_debug->value)
			gi.dprintf("DB: NULL database (gds_op_save())\n"); */
		return -1;
	}

	mysql_autocommit(db, false);

	DECLARE_ESCAPE(esc_pname, current->myskills.player_name);

	QUERY ("CALL CharacterExists(\"%s\", @exists);", esc_pname)

	QUERY ("SELECT @exists;")

	GET_RESULT

	if (!strcmp(row[0], "0"))
		i = 0;
	else
		i = 1;
	
	FREE_RESULT

	if (!i) // does not exist
	{
		// Create initial database.
		gi.dprintf("DB: Creating character \"%s\"!\n", current->myskills.player_name);
		QUERY ("CALL FillNewChar(\"%s\");", esc_pname)
	}

	id = gds_get_id(current->myskills.player_name, db);

	{ // real saving
		mysql_query(db, "START TRANSACTION");
		// reset tables (remove records for reinsertion)
		QUERY("CALL ResetTables(\"%s\");", esc_pname);

		/* do some escaping, just in case. */
		DECLARE_ESCAPE(esc_title, current->myskills.title);
		DECLARE_ESCAPE(esc_password, current->myskills.password);
		DECLARE_ESCAPE(esc_masterpw, current->myskills.email);
		DECLARE_ESCAPE(esc_owner, current->myskills.email);

		QUERY ("UPDATE userdata SET " 
			"title=\"%s\", "
			"playername=\"%s\", "
			"password=\"%s\", "
			"email=\"%s\", "
			"owner=if(length(owner) = 0 or owner is null, \"%s\", owner), "
			"member_since=\"%s\", "
			"last_played=CURDATE(), "
			"playtime_total=%d, "
			"playingtime=%d, "
			"isplaying = 1 "
			"WHERE char_idx=%d",
		 esc_title,
		 esc_pname,
		 esc_password,
		 esc_masterpw,
		 esc_owner,
 		 current->myskills.member_since,
		 current->myskills.total_playtime,
 		 current->myskills.playingtime, id)

		// talents
		for (i = 0; i < current->myskills.talents.count; ++i)
		{
			QUERY ("INSERT INTO talents VALUES (%d,%d,%d,%d)", 
				id, 
				current->myskills.talents.talent[i].id,
				current->myskills.talents.talent[i].upgradeLevel,
				current->myskills.talents.talent[i].maxLevel)
		}

		// abilities
	
		for (i = 0; i < numAbilities; ++i)
		{
			int index = FindAbilityIndex_S(i+1, &current->myskills);
			if (index != -1)
			{
				QUERY (MYSQL_INSERTABILITY, id, index, 
					current->myskills.abilities[index].level,
					current->myskills.abilities[index].max_level,
					current->myskills.abilities[index].hard_max,
					current->myskills.abilities[index].modifier,
					(int)current->myskills.abilities[index].disable,
					(int)current->myskills.abilities[index].general_skill)
			}
		}
		// gi.dprintf("saved abilities");
		
		//*****************************
		//in-game stats
		//*****************************

		QUERY (MYSQL_UPDATECDATA,
		 current->myskills.weapon_respawns,
		 current->myskills.current_health,
		 MAX_HEALTH(current->ent),
		 current->ent->client ? current->ent->client->pers.inventory[body_armor_index] : 0,
  		 MAX_ARMOR(current->ent),
 		 current->myskills.nerfme,
		 current->myskills.administrator, // flags
		 current->myskills.boss,  id) // last param WHERE char_idx = %d

		//*****************************
		//stats
		//*****************************

		QUERY (MYSQL_UPDATESTATS, 
		 current->myskills.shots,
		 current->myskills.shots_hit,
		 current->myskills.frags,
		 current->myskills.fragged,
  		 current->myskills.num_sprees,
 		 current->myskills.max_streak,
		 current->myskills.spree_wars,
		 current->myskills.break_sprees,
		 current->myskills.break_spree_wars,
		 current->myskills.suicides,
		 current->myskills.teleports,
		 current->myskills.num_2fers, id)

		//*****************************
		//standard stats
		//*****************************
		
		QUERY("UPDATE point_data SET "
			"exp=%d, exptnl=%d, level=%d, classnum=%d, skillpoints=%d, "
			"credits=%d, weap_points=%d, resp_weapon=%d, tpoints=%d "
			"WHERE char_idx=%d",
		 current->myskills.experience,
		 current->myskills.next_level,
         current->myskills.level,
		 current->myskills.class_num,
		 current->myskills.speciality_points,
 		 current->myskills.credits,
		 current->myskills.weapon_points,
		 current->myskills.respawn_weapon,
		 current->myskills.talents.talentPoints, id)

		//begin weapons
		for (i = 0; i < numWeapons; ++i)
		{
			int index = FindWeaponIndex_S(i+1, &current->myskills);
			if (index != -1)
			{
				int j;
				QUERY ("INSERT INTO weapon_meta VALUES (%d,%d,%d)", 
					id, 
					index,
				    current->myskills.weapons[index].disable)

				for (j = 0; j < MAX_WEAPONMODS; ++j)
				{
					QUERY ("INSERT INTO weapon_mods VALUES (%d,%d,%d,%d,%d,%d)", 
						id, index, j,
					    current->myskills.weapons[index].mods[j].level,
					    current->myskills.weapons[index].mods[j].soft_max,
					    current->myskills.weapons[index].mods[j].hard_max)
				}
			}
		}
		//end weapons

		gds_subop_insert_rune(db, current, id);

		QUERY (MYSQL_UPDATECTFSTATS, 
			current->myskills.flag_pickups,
			current->myskills.flag_captures,
			current->myskills.flag_returns,
			current->myskills.flag_kills,
			current->myskills.offense_kills,
			current->myskills.defense_kills,
			current->myskills.assists, id)
	} // end saving

	mysql_query(db, "COMMIT;");

	if (current->ent->client)
		if (current->ent->client->pers.inventory[power_cube_index] > current->ent->client->pers.max_powercubes)
			current->ent->client->pers.inventory[power_cube_index] = current->ent->client->pers.max_powercubes;

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_player_status);
#endif

	if (current->ent->gds_connection_id == current->connection_id)
		current->ent->gds_thread_status = GDS_STATUS_CHARACTER_SAVED;

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&mutex_player_status);
#endif

	mysql_autocommit(db, true);

	return id;
}

qboolean gds_op_load(gds_queue_t *current, MYSQL *db)
{
	int numAbilities, numWeapons, numRunes;
	int i, exists;
	int id;
	edict_t *player = current->ent;
	char escaped[32];
	MYSQL_ROW row;
	MYSQL_RES *result, *result_b;

	if (!db)
	{
		return false;
	}

	if (player->gds_connection_id != current->connection_id)
		return false; // Heh.

	mysql_real_escape_string(db, escaped, current->char_name, strlen(current->char_name));

	QUERY ("CALL CharacterExists(\"%s\", @Exists);", escaped)

	mysql_query (db, "SELECT @Exists;"); 

	GET_RESULT

	exists = atoi(row[0]);

	if (exists == 0)
	{
#ifndef GDS_NOMULTITHREADING
		pthread_mutex_lock(&mutex_player_status);
#endif
		if (player->gds_connection_id == current->connection_id)
			player->gds_thread_status = GDS_STATUS_CHARACTER_DOES_NOT_EXIST;
#ifndef GDS_NOMULTITHREADING
		pthread_mutex_unlock(&mutex_player_status);
#endif
		return false;
	}

	FREE_RESULT

	QUERY ("CALL GetCharID(\"%s\", @PID);", escaped)

	mysql_query (db, "SELECT @PID;"); 

	GET_RESULT

	id = atoi(row[0]);

	FREE_RESULT

	if (exists) // Exists? Then is it able to play?
	{
	    QUERY ("CALL GetCharacterLock(%d, @IsAble);", id)
	    mysql_query (db, "SELECT @IsAble;");
	    GET_RESULT

	    if (row && row[0])
		{
			if (atoi(row[0]) == 0 && !gds_singleserver->value)
			{
#ifndef GDS_NOMULTITHREADING
				pthread_mutex_lock(&mutex_player_status);
#endif
				if (player->gds_connection_id == current->connection_id)
					player->gds_thread_status = GDS_STATUS_ALREADY_PLAYING; // Already playing.
#ifndef GDS_NOMULTITHREADING
				pthread_mutex_unlock(&mutex_player_status);
#endif
				FREE_RESULT
				return false;
			}
		}else
		{
			const char* error = mysql_error(GDS_MySQL);
			if (error)
				gi.dprintf("DB: %s\n", error);
		}

	    FREE_RESULT
	}

	QUERY( "SELECT * FROM userdata WHERE char_idx=%d", id )

	GET_RESULT

	if (row)
	{
		if (row[1])
			strcpy(player->myskills.title, row[1]);

		strcpy(player->myskills.player_name, row[2]);

		if (row[3])
			strcpy(player->myskills.password, row[3]);

		if (row[4])
			strcpy(player->myskills.email, row[4]);

		if (row[5])
			strcpy(player->myskills.owner, row[5]);

		if (row[6])
			strcpy(player->myskills.member_since, row[6]);

		if (row[7])
			strcpy(player->myskills.last_played, row[7]);

		if (row[8])
			player->myskills.total_playtime =  atoi(row[8]);

		if (row[9])
			player->myskills.playingtime = atoi(row[9]);
	}
	else return false;

	FREE_RESULT

	if (player->gds_connection_id != current->connection_id)
		return false; // Don't waste time...

	QUERY( "SELECT COUNT(*) FROM talents WHERE char_idx=%d", id )

	GET_RESULT
	//begin talents
	player->myskills.talents.count = atoi(row[0]);

	FREE_RESULT

	QUERY( "SELECT * FROM talents WHERE char_idx=%d", id )

	GET_RESULT

	for (i = 0; i < player->myskills.talents.count; ++i)
	{
		//don't crash.
        if (i >= MAX_TALENTS)
			return false;

		player->myskills.talents.talent[i].id = atoi(row[1]);
		player->myskills.talents.talent[i].upgradeLevel = atoi(row[2]);
		player->myskills.talents.talent[i].maxLevel = atoi(row[3]);

		row = mysql_fetch_row(result);
		if ( !row )
			break;
	}

	FREE_RESULT
	//end talents

	if (player->gds_connection_id != current->connection_id)
		return false; // Still here.

	QUERY( "SELECT COUNT(*) FROM abilities WHERE char_idx=%d", id )

	GET_RESULT

	//begin abilities
	numAbilities = atoi(row[0]);
	
	FREE_RESULT

	QUERY( "SELECT * FROM abilities WHERE char_idx=%d", id )

	GET_RESULT

	for (i = 0; i < numAbilities; ++i)
	{
		int index;
		index = atoi(row[1]);

		if ((index >= 0) && (index < MAX_ABILITIES))
		{
			player->myskills.abilities[index].level			= atoi(row[2]);
			player->myskills.abilities[index].max_level		= atoi(row[3]);
			player->myskills.abilities[index].hard_max		= atoi(row[4]);
			player->myskills.abilities[index].modifier		= atoi(row[5]);
			player->myskills.abilities[index].disable		= atoi(row[6]);
			player->myskills.abilities[index].general_skill = atoi(row[7]);
			
			row = mysql_fetch_row(result);
			if ( !row )
				break;
		}
	}
	//end abilities

	FREE_RESULT

	if (player->gds_connection_id != current->connection_id)
		return false; // Patient enough...

	QUERY( "SELECT COUNT(*) FROM weapon_meta WHERE char_idx=%d", id )

	GET_RESULT

	//begin weapons
    numWeapons = atoi(row[0]);
	
	FREE_RESULT

	QUERY( "SELECT * FROM weapon_meta WHERE char_idx=%d", id )

	result_b = mysql_store_result(db);
	row = mysql_fetch_row(result_b);

	for (i = 0; i < numWeapons; ++i)
	{
		int index;
		index = atoi(row[1]);

		if ((index >= 0 ) && (index < MAX_WEAPONS))
		{
			int j;
			player->myskills.weapons[index].disable = atoi(row[2]);

			QUERY ("SELECT * FROM weapon_mods WHERE weapon_index=%d AND char_idx=%d", index, id)

			GET_RESULT

			if (row)
			{
				for (j = 0; j < MAX_WEAPONMODS; ++j)
				{
				
					player->myskills.weapons[index].mods[atoi(row[2])].level = atoi(row[3]);
					player->myskills.weapons[index].mods[atoi(row[2])].soft_max = atoi(row[4]);
					player->myskills.weapons[index].mods[atoi(row[2])].hard_max = atoi(row[5]);
				
					row = mysql_fetch_row(result);
					if (!row)
						break;
				}
			}

			FREE_RESULT
		}

		row = mysql_fetch_row(result_b);
		if (!row)
			break;

	}

	mysql_free_result(result_b);
	//end weapons

	// abort loading if player abandoned the server
	if (player->gds_connection_id != current->connection_id)
		return false; 

	//begin runes

	QUERY ("SELECT COUNT(*) FROM runes_meta WHERE char_idx=%d", id)

	GET_RESULT

	numRunes = atoi(row[0]);

	FREE_RESULT

	QUERY( "SELECT * FROM runes_meta WHERE char_idx=%d", id )

	GET_RESULT

	for (i = 0; i < numRunes; ++i)
	{
		int index;
		index = atoi(row[1]);
		if ((index >= 0) && (index < MAX_VRXITEMS))
		{
			int j;
			player->myskills.items[index].itemtype = atoi(row[2]);
			player->myskills.items[index].itemLevel = atoi(row[3]);
			player->myskills.items[index].quantity = atoi(row[4]);
			player->myskills.items[index].untradeable = atoi(row[5]);
			strcpy(player->myskills.items[index].id, row[6]);
			strcpy(player->myskills.items[index].name, row[7]);
			player->myskills.items[index].numMods = atoi(row[8]);
			player->myskills.items[index].setCode = atoi(row[9]);
			player->myskills.items[index].classNum = atoi(row[10]);

			QUERY ("SELECT type, mindex, value, rset FROM runes_mods WHERE rune_index=%d AND char_idx=%d", index, id)

			result_b = mysql_store_result(db);
			
			if (result_b)
				row = mysql_fetch_row(result_b);
			else
				row = NULL;

			int mod = 0;
			while (row && mod < MAX_VRXITEMMODS)
			{
				player->myskills.items[index].modifiers[mod].type = atoi(row[0]);
				player->myskills.items[index].modifiers[mod].index = atoi(row[1]);
				player->myskills.items[index].modifiers[mod].value = atoi(row[2]);
				player->myskills.items[index].modifiers[mod].set = atoi(row[3]);

				row = mysql_fetch_row(result_b);
				mod++;
			}

			mysql_free_result(result_b);
		}

		row = mysql_fetch_row(result);
		if (!row)
			break;
	}

	FREE_RESULT
	//end runes

	if (player->gds_connection_id != current->connection_id)
		return false; // Almost there.


	//*****************************
	//standard stats
	//*****************************

	QUERY("SELECT * FROM point_data WHERE char_idx=%d", id)

	GET_RESULT

	//Exp
	player->myskills.experience =  atoi(row[1]);
	//next_level
	player->myskills.next_level =  atoi(row[2]);
	//Level
	player->myskills.level =  atoi(row[3]);
	//Class number
	player->myskills.class_num = atoi(row[4]);
	//skill points
	player->myskills.speciality_points = atoi(row[5]);
	//credits
	player->myskills.credits = atoi(row[6]);
	//weapon points
	player->myskills.weapon_points = atoi(row[7]);
	//respawn weapon
	player->myskills.respawn_weapon = atoi(row[8]);
	//talent points
	player->myskills.talents.talentPoints = atoi(row[9]);

	FREE_RESULT

	QUERY("SELECT * FROM character_data WHERE char_idx=%d", id)

	GET_RESULT


	//*****************************
	//in-game stats
	//*****************************
	//respawns
	player->myskills.weapon_respawns = atoi(row[1]);
	//health
	player->myskills.current_health = atoi(row[2]);
	//max health
	player->myskills.max_health = atoi(row[3]);
	//armour
	player->myskills.current_armor = atoi(row[4]);
	//max armour
	player->myskills.max_armor = atoi(row[5]);
	//nerfme			(cursing a player maybe?)
	player->myskills.nerfme = atoi(row[6]);

	//*****************************
	//flags
	//*****************************
	//admin flag
	player->myskills.administrator = atoi(row[7]);
	//boss flag
	player->myskills.boss = atoi(row[8]);


	FREE_RESULT

	//*****************************
	//stats
	//*****************************

	QUERY( "SELECT * FROM game_stats WHERE char_idx=%d", id )

	GET_RESULT

	//shots fired
	player->myskills.shots = atoi(row[1]);
	//shots hit
	player->myskills.shots_hit = atoi(row[2]);
	//frags
	player->myskills.frags = atoi(row[3]);
	//deaths
	player->myskills.fragged = atoi(row[4]);
	//number of sprees
	player->myskills.num_sprees = atoi(row[5]);
	//max spree
	player->myskills.max_streak = atoi(row[6]);
	//number of wars
	player->myskills.spree_wars = atoi(row[7]);
	//number of sprees broken
	player->myskills.break_sprees = atoi(row[8]);
	//number of wars broken
	player->myskills.break_spree_wars = atoi(row[9]);
	//suicides
	player->myskills.suicides = atoi(row[10]);
	//teleports			(link this to "use tballself" maybe?)
	player->myskills.teleports = atoi(row[11]);
	//number of 2fers
	player->myskills.num_2fers = atoi(row[12]);

	FREE_RESULT

	QUERY( "SELECT * FROM ctf_stats WHERE char_idx=%d", id)

	GET_RESULT

	//CTF statistics
	player->myskills.flag_pickups =  atoi(row[1]);
	player->myskills.flag_captures =  atoi(row[2]);
	player->myskills.flag_returns =  atoi(row[3]);
	player->myskills.flag_kills =  atoi(row[4]);
	player->myskills.offense_kills =  atoi(row[5]);
	player->myskills.defense_kills =  atoi(row[6]);
	player->myskills.assists =  atoi(row[7]);
	//End CTF

	FREE_RESULT

	//Apply runes
	V_ResetAllStats(player);
	for (i = 0; i < 4; ++i)
		V_ApplyRune(player, &player->myskills.items[i]);

	//Apply health
	if (player->myskills.current_health > MAX_HEALTH(player))
		player->myskills.current_health = MAX_HEALTH(player);

	//Apply armor
	if (player->myskills.current_armor > MAX_ARMOR(player))
		player->myskills.current_armor = MAX_ARMOR(player);
	player->myskills.inventory[body_armor_index] = player->myskills.current_armor;

	//done
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&mutex_player_status);
#endif

	if (player->gds_connection_id == current->connection_id)
	{
		int result_status = vrx_get_login_status(player);
		if (result_status == 0)
		{

			player->gds_thread_status = GDS_STATUS_CHARACTER_LOADED; // You can play! :)
		}
		else
			player->gds_thread_status = result_status;
	}

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&mutex_player_status);
#endif

	return true;
}

void gds_op_saveclose(gds_queue_t *current, MYSQL *db)
{
	int id = gds_op_save(current, db);
	if (id != -1)
		QUERY ("UPDATE userdata SET isplaying = 0 WHERE char_idx = %d;", id)
}

// Start Connection to GDS/MySQL

void gds_create_process_queue()
{
	// gi.dprintf ("DB: Creating thread...");
	int rc = pthread_create(&QueueThread, &attr, gds_process_queue, NULL);
	
	if (rc)
	{
		gi.dprintf(" Failure creating thread! %d\n", rc);
		return;
	}/*else
		gi.dprintf(" Done!\n");*/
	gds_set_thread_running(true);
}

qboolean gds_connect()
{
	int rc;

	gi.dprintf("DB: Initializing connection... ");

	gds_singleserver = gi.cvar("gds_single", "0", 0); // default to multi server using sql.

	const char* database = gi.cvar("gds_dbaddress", DEFAULT_DATABASE, 0)->string;
	const char* user = gi.cvar("gds_dbuser", MYSQL_USER, 0)->string;
	const char* pw = gi.cvar("gds_dbpass", MYSQL_PW, 0)->string;
	const char* dbname = gi.cvar("gds_dbname", MYSQL_DBNAME, 0)->string;

	if (!GDS_MySQL)
	{
		GDS_MySQL = mysql_init(NULL);
		if (mysql_real_connect(GDS_MySQL, database, user, pw, dbname, 0, NULL, 0) == NULL)
		{
			gi.dprintf("Failure: %s\n", mysql_error(GDS_MySQL));
			mysql_close(GDS_MySQL);
			GDS_MySQL = NULL;
			return false;
		}
	}else
	{
		if (GDS_MySQL)
		{
			gi.dprintf("DB: Already connected\n");
		}
		return false;
	}

	/*gds_debug = gi.cvar("gds_debug", "0", 0);*/

	gi.dprintf("Success!\n");
	
#ifndef GDS_NOMULTITHREAD

	pthread_mutex_init(&mutex_gds_queue, NULL);
	pthread_mutex_init(&mutex_player_status, NULL);
    rc = pthread_mutex_init(&mutex_gds_thread_status, NULL);
    if (rc)
		gi.dprintf("mutex creation err: %d", rc);

	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

#endif

	return true;
}

void Mem_PrepareMutexes()
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_init(&MemMutex_Malloc, NULL);
	pthread_mutex_init(&MemMutex_Free, NULL);
#endif
}

qboolean gds_enabled()
{
	return GDS_MySQL != NULL;
}

qboolean vrx_mysql_isloading(edict_t *ent) {
#ifndef GDS_NOMULTITHREADING
    pthread_mutex_lock(&mutex_player_status);
#endif
    int status = ent->gds_thread_status;
#ifndef GDS_NOMULTITHREADING
    pthread_mutex_unlock(&mutex_player_status);
#endif
    return status == GDS_STATUS_CHARACTER_LOADING ||
           status == GDS_STATUS_CHARACTER_LOADED ||
           status == GDS_STATUS_CHARACTER_DOES_NOT_EXIST;
}

qboolean vrx_mysql_saveclose_character(edict_t* player) {
	if (gds_enabled())
	{
		gds_queue_add(player, GDS_SAVECLOSE, -1);
		return true;
	}

	return false;
}

#ifndef GDS_NOMULTITHREADING

void gds_handle_status(edict_t *player)
{
	if (!player)
		return;

	if (!player->inuse)
		return;

	if (!G_IsSpectator(player)) // don't handle plyers that are already logged in!
		return;
		
	if (!GDS_MySQL)
		return;

	pthread_mutex_lock(&mutex_player_status);

	if (player->gds_thread_status == GDS_STATUS_OK)
	{
		pthread_mutex_unlock(&mutex_player_status);
		return;
	}

	switch (player->gds_thread_status)
	{
	case GDS_STATUS_ALREADY_PLAYING:
		safe_centerprintf(player, "You seem to be already playing in this or another server.\nAsk an admin to free your character.\n");
	case GDS_STATUS_CHARACTER_SAVED:
		/*if (player->inuse)
			gi.cprintf(player, PRINT_LOW, "Character saved!\n");*/
		break;
	case GDS_STATUS_CHARACTER_DOES_NOT_EXIST: // does not exist?
		safe_centerprintf(player, "Creating a new Character!\n");
		vrx_create_new_character(player);
		OpenModeMenu(player);
		break;
	case GDS_STATUS_CHARACTER_LOADED:
		safe_centerprintf(player, "Your character was loaded succesfully.");
		OpenModeMenu(player);
		break;
	case GDS_STATUS_OK:
		break;
    case GDS_STATUS_CHARACTER_LOADING:
        return;
	default: // negative value
		CanJoinGame(player, player->gds_thread_status); //Sends out correct message.
	}

	// clear the status.
	player->gds_thread_status = GDS_STATUS_OK;
	pthread_mutex_unlock(&mutex_player_status);
}

void gds_finish_thread()
{
	void *status;
	int rc;

	if (GDS_MySQL)
	{
		gds_queue_add(NULL, GDS_EXITTHREAD, -1);

		gi.dprintf("DB: Finishing thread... ");
		rc = pthread_join(QueueThread, &status);
		gi.dprintf(" Done.\n", rc);

		if (rc)
			gi.dprintf("pthread_join: %d\n", rc);

		rc = pthread_mutex_destroy(&mutex_gds_queue);
		if (rc)
			gi.dprintf("pthread_mutex_destroy: %d\n", rc);

		rc = pthread_mutex_destroy(&mutex_player_status);
		if (rc)
			gi.dprintf("pthread_mutex_destroy: %d\n", rc);

		gds_freequeue_free();
		mysql_close(GDS_MySQL);
		GDS_MySQL = NULL;
	}
}
#else

void gds_finish_thread () {}
void gds_handle_status () {}

#endif


#endif // NO_GDS

void *V_Malloc(size_t Size, int Tag)
{
	void *Memory;

#if (!defined GDS_NOMULTITHREADING) && (!defined NO_GDS)
	pthread_mutex_lock(&MemMutex_Malloc);
#endif
	Memory = gi.TagMalloc (Size, Tag);

#if (!defined GDS_NOMULTITHREADING) && (!defined NO_GDS)
	pthread_mutex_unlock(&MemMutex_Malloc);
#endif

	return Memory;
}

void V_Free(void *mem)
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&MemMutex_Free);
#endif

	gi.TagFree (mem);

#ifndef GDS_NOMULTITHREADING
	pthread_mutex_unlock(&MemMutex_Free);
#endif
}
