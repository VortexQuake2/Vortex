#include "../../g_local.h"
#include "v_characterio.h"
#include "v_sqlite_unidb.h"
#include "gds.h"

char_io_t vrx_char_io;

void vrx_setup_sqlite_io();
#ifndef NO_GDS
void vrx_setup_mysql_io();
#endif

void vrx_init_char_io() {
    memset(&vrx_char_io, 0, sizeof vrx_char_io);
    if (savemethod == NULL) {
        gi.error("vrx_init_char_io called before savemethod has been initialized.");
        return;
    }

    int method = savemethod->value;
    switch (method) {
#ifndef NO_GDS
        case SAVEMETHOD_MYSQL:
            vrx_setup_mysql_io();
            break;
#endif
        default:
            gi.dprintf("unsupported method, defaulting to 3 (sqlite single file mode)");
        case SAVEMETHOD_SQLITE:
            vrx_setup_sqlite_io();
            break;
    }

#ifndef GDS_NOMULTITHREADING
    Mem_PrepareMutexes();
#endif
}

void vrx_close_char_io() {
    memset(&vrx_char_io, 0, sizeof vrx_char_io);

    int method = savemethod->value;
    switch (method) {
#ifndef NO_GDS
        case 2:
            gds_finish_thread();
            break;
#endif
        default:
            gi.dprintf("unsupported method, defaulting to 3 (sqlite single file mode)");
        case 3:
            cdb_end_connection();
            break;
    }
}

void vrx_notify_owner_nonexistent(void* args)
{
    event_owner_error_t* evt = args;

    if (evt->connection_id != evt->ent->gds_connection_id) {
        V_Free(args);
        return;
    }

    gi.cprintf(evt->ent, PRINT_HIGH, "The character '%s' does not exist. You cannot use it as an owner.\n", evt->owner_name);

    V_Free(args);
}

void vrx_notify_owner_bad_password(void* args)
{
    event_owner_error_t* evt = args;

    if (evt->connection_id != evt->ent->gds_connection_id) {
        V_Free(args);
        return;
    }

    gi.cprintf(evt->ent, PRINT_HIGH, "The password you entered is not correct.\n");
    V_Free(args);
}

void vrx_notify_owner_success(void* args)
{
    event_owner_error_t* evt = args;

    if (evt->connection_id != evt->ent->gds_connection_id) {
        V_Free(args);
        return;
    }

    assert(sizeof evt->ent->myskills.owner == sizeof evt->owner_name);
    strcpy(evt->ent->myskills.owner, evt->owner_name);

    gi.cprintf(evt->ent, PRINT_HIGH, "Owner set successfully.\n");
    V_Free(args);
}

void vrx_chario_noop(edict_t* player) { }

qboolean vrx_sqlite_character_exists(edict_t *ent) {
    return cdb_get_id(ent->client->pers.netname) != -1;
}

qboolean vrx_sqlite_isloading(edict_t *ent) {
    return false; // We're strictly single threaded.
}

void vrx_setup_sqlite_io() {
    vrx_char_io = (char_io_t) {
        .save_player_runes = &cdb_save_runes,
        .save_player = &cdb_save_player,
        .save_close_player = &cdb_saveclose_player,
        .load_player = &cdb_load_player,
        .handle_status = NULL,
        .character_exists = &vrx_sqlite_character_exists,
        .is_loading = &vrx_sqlite_isloading,
        .set_owner = &cdb_set_owner,
        .type = SAVEMETHOD_SQLITE
    };

    cdb_start_connection();
}

#ifndef NO_GDS
qboolean vrx_mysql_save_character(edict_t* player) {
    if (gds_enabled())
    {
        gds_queue_add(player, GDS_SAVE, -1);
        return true;
    }

    return false;
}

qboolean vrx_mysql_save_character_runes(edict_t* player) {
    if (gds_enabled())
    {
        gds_queue_add(player, GDS_SAVERUNES, -1);
        return true;
    }

    return false;
}


qboolean vrx_mysql_load_character(edict_t* player) {
    if (gds_enabled())
    {
        if (player->gds_thread_status == GDS_STATUS_CHARACTER_LOADING) {
            gi.cprintf(player, PRINT_HIGH, "You're already queued for loading.\n");
            return false;
        }

        gi.cprintf(player, PRINT_HIGH, "You're now queued for loading.\n");
        gds_queue_add(player, GDS_LOAD, -1);
        return true;
    }

    return false;
}



void vrx_setup_mysql_io() {
    vrx_char_io = (char_io_t) {
            .save_player_runes = &vrx_mysql_save_character_runes,
            .save_player = &vrx_mysql_save_character,
            .save_close_player = &vrx_mysql_saveclose_character,
            .load_player = &vrx_mysql_load_character,
            .is_loading = &vrx_mysql_isloading,
            .handle_status = &gds_handle_status,
        .set_owner = &gds_queue_add_setowner,
        .type = SAVEMETHOD_MYSQL
    };

    gds_connect();
}

#endif