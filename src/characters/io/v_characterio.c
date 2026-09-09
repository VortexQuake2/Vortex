#include "../../g_local.h"
#include "v_characterio.h"
#include "v_sqlite_unidb.h"
#include "gds.h"
#include "characters/class_limits.h"

char_io_t vrx_char_io;

void vrx_setup_sqlite_io();
void vrx_setup_relay_io();

void vrx_init_char_io() {
    memset(&vrx_char_io, 0, sizeof vrx_char_io);
    if (savemethod == NULL) {
        gi.error("vrx_init_char_io called before savemethod has been initialized.");
        return;
    }

    const int method = savemethod->value;
    switch (method) {
        case SAVEMETHOD_RELAY:
            vrx_setup_relay_io();
            break;
        default:
            gi.dprintf("unsupported method, defaulting to 3 (sqlite single file mode)");
        case SAVEMETHOD_SQLITE:
            vrx_setup_sqlite_io();
            break;
    }

    Mem_PrepareMutexes();
}

void vrx_close_char_io() {
    memset(&vrx_char_io, 0, sizeof vrx_char_io);

    const int method = savemethod->value;
    switch (method) {
        case SAVEMETHOD_RELAY:
            // relay doesn't need explicit shutdown here, handled by server exit
            break;
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

    if (evt->connection_id != evt->ent->gds.connection_id) {
        return;
    }

    gi.cprintf(evt->ent, PRINT_HIGH, "The character '%s' does not exist. You cannot use it as an owner.\n", evt->owner_name);
}

void vrx_notify_owner_bad_password(void* args)
{
    const event_owner_error_t* evt = args;

    if (evt->connection_id != evt->ent->gds.connection_id) {
        return;
    }

    gi.cprintf(evt->ent, PRINT_HIGH, "The password you entered is not correct.\n");
}

void vrx_notify_owner_success(void* args)
{
    const event_owner_error_t* evt = args;

    if (evt->connection_id != evt->ent->gds.connection_id) {
        return;
    }

    assert(sizeof evt->ent->client->resp.pstats.owner == sizeof evt->owner_name);
    strcpy(evt->ent->client->resp.pstats.owner, evt->owner_name);

    gi.cprintf(evt->ent, PRINT_HIGH, "Owner set successfully.\n");
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
        .multithread = false,
        .character_exists = &vrx_sqlite_character_exists,
        .is_loading = &vrx_sqlite_isloading,
        .set_owner = &cdb_set_owner,
        .type = SAVEMETHOD_SQLITE
    };

    cdb_start_connection();
}

// for async character loading
void vrx_notify_character_load_completion(edict_t *ent, playertransfer_t *sk) {
    // Notify character system that loading is done
    ent->gds.connection_load_id = 0;

    const int result_status = vrx_get_login_status(ent);
    if (result_status < 0) {
        vrx_print_login_status(ent, result_status);
        return;
    }

    ent->myskills = *sk->skills;
    ent->client->resp.pstats = *sk->stats;

    vrx_runes_unapply(ent);
    for (int i = 0; i < 4; ++i)
        vrx_runes_apply(ent, &sk->stats->items[i]);

    // ent->client->resp.pstats.inventory[body_armor_index] = sk->skills->current_armor;

    //done
    vrx_open_mode_menu(ent);
}

