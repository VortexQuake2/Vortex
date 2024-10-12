#undef NO_GDS
#include "g_local.h"
#include "v_characterio.h"
#ifndef NO_GDS

#include <mysql.h>
#include "gds.h"
#include "characters/class_limits.h"

#include <pthread.h>

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


// runes

const char *MYSQL_INSERTRMETA = "INSERT INTO runes_meta VALUES (%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,%d,%d);";

const char *MYSQL_INSERTRMOD = "INSERT INTO runes_mods VALUES (%d,%d,%d,%d,%d,%d,%d);";

// update stuff


// MYSQL
MYSQL *GDS_MySQL = NULL;

// Queue. Make certain things unions because they're mutually exclusive.
typedef struct {
    edict_t *ent; /* for most operations. */
    gds_op operation; /* GDS_LOAD, GDS_SAVE, etc... */
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

static gds_queue_t *first = NULL;
static gds_queue_t *last = NULL;

// CVARS
// cvar_t *gds_debug; // Should I actually use this? -az
// if gds_singleserver is set then bypass the isplaying check.
static cvar_t *gds_singleserver;
static cvar_t *gds_serverkey;

// Threading
static pthread_t QueueThread;
static pthread_attr_t attr;
static pthread_mutex_t mutex_gds_queue;
static pthread_mutex_t MemMutex_Free;
static pthread_mutex_t MemMutex_Malloc;
static pthread_mutex_t mutex_gds_thread_status;

static qboolean ThreadRunning;

// Prototypes
int gds_op_save(gds_queue_t *myskills, MYSQL *db);

qboolean gds_op_load(gds_queue_t *current, MYSQL *db);

void gds_op_saveclose(gds_queue_t *current, MYSQL *db);

int gds_op_save_runes(gds_queue_t *current, MYSQL *db);

void gds_create_process_queue();

qboolean gds_process_queue_thread_running();

// Utility Functions

// va used only in the thread.
char *myva(const char *format, ...) {
    va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

    return string;
}

int FindRuneIndex_S(int index, skills_t *myskills) {
    int count = 0;

    for (int i = 0; i < MAX_VRXITEMS; ++i) {
        if (myskills->items[i].itemtype != ITEM_NONE) {
            ++count;
            if (count == index) {
                return i;
            }
        }
    }
    return -1; //just in case something messes up
}

int V_WeaponUpgradeVal_S(skills_t *myskills, int weapnum) {
    //Returns an integer value, usualy from 0-100. Used as a % of maximum upgrades statistic

    float iCount;

    float iMax = iCount = 0.0f;

    if ((weapnum >= MAX_WEAPONS) || (weapnum < 0))
        return -666; //BAD weapon number

    for (int i = 0; i < MAX_WEAPONMODS; ++i) {
        iCount += myskills->weapons[weapnum].mods[i].current_level;
        iMax += myskills->weapons[weapnum].mods[i].soft_max;
    }

    if (iMax == 0)
        return 0;

    float val = (iCount / iMax) * 100;

    return (int) val;
}

int CountWeapons_S(skills_t *myskills) {
    int count = 0;
    for (int i = 0; i < MAX_WEAPONS; ++i) {
        if (V_WeaponUpgradeVal_S(myskills, i) > 0)
            count++;
    }
    return count;
}


int CountRunes_S(skills_t *myskills) {
    int count = 0;

    for (int i = 0; i < MAX_VRXITEMS; ++i) {
        if (myskills->items[i].itemtype != ITEM_NONE)
            ++count;
    }
    return count;
}

int CountAbilities_S(skills_t *myskills) {
    int count = 0;
    for (int i = 0; i < MAX_ABILITIES; ++i) {
        if (!myskills->abilities[i].disable)
            ++count;
    }
    return count;
}

int FindAbilityIndex_S(int index, skills_t *myskills) {
    int count = 0;
    for (int i = 0; i < MAX_ABILITIES; ++i) {
        if (!myskills->abilities[i].disable) {
            ++count;
            if (count == index)
                return i;
        }
    }
    return -1; //just in case something messes up
}

int FindWeaponIndex_S(int index, skills_t *myskills) {
    int count = 0;
    for (int i = 0; i < MAX_WEAPONS; ++i) {
        if (V_WeaponUpgradeVal_S(myskills, i) > 0) {
            count++;
            if (count == index)
                return i;
        }
    }
    return -1; //just in case something messes up
}


// *********************************
// QUEUE functions
// *********************************

void gds_queue_push(gds_queue_t *current, qboolean lock) {
    if (lock)
        pthread_mutex_lock(&mutex_gds_queue);

    if (current) {
        last->next = current;
        last = current;
        last->next = NULL;
    }

    if (lock)
        pthread_mutex_unlock(&mutex_gds_queue);
}

/* queue mutex must be locked before calling this
 * will make 'last' into a new operation
 */
void gds_queue_make_operation() {
    if (!first) {
        first = vrx_malloc(sizeof(gds_queue_t), TAG_GAME);
        memset(first, 0, sizeof(gds_queue_t));
        last = first;
        return;
    }

    gds_queue_t *newest = vrx_malloc(sizeof(gds_queue_t), TAG_GAME);
    gds_queue_push(newest, false); // false since we're already locked
}

void gds_queue_add(edict_t *ent, gds_op operation, int index) {
    if ((!ent || !ent->client) && operation != GDS_DISCONNECT) {
        gi.dprintf("gds_queue_add: Null Entity or Client!\n");
    }

    if (!GDS_MySQL) {
        safe_cprintf(ent, PRINT_HIGH, "It seems that there's no connection to the database. Contact an admin ASAP.\n");
        return;
    }

    if (operation == GDS_LOAD) {
        // if they don't match, it means someone replaced the owner of this entity.
        assert(ent);
        ent->gds_connection_load_id = ent->gds_connection_id;
    }

    if (operation == GDS_DISCONNECT && ent) {
        //if (gds_debug->value)
        gi.dprintf("gds_queue_add: Called with an entity on an exit thread operation?\n");
    }

    if (operation == GDS_STASH_STORE) {
        if (index < 0 || index >= MAX_VRXITEMS) {
            gi.dprintf("called GDS_STASH_STORE with an invalid item index (%d)\n", index);
            return;
        }
    }

    if (operation == GDS_STASH_TAKE) {
        if (index < 0) {
            gi.dprintf("called GDS_STASH_STARE with negative index (%d)\n", index);
            return;
        }
    }

    pthread_mutex_lock(&mutex_gds_queue);
    gds_queue_make_operation();

    last->ent = ent;
    last->next = NULL; // make sure last node has null pointer
    last->operation = operation;
    last->connection_id = ent ? ent->gds_connection_id : 0;

    if (ent) {
        assert(sizeof last->char_name == sizeof ent->client->pers.netname);
        strcpy_s(last->char_name, sizeof last->char_name, ent->client->pers.netname);
    }

    if (operation == GDS_STASH_TAKE) {
        last->stash_index = index;
    } else if (operation == GDS_STASH_STORE) {
        // remove the item from the player.
        // there's a chance we could lose this data if the server crashes
        // before this data is committed
        // but oh well. -az
        assert(ent);
        last->item = ent->myskills.items[index];
        memset(&ent->myskills.items[index], 0, sizeof(item_t));
    } else if (operation == GDS_SAVE ||
               operation == GDS_SAVECLOSE ||
               operation == GDS_SAVERUNES) {
        memcpy(&last->myskills, &ent->myskills, sizeof(skills_t));
    } else if (operation == GDS_STASH_CLOSE) {
        if (ent)
            last->gds_id = -1;
        else {
            last->gds_id = index;
        }
    } else if (operation == GDS_STASH_GET_PAGE) {
        last->page_index = index;
    }

    // if it doesn't exist, create the process queue.
    if (!gds_process_queue_thread_running())
        gds_create_process_queue();

    pthread_mutex_unlock(&mutex_gds_queue);
}

void gds_queue_add_setowner(edict_t *ent, char *charname, char *masterpw, qboolean reset) {
    pthread_mutex_lock(&mutex_gds_queue);
    gds_queue_make_operation();

    last->ent = ent;
    last->operation = GDS_SET_OWNER;
    last->connection_id = ent->gds_connection_id;
    strcpy_s(last->owner_name, sizeof last->owner_name, charname);
    strcpy_s(last->char_name, sizeof last->char_name, ent->client->pers.netname);

    // these verifications should be done by Cmd_SetOwner_f
    // if (strlen(masterpw) < sizeof last->masterpw)
    strcpy_s(last->masterpw, sizeof last->masterpw, masterpw);
    // else {}

    if (!gds_process_queue_thread_running())
        gds_create_process_queue();

    pthread_mutex_unlock(&mutex_gds_queue);
}

gds_queue_t *gds_queue_pop_first() {
    gds_queue_t *node;

    pthread_mutex_lock(&mutex_gds_queue);
    node = first; // save node on top to delete

    if (first && first->next) // does our node have something to do?
        first = first->next; // set first to next node
    else {
        first = NULL;
        last = NULL;
    }

    // give me the assgined entity of the first node
    // node must be freed by caller
    pthread_mutex_unlock(&mutex_gds_queue);
    return node;
}


void query(MYSQL *db, const char *a, ...) {
    va_list args;
    va_start(args, a);

    char format[1024];
    vsnprintf(format, sizeof format, a, args);
    if (mysql_query(db, format)) {
        gi.dprintf("DB: %s", mysql_error(db));
    };
    va_end(args);
}

void run(MYSQL *db, const char *a) {
    if (mysql_query(db, a)) {
        gi.dprintf("DB: %s", mysql_error(db));
    };
}

#define DECLARE_ESCAPE(varname, src) char varname[sizeof (src) * 2];\
mysql_real_escape_string(db, varname, src, strlen(src));

// *********************************
// Database Thread
// *********************************

qboolean gds_process_queue_thread_running() {
    qboolean temp;
    pthread_mutex_lock(&mutex_gds_thread_status);

    temp = ThreadRunning;

    pthread_mutex_unlock(&mutex_gds_thread_status);
    return temp;
}

void gds_set_thread_running(qboolean running) {
    pthread_mutex_lock(&mutex_gds_thread_status);
    ThreadRunning = running;
    pthread_mutex_unlock(&mutex_gds_thread_status);
}


void gds_op_stash_open(gds_queue_t *current, MYSQL *db);

void gds_op_stash_store(gds_queue_t *current, MYSQL *db);

void gds_op_stash_take(gds_queue_t *current, MYSQL *db);

void gds_op_stash_close(gds_queue_t *current, MYSQL *db);

void gds_op_stash_get_page(gds_queue_t *current, MYSQL *db);


void gds_op_set_owner(gds_queue_t *current, MYSQL *db);

void *gds_process_queue(void *unused) {
    gds_queue_t *current = gds_queue_pop_first();
    qboolean exit = false;

    while (current && !exit) {
        if (!current) {
            gds_set_thread_running(false);
            pthread_exit(NULL);
            continue;
        }

        switch (current->operation) {
            case GDS_LOAD:
                gds_op_load(current, GDS_MySQL);
                break;
            case GDS_SAVE:
                gds_op_save(current, GDS_MySQL);
                break;
            case GDS_SAVECLOSE:
                gds_op_saveclose(current, GDS_MySQL);
                break;
            case GDS_SAVERUNES:
                gds_op_save_runes(current, GDS_MySQL);
                break;
            case GDS_STASH_OPEN:
                gds_op_stash_open(current, GDS_MySQL);
                break;
            case GDS_STASH_STORE:
                gds_op_stash_store(current, GDS_MySQL);
                break;
            case GDS_STASH_TAKE:
                gds_op_stash_take(current, GDS_MySQL);
                break;
            case GDS_STASH_CLOSE:
                gds_op_stash_close(current, GDS_MySQL);
                break;
            case GDS_STASH_GET_PAGE:
                gds_op_stash_get_page(current, GDS_MySQL);
                break;
            case GDS_SET_OWNER:
                gds_op_set_owner(current, GDS_MySQL);
                break;
            case GDS_DISCONNECT: {
                char esc_key[sizeof (gds_serverkey->string) * 2];
                mysql_real_escape_string(GDS_MySQL, esc_key, gds_serverkey->string, strlen(gds_serverkey->string));
                query(GDS_MySQL, "CALL NotifyServerStatus('%s', 0);", esc_key);
                exit = true;
            }
                break;
            default: {
                gi.dprintf("gds_process_queue: Unknown operation %d\n", current->operation);
            }
        }

        vrx_free(current);
        current = gds_queue_pop_first();
    }

    gds_set_thread_running(false);
    pthread_exit(NULL);
    return NULL;
}

// *********************************
// MYSQL functions
// *********************************


int gds_get_id(const char *name, MYSQL *db) {
    char escaped[32];

    mysql_real_escape_string(db, escaped, name, strlen(name));

    query(db, "CALL GetCharID(\"%s\", @PID);", escaped);
    query(db, "SELECT @PID;");

    MYSQL_RES *result = mysql_store_result(db);

    if (result != NULL) {
        MYSQL_ROW row = mysql_fetch_row(result);
        int id = -1;
        if (row[0]) {
            id = strtol(row[0], NULL, 10);
        }

        mysql_free_result(result);
        return id;
    }

    return -1;
}


int gds_get_owner_id(char *charname, MYSQL *db) {
    char escaped[64];

    mysql_real_escape_string(db, escaped, charname, strlen(charname));
    query(db, "select char_idx from userdata "
          "where playername = (select owner from userdata where playername = '%s') "
          "or (playername = '%s' and email is not null and LENGTH(email) > 0)",
          escaped, escaped
    );

    MYSQL_RES *result = mysql_store_result(db);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (!row) {
        mysql_free_result(result);
        return -1;
    }

    int id = strtol(row[0], NULL, 10);
    mysql_free_result(result);
    return id;
}

void gds_subop_stash_get_page(int owner_id, item_t *page, int items_per_page, int page_index, MYSQL *db) {
    memset(page, 0, sizeof(item_t) * items_per_page);

    const int start_index = page_index * items_per_page;
    const int end_index = start_index + items_per_page;

    mysql_query(db, "BEGIN TRANSACTION");

    // reader notice: scoping like this prevents using the variables from the wrong context.
    {
        query(db, "select stash_index, itemtype, itemlevel, quantity, untradeable, "
              "id, name, nummods, setcode, classnum "
              "from stash_runes_meta where char_idx=%d "
              "and stash_index >= %d and stash_index < %d",
              owner_id, start_index, end_index
        );

        MYSQL_RES *item_head_result = mysql_store_result(db);

        if (item_head_result == NULL) {
            mysql_query(db, "COMMIT");
            return;
        }

        MYSQL_ROW item_head_row = mysql_fetch_row(item_head_result);

        int safeguard_items = 0;
        while (item_head_row && safeguard_items < items_per_page) {
            const int page_row_index = strtol(item_head_row[0], NULL, 10) - start_index;
            assert(page_row_index >= 0 && page_row_index < items_per_page);

            item_t *item = &page[page_row_index];

            item->itemtype = strtol(item_head_row[1], NULL, 10);
            item->itemLevel = strtol(item_head_row[2], NULL, 10);
            item->quantity = strtol(item_head_row[3], NULL, 10);
            item->untradeable = strtol(item_head_row[4], NULL, 10);
            strncpy_s(item->id, sizeof item->id, item_head_row[5], sizeof item->id - 1);
            strncpy_s(item->name, sizeof item->name, item_head_row[6], sizeof item->name - 1);
            item->numMods = strtol(item_head_row[7], NULL, 10);
            item->setCode = strtol(item_head_row[8], NULL, 10);
            item->classNum = strtol(item_head_row[9], NULL, 10);

            safeguard_items++;
            item_head_row = mysql_fetch_row(item_head_result);
        }

        mysql_free_result(item_head_result);
    }

    // we fetch everything in one query.
    {
        query(db, "select stash_index, rune_mod_index, type, mod_index, value, rset "
              "from stash_runes_mods where char_idx = %d "
              "and stash_index >= %d and stash_index < %d order by stash_index", owner_id, start_index, end_index
        );

        MYSQL_RES *item_modifier_result = mysql_store_result(db);
        MYSQL_ROW item_modifier_row = mysql_fetch_row(item_modifier_result);
        int last_row = 0;
        int mod_index = 0;
        while (item_modifier_row) {
            const int page_row_index = strtol(item_modifier_row[0], NULL, 10) - start_index;
            assert(page_row_index >= 0 && page_row_index < items_per_page);

            if (last_row != page_row_index) {
                mod_index = 0;
                last_row = page_row_index;
            }

            if (mod_index >= MAX_VRXITEMMODS) {
                item_modifier_row = mysql_fetch_row(item_modifier_result);
                continue;
            }

            imodifier_t *mod = &page[page_row_index].modifiers[mod_index];

            mod->type = strtol(item_modifier_row[2], NULL, 10);
            mod->index = strtol(item_modifier_row[3], NULL, 10);
            mod->value = strtol(item_modifier_row[4], NULL, 10);
            mod->set = strtol(item_modifier_row[5], NULL, 10);

            item_modifier_row = mysql_fetch_row(item_modifier_result);
            mod_index++;
        }
        mysql_free_result(item_modifier_result);
    }

    mysql_query(db, "COMMIT");
}


void gds_op_set_owner(gds_queue_t *current, MYSQL *db) {
    int new_owner_id = gds_get_id(current->owner_name, db);

    event_owner_error_t *evt = vrx_malloc(sizeof(event_owner_error_t), TAG_GAME);
    assert(sizeof evt->owner_name == sizeof current->owner_name);
    strcpy_s(evt->owner_name, sizeof evt->owner_name, current->owner_name);
    evt->ent = current->ent;
    evt->connection_id = current->connection_id;

    if (new_owner_id == -1) {
        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_owner_nonexistent,
                      .data = evt
                  });
        return;
    }

    int id = gds_get_id(current->char_name, db);

    DECLARE_ESCAPE(esc_ownername, current->owner_name);
    DECLARE_ESCAPE(esc_password, current->masterpw);

    query(db,
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
          id
    );

    uint64_t rows = mysql_affected_rows(db);
    if (rows == 0) {
        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_owner_bad_password,
                      .data = evt
                  });
    } else {
        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_owner_success,
                      .data = evt
                  });
    }
}

void gds_op_stash_get_page(gds_queue_t *current, MYSQL *db) {
    int owner_id = gds_get_owner_id(current->char_name, db);

    if (owner_id == -1) {
        stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_stash_no_owner,
                      .data = notif
                  });

        return;
    }

    stash_page_event_t *evt = vrx_malloc(sizeof(stash_page_event_t), TAG_GAME);
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

    defer_add(&current->ent->client->defers, (defer_t){
                  .function = vrx_notify_open_stash,
                  .data = evt
              });
}

/* opening the stash is a surprisingly complicated transaction. */
void gds_op_stash_open(gds_queue_t *current, MYSQL *db) {
    int owner_id = gds_get_owner_id(current->char_name, db);

    if (owner_id == -1) {
        stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_stash_no_owner,
                      .data = notif
                  });

        return;
    }

    mysql_autocommit(db, false);
    mysql_query(db, "START TRANSACTION");

    /* check if the character is locked */
    query(db, "select lock_char_id from stash where char_idx=%d for update", owner_id);;

    MYSQL_RES *lock_status_result = mysql_store_result(db);
    qboolean locked = false;

    if (lock_status_result != NULL) {
        MYSQL_ROW lock_status_row = mysql_fetch_row(lock_status_result);
        if (lock_status_row[0] != NULL)
            locked = true;
    } else {
        gi.dprintf("row for character %d does not exist.\n", owner_id);
        locked = true;
    }

    mysql_free_result(lock_status_result);

    if (locked) {
        stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_stash_locked,
                      .data = notif
                  });
    } else {
        /* lock the stash */
        int id = gds_get_id(current->char_name, db);
        query(db, "update stash set lock_char_id = %d where char_idx=%d", id, owner_id);;

        /* fetch a page and open the stash */
        stash_page_event_t *notif = vrx_malloc(sizeof(stash_page_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;
        notif->gds_owner_id = owner_id;
        notif->pagenum = 0;

        gds_subop_stash_get_page(
            owner_id,
            notif->page,
            sizeof notif->page / sizeof(item_t),
            0,
            db
        );

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_open_stash,
                      .data = notif
                  });
    }

    mysql_query(db, "COMMIT");
    mysql_autocommit(db, true);
}

void gds_op_stash_close(gds_queue_t *current, MYSQL *db) {
    const int owner_id = current->ent ? gds_get_owner_id(current->char_name, db) : current->gds_id;

    if (owner_id == -1 && current->ent) {
        stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_stash_no_owner,
                      .data = notif
                  });

        return;
    }

    query(db, "update stash set lock_char_id = NULL where char_idx=%d", owner_id);;
}

void gds_op_stash_store(gds_queue_t *current, MYSQL *db) {
    int owner_id = gds_get_owner_id(current->char_name, db);

    if (owner_id == -1) {
        stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
        notif->ent = current->ent;
        notif->gds_connection_id = current->connection_id;

        defer_add(&current->ent->client->defers, (defer_t){
                      .function = vrx_notify_stash_no_owner,
                      .data = notif
                  });

        return;
    }

    mysql_autocommit(db, false);

    mysql_query(db, "START TRANSACTION");

    /* find a reasonable stash index for our rune */
    int index = 0; {
        query(db, "select stash_index from stash_runes_meta "
              "where char_idx=%d "
              "order by stash_index", owner_id
        );

        MYSQL_RES *res = mysql_store_result(db);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            int index_result = strtol(row[0], NULL, 10);

            // we found a free slot
            if (index < index_result)
                break;

            // continue looking
            index++;
        }

        mysql_free_result(res);
    }

    item_t item = current->item;

    query(db, "INSERT INTO stash_runes_meta "
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
          item.classNum
    );

    for (int j = 0; j < MAX_VRXITEMMODS; ++j) {
        // TYPE_NONE, so skip it
        if (item.modifiers[j].type == 0 ||
            item.modifiers[j].value == 0)
            continue;

        query(db, "INSERT INTO stash_runes_mods "
              "VALUES (%d,%d,%d,%d,%d,%d,%d)",
              owner_id, index, j,
              item.modifiers[j].type,
              item.modifiers[j].index,
              item.modifiers[j].value,
              item.modifiers[j].set
        );
    }

    mysql_query(db, "COMMIT");
    mysql_autocommit(db, true);
}


void gds_op_stash_take(gds_queue_t *current, MYSQL *db) {
    int owner_id = gds_get_owner_id(current->char_name, db);
    int id = gds_get_id(current->char_name, db);

    /* check if we're the owners of the stash before taking */
    {
        query(db, "select char_idx from stash where lock_char_id=%d", id);;

        MYSQL_RES *res = mysql_store_result(db);
        MYSQL_ROW row = mysql_fetch_row(res);
        if (!row || strtol(row[0], NULL, 10) != owner_id) {
            stash_event_t *notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
            notif->ent = current->ent;
            notif->gds_connection_id = current->connection_id;

            defer_add(&current->ent->client->defers, (defer_t){
                          .function = vrx_notify_stash_locked,
                          .data = notif
                      });

            mysql_free_result(res);
            return;
        }

        mysql_free_result(res);
    }

    /* take it */
    item_t item = {0};
    {
        query(db, "select stash_index, itemtype, itemlevel, quantity, untradeable, "
              "id, name, nummods, setcode, classnum "
              "from stash_runes_meta where char_idx=%d "
              "and stash_index = %d",
              owner_id, current->stash_index
        );

        MYSQL_RES *item_head_result = mysql_store_result(db);
        MYSQL_ROW item_head_row = mysql_fetch_row(item_head_result);

        if (item_head_row) {
            item.itemtype = strtol(item_head_row[1], NULL, 10);
            item.itemLevel = strtol(item_head_row[2], NULL, 10);
            item.quantity = strtol(item_head_row[3], NULL, 10);
            item.untradeable = strtol(item_head_row[4], NULL, 10);
            strncpy_s(item.id, sizeof item.id, item_head_row[5], sizeof item.id - 1);
            strncpy_s(item.name, sizeof item.name, item_head_row[6], sizeof item.name - 1);
            item.numMods = strtol(item_head_row[7], NULL, 10);
            item.setCode = strtol(item_head_row[8], NULL, 10);
            item.classNum = strtol(item_head_row[9], NULL, 10);
        } else {
            // TODO: defer "the item has been removed already" -- this shouldn't happen, however.
        }

        mysql_free_result(item_head_result);
    } {
        query(db, "select stash_index, rune_mod_index, type, mod_index, value, rset "
              "from stash_runes_mods where char_idx = %d "
              "and stash_index = %d", owner_id, current->stash_index
        );

        MYSQL_RES *item_modifier_result = mysql_store_result(db);
        MYSQL_ROW item_modifier_row = mysql_fetch_row(item_modifier_result);
        int mod_index = 0;
        while (item_modifier_row && mod_index < MAX_VRXITEMMODS) {
            /*const int modn = strtol(item_modifier_row[1], NULL, 10);
            assert(modn >= 0 && modn < MAX_VRXITEMMODS);*/

            imodifier_t *mod = &item.modifiers[mod_index];

            mod->type = strtol(item_modifier_row[2], NULL, 10);
            mod->index = strtol(item_modifier_row[3], NULL, 10);
            mod->value = strtol(item_modifier_row[4], NULL, 10);
            mod->set = strtol(item_modifier_row[5], NULL, 10);

            item_modifier_row = mysql_fetch_row(item_modifier_result);
            mod_index++;
        }

        mysql_free_result(item_modifier_result);
    }

    query(db, "delete from stash_runes_meta where stash_index=%d and char_idx=%d", current->stash_index, owner_id);
    query(db, "delete from stash_runes_mods where stash_index=%d and char_idx=%d", current->stash_index, owner_id);

    stash_taken_event_t *evt = vrx_malloc(sizeof(stash_taken_event_t), TAG_GAME);
    evt->ent = current->ent;
    evt->gds_connection_id = current->connection_id;
    vrx_item_copy(&item, &evt->taken);

    assert(sizeof evt->requester == sizeof current->char_name);
    strcpy_s(evt->requester, sizeof evt->requester, current->char_name);

    defer_add(&current->ent->client->defers, (defer_t){
                  .function = vrx_notify_stash_taken,
                  .data = evt
              });
}

void gds_subop_insert_rune(MYSQL *db, gds_queue_t *current, int id) {
    //begin runes
    int numRunes = CountRunes_S(&current->myskills);
    for (int i = 0; i < numRunes; ++i) {
        int index = FindRuneIndex_S(i + 1, &current->myskills);
        if (index != -1) {
            item_t item = current->myskills.items[index];

            query(db, MYSQL_INSERTRMETA, id,
                  index,
                  item.itemtype,
                  item.itemLevel,
                  item.quantity,
                  item.untradeable,
                  item.id,
                  item.name,
                  item.numMods,
                  item.setCode,
                  item.classNum
            );

            for (int j = 0; j < MAX_VRXITEMMODS; ++j) {
                // TYPE_NONE, so skip it
                if (item.modifiers[j].type == 0 ||
                    item.modifiers[j].value == 0)
                    continue;

                query(db, MYSQL_INSERTRMOD, id, index, j,
                      item.modifiers[j].type,
                      item.modifiers[j].index,
                      item.modifiers[j].value,
                      item.modifiers[j].set
                );
            }
        }
    }
    //end runes
}

// GDS_Save except it only deals with runes.
int gds_op_save_runes(gds_queue_t *current, MYSQL *db) {
    const int id = gds_get_id(current->myskills.player_name, db);
    mysql_autocommit(db, false);

    mysql_query(db, "START TRANSACTION");

    query(db, "DELETE FROM runes_meta WHERE char_idx=%d", id);
    query(db, "DELETE FROM runes_mods WHERE char_idx=%d", id);

    gds_subop_insert_rune(db, current, id);

    mysql_query(db, "COMMIT"); // hopefully this will avoid shit breaking
    mysql_autocommit(db, true);

    return 0;
}

int gds_op_save(gds_queue_t *current, MYSQL *db) {
    int i;
    int id; // character ID
    const int numAbilities = CountAbilities_S(&current->myskills);
    const int numWeapons = CountWeapons_S(&current->myskills);
    MYSQL_ROW row;
    MYSQL_RES *result;

    if (!db) {
        /*		if (gds_debug->value)
                    gi.dprintf("DB: NULL database (gds_op_save())\n"); */
        return -1;
    }

    mysql_autocommit(db, false);

    DECLARE_ESCAPE(esc_pname, current->myskills.player_name);

    query(db, "CALL CharacterExists(\"%s\", @exists);", esc_pname);

    run(db, "SELECT @exists;");

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    if (!strcmp(row[0], "0"))
        i = 0;
    else
        i = 1;

    mysql_free_result(result);

    if (!i) // does not exist
    {
        // Create initial database.
        gi.dprintf("DB: Creating character \"%s\"!\n", current->myskills.player_name);
        query(db, "CALL FillNewChar(\"%s\");", esc_pname);
    }

    id = gds_get_id(current->myskills.player_name, db); {
        // real saving
        mysql_query(db, "START TRANSACTION");
        // reset tables (remove records for reinsertion)
        query(db, "CALL ResetTables(\"%s\");", esc_pname);;

        /* do some escaping, just in case. */
        DECLARE_ESCAPE(esc_title, current->myskills.title);
        DECLARE_ESCAPE(esc_password, current->myskills.password);
        DECLARE_ESCAPE(esc_masterpw, current->myskills.email);
        DECLARE_ESCAPE(esc_owner, current->myskills.email);

        query(db, "UPDATE userdata SET "
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
              current->myskills.playingtime, id);

        // talents
        for (i = 0; i < current->myskills.talents.count; ++i) {
            query(db, "INSERT INTO talents (char_idx, id, upgrade_level, max_level) "
                  "VALUES (%d,%d,%d,%d)",
                  id,
                  current->myskills.talents.talent[i].id,
                  current->myskills.talents.talent[i].upgradeLevel,
                  current->myskills.talents.talent[i].maxLevel);
        }

        // abilities

        for (i = 0; i < numAbilities; ++i) {
            int index = FindAbilityIndex_S(i + 1, &current->myskills);
            if (index != -1) {
                query(
                    db,
                    "INSERT INTO abilities (char_idx, aindex, level, max_level, hard_max, modifier, disable, general_skill) "
                    " VALUES (%d,%d,%d,%d,%d,%d,%d,%d)", id, index,
                    current->myskills.abilities[index].level,
                    current->myskills.abilities[index].max_level,
                    current->myskills.abilities[index].hard_max,
                    current->myskills.abilities[index].modifier,
                    (int) current->myskills.abilities[index].disable,
                    (int) current->myskills.abilities[index].general_skill);
            }
        }
        // gi.dprintf("saved abilities");

        //*****************************
        //in-game stats
        //*****************************

        query(db, "UPDATE character_data SET "
              "respawns=%d, "
              "health=%d, "
              "maxhealth=%d, "
              "armour=%d, "
              "maxarmour=%d, "
              "nerfme=%d, "
              "adminlevel=%d, "
              "bosslevel=%d, "
              "prestigelevel=%d, "
              "prestigepoints=%d "
              "WHERE char_idx=%d;",
              current->myskills.weapon_respawns,
              current->myskills.current_health,
              MAX_HEALTH(current->ent),
              current->ent->client ? current->ent->client->pers.inventory[body_armor_index] : 0,
              MAX_ARMOR(current->ent),
              current->myskills.nerfme,
              current->myskills.administrator, // flags
              current->myskills.boss,
              current->myskills.prestige.total,
              current->myskills.prestige.points,
              id
        );

        //*****************************
        //stats
        //*****************************

        query(db, "UPDATE game_stats SET "
              "shots=%d, "
              "shots_hit=%d, "
              "frags=%d, "
              "fragged=%d, "
              "num_sprees=%d, "
              "max_streak=%d, "
              "spree_wars=%d, "
              "broken_sprees=%d, "
              "broken_spreewars=%d, "
              "suicides=%d, "
              "teleports=%d, "
              "num_2fers=%d "
              "WHERE char_idx=%d;",
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
              current->myskills.num_2fers, id
        );

        //*****************************
        //standard stats
        //*****************************

        query(db, "UPDATE point_data SET "
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
              current->myskills.talents.talentPoints, id
        );

        //begin weapons
        for (i = 0; i < numWeapons; ++i) {
            int index = FindWeaponIndex_S(i + 1, &current->myskills);
            if (index != -1) {
                int j;
                query(db, "INSERT INTO weapon_meta VALUES (%d,%d,%d)",
                      id,
                      index,
                      current->myskills.weapons[index].disable
                );

                for (j = 0; j < MAX_WEAPONMODS; ++j) {
                    query(db, "INSERT INTO weapon_mods VALUES (%d,%d,%d,%d,%d,%d)",
                          id, index, j,
                          current->myskills.weapons[index].mods[j].level,
                          current->myskills.weapons[index].mods[j].soft_max,
                          current->myskills.weapons[index].mods[j].hard_max
                    );
                }
            }
        }
        //end weapons

        gds_subop_insert_rune(db, current, id);

        query(db, "UPDATE ctf_stats SET "
              " flag_pickups=%d, "
              " flag_captures=%d, "
              " flag_returns=%d, "
              " flag_kills=%d, "
              " offense_kills=%d, "
              " defense_kills=%d, "
              " assists=%d WHERE char_idx=%d",
              current->myskills.flag_pickups,
              current->myskills.flag_captures,
              current->myskills.flag_returns,
              current->myskills.flag_kills,
              current->myskills.offense_kills,
              current->myskills.defense_kills,
              current->myskills.assists, id);

        query(db, "INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
              id, PRESTIGE_CREDITS, 0, current->myskills.prestige.creditLevel);

        query(db, "INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
              id, PRESTIGE_ABILITY_POINTS, 0, current->myskills.prestige.abilityPoints);

        query(db, "INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
              id, PRESTIGE_WEAPON_POINTS, 0, current->myskills.prestige.weaponPoints);

        // softmax bump points - param is the ab index, level is the bump
        for (i = 0; i < MAX_ABILITIES; i++) {
            if (current->myskills.prestige.softmaxBump[i] > 0) {
                query(db, "INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
                      id, PRESTIGE_SOFTMAX_BUMP, i, current->myskills.prestige.softmaxBump[i]);
            }
        }

        // class skills - param is the ab index, level is always 0
        for (i = 0; i < MAX_ABILITIES; i++) {
            if (vrx_prestige_has_ability(&current->myskills.prestige, i)) {
                query(db, "INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
                      id, PRESTIGE_CLASS_SKILL, i, 0);
            }
        }
    } // end saving

    mysql_query(db, "COMMIT;");

    // TODO: defer this
    if (current->ent->client)
        if (current->ent->client->pers.inventory[power_cube_index] > current->ent->client->pers.max_powercubes)
            current->ent->client->pers.inventory[power_cube_index] = current->ent->client->pers.max_powercubes;


    mysql_autocommit(db, true);

    return id;
}

struct evt_character_locked_t {
    edict_t *ent;
    int connection_id;
};

struct evt_character_loaded_t {
    edict_t *ent;
    skills_t sk;
    int connection_id;
};


void vrx_notify_character_locked(void *args) {
    struct evt_character_locked_t *evt = args;
    if (evt->ent->gds_connection_id == evt->connection_id && evt->ent->inuse) {
        gi.cprintf(evt->ent, PRINT_HIGH, "Character is locked.\nPlease try again later.\n");

        evt->ent->gds_connection_load_id = -1;
    }
}

void vrx_notify_start_character_creation(void *args) {
    struct evt_character_locked_t *evt = args;
    if (evt->ent->gds_connection_id == evt->connection_id && evt->ent->inuse) {
        vrx_create_new_character(evt->ent);
        vrx_open_mode_menu(evt->ent);

        evt->ent->gds_connection_load_id = -1;
    }
}

void vrx_notify_join_game(void *args) {
    struct evt_character_loaded_t *evt = args;
    edict_t *ent = evt->ent;

    if (ent->gds_connection_id != evt->connection_id || !ent->inuse) {
        // this is a stale event
        return;
    }

    skills_t sk = evt->sk;
    ent->myskills = evt->sk;

    vrx_runes_unapply(ent);
    for (int i = 0; i < 4; ++i)
        vrx_runes_apply(ent, &sk.items[i]);

    //Apply health
    if (sk.current_health > MAX_HEALTH(ent))
        sk.current_health = MAX_HEALTH(ent);

    //Apply armor
    if (sk.current_armor > MAX_ARMOR(ent))
        sk.current_armor = MAX_ARMOR(ent);
    sk.inventory[body_armor_index] = sk.current_armor;

    //done
    ent->gds_connection_load_id = -1;
    int result_status = vrx_get_login_status(ent);
    if (result_status < 0) {
        vrx_print_login_status(ent, result_status);
    } else {
        vrx_open_mode_menu(ent);
    }
}

qboolean gds_op_load(gds_queue_t *current, MYSQL *db) {
    int numAbilities, numWeapons, numRunes;
    int i, exists;
    int id;
    edict_t *player = current->ent;
    char escaped[32];
    MYSQL_ROW row;
    MYSQL_RES *result, *result_b;
    skills_t sk = {0};

    if (!db) {
        return false;
    }

    mysql_real_escape_string(db, escaped, current->char_name, strlen(current->char_name));

    query(db, "CALL CharacterExists(\"%s\", @Exists);", escaped);

    mysql_query(db, "SELECT @Exists");

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    exists = strtol(row[0], NULL, 10);

    if (exists == 0) {
        // create a defer for this
        struct evt_character_locked_t *evt = vrx_malloc(sizeof(struct evt_character_locked_t), TAG_GAME);
        evt->ent = player;
        evt->connection_id = current->connection_id;

        defer_add(
            &player->client->defers,
            (defer_t){
                .function = vrx_notify_start_character_creation,
                .data = evt
            });


        return false;
    }

    mysql_free_result(result);

    query(db, "CALL GetCharID(\"%s\", @PID);", escaped);

    mysql_query(db, "SELECT @PID");

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    id = strtol(row[0], NULL, 10);

    mysql_free_result(result);

    if (exists) // Exists? Then is it able to play?
    {
        DECLARE_ESCAPE(esc_key, gds_serverkey->string);
        query(db, "CALL GetCharacterLock(%d, '%s', @IsAble)", id, esc_key);
        mysql_query(db, "SELECT @IsAble");
        result = mysql_store_result(db);
        row = mysql_fetch_row(result);

        if (row && row[0]) {
            if (strtol(row[0], NULL, 10) == 0 && !gds_singleserver->value) {
                struct evt_character_locked_t *evt = vrx_malloc(sizeof(struct evt_character_locked_t), TAG_GAME);
                evt->ent = player;
                evt->connection_id = current->connection_id;

                defer_add(
                    &player->client->defers,
                    (defer_t){
                        .function = vrx_notify_character_locked,
                        .data = evt
                    });

                mysql_free_result(result);
                return false;
            }
        } else {
            const char *error = mysql_error(GDS_MySQL);
            if (error)
                gi.dprintf("DB: %s\n", error);
        }

        mysql_free_result(result);
    }

    query(db, "SELECT char_idx, "
          "title, "
          "playername, "
          "password, "
          "email, "
          "owner, "
          "member_since, "
          "last_played, "
          "playtime_total, "
          "playingtime "
          "FROM userdata WHERE char_idx=%d",
          id
    );

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    if (row) {
        if (row[1])
            strcpy_s(sk.title, sizeof sk.title, row[1]);

        strcpy_s(sk.player_name, sizeof sk.player_name, row[2]);

        if (row[3])
            strcpy_s(sk.password, sizeof sk.password, row[3]);

        if (row[4])
            strcpy_s(sk.email, sizeof sk.email, row[4]);

        if (row[5])
            strcpy_s(sk.owner, sizeof sk.owner, row[5]);

        if (row[6])
            strcpy_s(sk.member_since, sizeof sk.member_since, row[6]);

        if (row[7])
            strcpy_s(sk.last_played, sizeof sk.last_played, row[7]);

        if (row[8])
            sk.total_playtime = strtol(row[8], NULL, 10);

        if (row[9])
            sk.playingtime = strtol(row[9], NULL, 10);
    } else return false;

    mysql_free_result(result);

    query(db, "SELECT COUNT(*) FROM talents WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //begin talents
    sk.talents.count = strtol(row[0], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT * FROM talents WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    for (i = 0; i < sk.talents.count; ++i) {
        //don't crash.
        if (i >= MAX_TALENTS)
            return false;

        sk.talents.talent[i].id = strtol(row[1], NULL, 10);
        sk.talents.talent[i].upgradeLevel = strtol(row[2], NULL, 10);
        sk.talents.talent[i].maxLevel = strtol(row[3], NULL, 10);

        row = mysql_fetch_row(result);
        if (!row)
            break;
    }

    mysql_free_result(result);
    //end talents

    query(db, "SELECT COUNT(*) FROM abilities WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //begin abilities
    numAbilities = strtol(row[0], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT * FROM abilities WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    for (i = 0; i < numAbilities; ++i) {
        int index;
        index = strtol(row[1], NULL, 10);

        if ((index >= 0) && (index < MAX_ABILITIES)) {
            sk.abilities[index].level = strtol(row[2], NULL, 10);
            sk.abilities[index].max_level = strtol(row[3], NULL, 10);
            sk.abilities[index].hard_max = strtol(row[4], NULL, 10);
            sk.abilities[index].modifier = strtol(row[5], NULL, 10);
            sk.abilities[index].disable = strtol(row[6], NULL, 10);
            sk.abilities[index].general_skill = strtol(row[7], NULL, 10);

            row = mysql_fetch_row(result);
            if (!row)
                break;
        }
    }
    //end abilities

    mysql_free_result(result);


    query(db, "SELECT COUNT(*) FROM weapon_meta WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //begin weapons
    numWeapons = strtol(row[0], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT * FROM weapon_meta WHERE char_idx=%d", id);

    result_b = mysql_store_result(db);
    row = mysql_fetch_row(result_b);

    for (i = 0; i < numWeapons; ++i) {
        int index;
        index = strtol(row[1], NULL, 10);

        if ((index >= 0) && (index < MAX_WEAPONS)) {
            int j;
            sk.weapons[index].disable = strtol(row[2], NULL, 10);

            query(db, "SELECT * FROM weapon_mods WHERE weapon_index=%d AND char_idx=%d", index, id);

            result = mysql_store_result(db);
            row = mysql_fetch_row(result);

            if (row) {
                for (j = 0; j < MAX_WEAPONMODS; ++j) {
                    sk.weapons[index].mods[strtol(row[2], NULL, 10)].level = strtol(row[3], NULL, 10);
                    sk.weapons[index].mods[strtol(row[2], NULL, 10)].soft_max = strtol(row[4], NULL, 10);
                    sk.weapons[index].mods[strtol(row[2], NULL, 10)].hard_max = strtol(row[5], NULL, 10);

                    row = mysql_fetch_row(result);
                    if (!row)
                        break;
                }
            }

            mysql_free_result(result);
        }

        row = mysql_fetch_row(result_b);
        if (!row)
            break;
    }

    mysql_free_result(result_b);
    //end weapons

    //begin runes

    query(db, "SELECT COUNT(*) FROM runes_meta WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    numRunes = strtol(row[0], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT * FROM runes_meta WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    for (i = 0; i < numRunes; ++i) {
        int index;
        index = strtol(row[1], NULL, 10);
        if ((index >= 0) && (index < MAX_VRXITEMS)) {
            int j;

            sk.items[index].itemtype = strtol(row[2], NULL, 10);
            sk.items[index].itemLevel = strtol(row[3], NULL, 10);
            sk.items[index].quantity = strtol(row[4], NULL, 10);
            sk.items[index].untradeable = strtol(row[5], NULL, 10);
            strcpy_s(sk.items[index].id, sizeof sk.items[index].id, row[6]);
            strcpy_s(sk.items[index].name, sizeof sk.items[index].name, row[7]);
            sk.items[index].numMods = strtol(row[8], NULL, 10);
            sk.items[index].setCode = strtol(row[9], NULL, 10);
            sk.items[index].classNum = strtol(row[10], NULL, 10);

            query(db, "SELECT type, mindex, value, rset FROM runes_mods WHERE rune_index=%d AND char_idx=%d", index,
                  id);

            result_b = mysql_store_result(db);

            if (result_b)
                row = mysql_fetch_row(result_b);
            else
                row = NULL;

            int mod = 0;
            while (row && mod < MAX_VRXITEMMODS) {
                sk.items[index].modifiers[mod].type = strtol(row[0], NULL, 10);
                sk.items[index].modifiers[mod].index = strtol(row[1], NULL, 10);
                sk.items[index].modifiers[mod].value = strtol(row[2], NULL, 10);
                sk.items[index].modifiers[mod].set = strtol(row[3], NULL, 10);

                row = mysql_fetch_row(result_b);
                mod++;
            }

            mysql_free_result(result_b);
        }

        row = mysql_fetch_row(result);
        if (!row)
            break;
    }

    mysql_free_result(result);
    //end runes

    //*****************************
    //standard stats
    //*****************************

    query(db, "SELECT * FROM point_data WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //Exp
    sk.experience = strtol(row[1], NULL, 10);
    //next_level
    sk.next_level = strtol(row[2], NULL, 10);
    //Level
    sk.level = strtol(row[3], NULL, 10);
    //Class number
    sk.class_num = strtol(row[4], NULL, 10);
    //skill points
    sk.speciality_points = strtol(row[5], NULL, 10);
    //credits
    sk.credits = strtol(row[6], NULL, 10);
    //weapon points
    sk.weapon_points = strtol(row[7], NULL, 10);
    //respawn weapon
    sk.respawn_weapon = strtol(row[8], NULL, 10);
    //talent points
    sk.talents.talentPoints = strtol(row[9], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT char_idx, "
          "respawns, "
          "health, maxhealth, "
          "armour, "
          "maxarmour, "
          "nerfme, "
          "adminlevel, "
          "bosslevel, "
          "prestigelevel, "
          "prestigepoints "
          "FROM character_data WHERE char_idx=%d", id
    );

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //*****************************
    //in-game stats
    //*****************************
    sk.weapon_respawns = strtol(row[1], NULL, 10);
    sk.current_health = strtol(row[2], NULL, 10);
    sk.max_health = strtol(row[3], NULL, 10);
    sk.current_armor = strtol(row[4], NULL, 10);
    sk.max_armor = strtol(row[5], NULL, 10);
    sk.nerfme = strtol(row[6], NULL, 10);
    sk.administrator = strtol(row[7], NULL, 10);
    sk.boss = strtol(row[8], NULL, 10);
    sk.prestige.total = strtol(row[9], NULL, 10);
    sk.prestige.points = strtol(row[10], NULL, 10);

    mysql_free_result(result);

    //*****************************
    //stats
    //*****************************

    query(db, "SELECT * FROM game_stats WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //shots fired
    sk.shots = strtol(row[1], NULL, 10);
    //shots hit
    sk.shots_hit = strtol(row[2], NULL, 10);
    //frags
    sk.frags = strtol(row[3], NULL, 10);
    //deaths
    sk.fragged = strtol(row[4], NULL, 10);
    //number of sprees
    sk.num_sprees = strtol(row[5], NULL, 10);
    //max spree
    sk.max_streak = strtol(row[6], NULL, 10);
    //number of wars
    sk.spree_wars = strtol(row[7], NULL, 10);
    //number of sprees broken
    sk.break_sprees = strtol(row[8], NULL, 10);
    //number of wars broken
    sk.break_spree_wars = strtol(row[9], NULL, 10);
    //suicides
    sk.suicides = strtol(row[10], NULL, 10);
    //teleports			(link this to "use tballself" maybe?)
    sk.teleports = strtol(row[11], NULL, 10);
    //number of 2fers
    sk.num_2fers = strtol(row[12], NULL, 10);

    mysql_free_result(result);

    query(db, "SELECT * FROM ctf_stats WHERE char_idx=%d", id);

    result = mysql_store_result(db);
    row = mysql_fetch_row(result);

    //CTF statistics
    sk.flag_pickups = strtol(row[1], NULL, 10);
    sk.flag_captures = strtol(row[2], NULL, 10);
    sk.flag_returns = strtol(row[3], NULL, 10);
    sk.flag_kills = strtol(row[4], NULL, 10);
    sk.offense_kills = strtol(row[5], NULL, 10);
    sk.defense_kills = strtol(row[6], NULL, 10);
    sk.assists = strtol(row[7], NULL, 10);
    //End CTF

    mysql_free_result(result);
    // prestige
    query(db, "SELECT char_idx, pindex, param, level FROM prestige WHERE char_idx=%d", id);;

    result = mysql_store_result(db);
    if (result) {
        row = mysql_fetch_row(result);
        while (row) {
            int pindex = strtol(row[1], NULL, 10);
            int param = strtol(row[2], NULL, 10);
            int level = strtol(row[3], NULL, 10);

            switch (pindex) {
                case PRESTIGE_CREDITS:
                    sk.prestige.creditLevel = level;
                    break;
                case PRESTIGE_ABILITY_POINTS:
                    sk.prestige.abilityPoints = level;
                    break;
                case PRESTIGE_WEAPON_POINTS:
                    sk.prestige.weaponPoints = level;
                    break;
                case PRESTIGE_SOFTMAX_BUMP:
                        sk.prestige.softmaxBump[param] = level;
                    break;
                case PRESTIGE_CLASS_SKILL:
                    sk.prestige.classSkill[param / 32] |= (1 << (param % 32));
                    break;
                default: break;
            }

            row = mysql_fetch_row(result);
        }
    }

    mysql_free_result(result);

    // defer join game
    struct evt_character_loaded_t *evt = vrx_malloc(sizeof(struct evt_character_loaded_t), TAG_GAME);
    evt->ent = player;
    evt->sk = sk;
    evt->connection_id = current->connection_id;
    defer_add(
        &player->client->defers,
        (defer_t){
            .function = vrx_notify_join_game,
            .data = evt
        });

    return true;
}

void gds_op_saveclose(gds_queue_t *current, MYSQL *db) {
    int id = gds_op_save(current, db);
    if (id != -1) {
        query(db,
        "UPDATE userdata SET isplaying = 0 WHERE char_idx = %d;", id);
        query(db, "UPDATE stash SET lock_char_id = NULL WHERE lock_char_id = %d;",
            id);
    }
}

// Start Connection to GDS/MySQL

void gds_create_process_queue() {
    // gi.dprintf ("DB: Creating thread...");
    int rc = pthread_create(&QueueThread, &attr, gds_process_queue, NULL);

    if (rc) {
        gi.dprintf(" Failure creating thread! %d\n", rc);
        return;
    } /*else
		gi.dprintf(" Done!\n");*/
    gds_set_thread_running(true);
}

qboolean gds_connect() {
    int rc;

    gi.dprintf("DB: Initializing connection... ");

    gds_singleserver = gi.cvar("gds_single", "0", 0); // default to multi server using sql.
    gds_serverkey = gi.cvar("gds_serverkey", "123456", 0);

    const char *database = gi.cvar("gds_dbaddress", DEFAULT_DATABASE, 0)->string;
    const char *user = gi.cvar("gds_dbuser", MYSQL_USER, 0)->string;
    const char *pw = gi.cvar("gds_dbpass", MYSQL_PW, 0)->string;
    const char *dbname = gi.cvar("gds_dbname", MYSQL_DBNAME, 0)->string;

    if (!GDS_MySQL) {
        GDS_MySQL = mysql_init(NULL);
        if (mysql_real_connect(GDS_MySQL,
            database, user, pw, dbname, 0, NULL, 0) == NULL) {
            gi.dprintf("Failure: %s\n", mysql_error(GDS_MySQL));
            mysql_close(GDS_MySQL);
            GDS_MySQL = NULL;
            return false;
        }
    } else {
        if (GDS_MySQL) {
            gi.dprintf("DB: Already connected\n");
        }
        return false;
    }

    char esc_key[sizeof (gds_serverkey->string) * 2];
    mysql_real_escape_string(GDS_MySQL, esc_key, gds_serverkey->string, strlen(gds_serverkey->string));
    query(GDS_MySQL, "CALL NotifyServerStatus('%s', 1);", esc_key);

    /*gds_debug = gi.cvar("gds_debug", "0", 0);*/

    gi.dprintf("Success!\n");

    pthread_mutex_init(&mutex_gds_queue, NULL);
    rc = pthread_mutex_init(&mutex_gds_thread_status, NULL);
    if (rc)
        gi.dprintf("mutex creation err: %d", rc);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    return true;
}

void Mem_PrepareMutexes() {
    pthread_mutex_init(&MemMutex_Malloc, NULL);
    pthread_mutex_init(&MemMutex_Free, NULL);
}

qboolean gds_enabled() {
    return GDS_MySQL != NULL;
}

qboolean vrx_mysql_isloading(edict_t *ent) {
    // cleared when done loading
    return ent->gds_connection_load_id > 0;
}

qboolean vrx_mysql_saveclose_character(edict_t *player) {
    if (gds_enabled()) {
        gds_queue_add(player, GDS_SAVECLOSE, -1);
        return true;
    }

    return false;
}

void gds_finish_thread() {
    void *status;
    int rc;

    if (GDS_MySQL) {
        gds_queue_add(NULL, GDS_DISCONNECT, -1);

        gi.dprintf("DB: Finishing thread... ");
        rc = pthread_join(QueueThread, &status);
        gi.dprintf(" Done.\n", rc);

        if (rc)
            gi.dprintf("pthread_join: %d\n", rc);

        rc = pthread_mutex_destroy(&mutex_gds_queue);
        if (rc)
            gi.dprintf("pthread_mutex_destroy: %d\n", rc);

        mysql_close(GDS_MySQL);
        GDS_MySQL = NULL;
    }
}

#endif // NO_GDS

void *vrx_malloc(size_t Size, int Tag) {
    void *Memory;
    pthread_mutex_lock(&MemMutex_Malloc);
    Memory = gi.TagMalloc(Size, Tag);
    pthread_mutex_unlock(&MemMutex_Malloc);
    return Memory;
}

void vrx_free(void *mem) {
    pthread_mutex_lock(&MemMutex_Free);
    gi.TagFree(mem);
    pthread_mutex_unlock(&MemMutex_Free);
}
