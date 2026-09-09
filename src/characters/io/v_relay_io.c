#include "../../g_local.h"
#include "v_characterio.h"
#include "../../server/relay.h"
#include "../../server/relay_msgpack.h"

// Command names for the relay protocol
const char* const COMMAND_CHAR_LOAD = "Load";
const char* const COMMAND_CHAR_SAVE = "Save";
const char* const COMMAND_CHAR_SAVE_CLOSE = "SaveAndClose";
const char* const COMMAND_STASH_OPEN = "StashOpen";
const char* const COMMAND_STASH_CLOSE = "StashClose";
const char* const COMMAND_STASH_STORE = "StashStore";
const char* const COMMAND_STASH_TAKE = "StashTake";
const char* const COMMAND_STASH_GET_PAGE = "StashPage";
const char* const COMMAND_SET_OWNER = "SetOwner";

// Helper to notify that the stash is locked
static void vrx_relay_notify_stash_locked(edict_t* ent) {
    stash_event_t* notif = vrx_malloc(sizeof(stash_event_t), TAG_GAME);
    notif->ent = ent;
    notif->gds_connection_id = ent->gds.connection_id;
    vrx_notify_stash_locked(notif);
    vrx_free(notif);
}

// char_io_t implementation

qboolean vrx_relay_save_player(edict_t* ent) {
    if (!vrx_relay_is_connected()) return false;

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 4);

    msgpack_pack_str(&pk, strlen(COMMAND_CHAR_SAVE));
    msgpack_pack_str_body(&pk, COMMAND_CHAR_SAVE, strlen(COMMAND_CHAR_SAVE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    msgpack_pack_skills(&pk, &ent->myskills, ent->gds.connection_id);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_save_close_player(edict_t* ent) {
    if (!vrx_relay_is_connected()) return false;

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 4);

    msgpack_pack_str(&pk, strlen(COMMAND_CHAR_SAVE_CLOSE));
    msgpack_pack_str_body(&pk, COMMAND_CHAR_SAVE_CLOSE, strlen(COMMAND_CHAR_SAVE_CLOSE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    msgpack_pack_skills(&pk, &ent->myskills, ent->gds.connection_id);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_save_player_runes(edict_t* ent) {
    // Relay protocol might not need a specialized "runes only" save, 
    // but we can implement it if bandwidth is an issue.
    // For now, just save everything.
    return vrx_relay_save_player(ent);
}

qboolean vrx_relay_load_player(edict_t* ent) {
    if (!vrx_relay_is_connected()) {
        gi.cprintf(ent, PRINT_HIGH, "Relay not connected, cannot load.\n");
        return false;
    }

    static int64_t load_id = 1;

    load_id++;

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 4);
    msgpack_pack_str(&pk, strlen(COMMAND_CHAR_LOAD));
    msgpack_pack_str_body(&pk, COMMAND_CHAR_LOAD, strlen(COMMAND_CHAR_LOAD));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    const char* pass = Info_ValueForKey(ent->client->pers.userinfo, "vrx_password");
    msgpack_pack_str(&pk, strlen(pass));
    msgpack_pack_str_body(&pk, pass, strlen(pass));

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);

    ent->gds.connection_load_id = load_id;
    return true;
}

qboolean vrx_relay_is_loading(edict_t* ent) {
    return ent->gds.connection_load_id != 0;
}

void vrx_relay_set_owner(edict_t* ent, char* charname, char* mpw, qboolean reset) {
    if (!vrx_relay_is_connected()) return;

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 6);
    msgpack_pack_str(&pk, strlen(COMMAND_SET_OWNER));
    msgpack_pack_str_body(&pk, COMMAND_SET_OWNER, strlen(COMMAND_SET_OWNER));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    msgpack_pack_str(&pk, strlen(mpw));
    msgpack_pack_str_body(&pk, mpw, strlen(mpw));

    if (reset)
        msgpack_pack_true(&pk);
    else
        msgpack_pack_false(&pk);

    msgpack_pack_str(&pk, strlen(charname));
    msgpack_pack_str_body(&pk, charname, strlen(charname));

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
}

// stash_io_t implementation

qboolean vrx_relay_stash_open(edict_t* ent) {
    if (!vrx_relay_is_connected()) {
        vrx_relay_notify_stash_locked(ent);
        return false;
    }

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_OPEN));
    msgpack_pack_str_body(&pk, COMMAND_STASH_OPEN, strlen(COMMAND_STASH_OPEN));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));
    msgpack_pack_int64(&pk, ent->gds.connection_id);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_stash_close(edict_t* ent) {
    if (!vrx_relay_is_connected()) return false;

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 3);
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_CLOSE));
    msgpack_pack_str_body(&pk, COMMAND_STASH_CLOSE, strlen(COMMAND_STASH_CLOSE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int(&pk, ent->gds.connection_id);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_stash_store(edict_t* ent, item_t* item) {
    if (!vrx_relay_is_connected()) {
        vrx_relay_notify_stash_locked(ent);
        return false;
    }

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 4);
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_STORE));
    msgpack_pack_str_body(&pk, COMMAND_STASH_STORE, strlen(COMMAND_STASH_STORE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);
    
    msgpack_pack_item(&pk, item);

    auto result = vrx_relay_send_sbuffer(&sbuf);

    msgpack_sbuffer_destroy(&sbuf);
    
    return result;
}

qboolean vrx_relay_stash_take(edict_t* ent, int stash_index) {
    if (!vrx_relay_is_connected()) {
        vrx_relay_notify_stash_locked(ent);
        return false;
    }

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 5);
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_TAKE));
    msgpack_pack_str_body(&pk, COMMAND_STASH_TAKE, strlen(COMMAND_STASH_TAKE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    msgpack_pack_int(&pk, stash_index / MAX_STASH_PAGE_ITEMS); // page
    msgpack_pack_int(&pk, stash_index % MAX_STASH_PAGE_ITEMS);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_stash_get_page(edict_t* ent, int page, int numitems) {
    if (!vrx_relay_is_connected()) {
        vrx_relay_notify_stash_locked(ent);
        return false;
    }

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 4);
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_GET_PAGE));
    msgpack_pack_str_body(&pk, COMMAND_STASH_GET_PAGE, strlen(COMMAND_STASH_GET_PAGE));

    msgpack_pack_str(&pk, strlen(ent->client->pers.netname));
    msgpack_pack_str_body(&pk, ent->client->pers.netname, strlen(ent->client->pers.netname));

    msgpack_pack_int64(&pk, ent->gds.connection_id);

    msgpack_pack_int(&pk, page);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

qboolean vrx_relay_stash_close_by_id(int owner_id) {
    if (!vrx_relay_is_connected()) {
        return false;
    }

    msgpack_sbuffer sbuf;
    msgpack_packer pk;
    msgpack_sbuffer_init(&sbuf);
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    msgpack_pack_array(&pk, 2);
    const char* COMMAND_STASH_CLOSE_BY_ID = "StashCloseById";
    msgpack_pack_str(&pk, strlen(COMMAND_STASH_CLOSE_BY_ID));
    msgpack_pack_str_body(&pk, COMMAND_STASH_CLOSE_BY_ID, strlen(COMMAND_STASH_CLOSE_BY_ID));

    msgpack_pack_int(&pk, owner_id);

    vrx_relay_send_sbuffer(&sbuf);
    msgpack_sbuffer_destroy(&sbuf);
    return true;
}

void vrx_setup_relay_io() {
    vrx_char_io = (char_io_t) {
        .save_player_runes = &vrx_relay_save_player_runes,
        .save_player = &vrx_relay_save_player,
        .save_close_player = &vrx_relay_save_close_player,
        .load_player = &vrx_relay_load_player,
        .multithread = true,
        .is_loading = &vrx_relay_is_loading,
        .set_owner = &vrx_relay_set_owner,
        .type = SAVEMETHOD_RELAY
    };
    gi.dprintf("RS: IO initialized.\n");
}

void vrx_setup_relay_stash_io() {
    vrx_stash_io = (stash_io_t) {
        .open_stash = &vrx_relay_stash_open,
        .close_stash = &vrx_relay_stash_close,
        .store = &vrx_relay_stash_store,
        .take = &vrx_relay_stash_take,
        .get_page = &vrx_relay_stash_get_page,
        .close_stash_by_id = &vrx_relay_stash_close_by_id
    };
    gi.dprintf("RS: Stash IO initialized.\n");
}
