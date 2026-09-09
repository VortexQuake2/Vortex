#include "relay_handlers.h"
#include "relay_msgpack.h"
#include "../characters/v_stash.h"
#include "../combat/abilities/g_abilities.h"
#include <msgpack.h>

#include "characters/io/v_characterio.h"

msgpack_object_str* msgpack_get_map_string(msgpack_object_map* map, const char* key) {
    for (uint32_t i = 0; i < map->size; i++) {
        msgpack_object_kv* kv = map->ptr + i;

        if (kv->key.type != MSGPACK_OBJECT_STR) {
            continue;
        }

        if (kv->key.via.str.size == strlen(key) && memcmp(kv->key.via.str.ptr, key, strlen(key)) == 0) {
            if (kv->val.type != MSGPACK_OBJECT_STR) {
                continue;
            }

            return &kv->val.via.str;
        }
    }

    return nullptr;
}

int msgpack_streq(msgpack_object_str* str, const char* cmp) {
    return memcmp(str->ptr, cmp, str->size) == 0;
}

edict_t* vrx_relay_find_client(int connection_id) {
    for (int i = 1; i <= game.maxclients; i++) {
        edict_t* check = &g_edicts[i];
        if (check->inuse && check->client && check->gds.connection_id == connection_id) {
            return check;
        }
    }

    return nullptr;
}

qboolean vrx_relay_try_message_relay(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    if (!msgpack_streq(type, COMMAND_RELAY))
        return false;

    msgpack_object_str* message = arr->ptr[1].type == MSGPACK_OBJECT_STR ? &arr->ptr[1].via.str : nullptr;
    if (message == nullptr) {
        gi.dprintf("RS: Received relay message does not have a valid message field.\n");
        *invalid = true;
        return false;
    }

    // az: nullptr-terminate the string
    char* msg = calloc(message->size + 1, 1);
    if (msg == nullptr) {
        gi.dprintf("RS: Failed to allocate memory for relay message.\n");
        *invalid = true;
        return false;
    }

    memcpy(msg, message->ptr, message->size);

    for (int j = 1; j <= game.maxclients; j++) {
        const edict_t *other = &g_edicts[j];
        if (!other->inuse)
            continue;
        if (!other->client)
            continue;
        if (other->svflags & SVF_MONSTER)
            continue;

        gi.cprintf(other, PRINT_CHAT, "[relay] %s\n", msg);
    }

    // az: print to the server console
    gi.cprintf(nullptr, PRINT_CHAT, "[relay] %s\n", msg);
    free(msg);

    return true;
}


qboolean vrx_relay_try_character_loaded(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    if (!msgpack_streq(type, "LoadResult"))
        return false;

    if (arr->size < 3) {
        *invalid = true;
        return false;
    }

    auto status = arr->ptr[2].type == MSGPACK_OBJECT_STR ? &arr->ptr[2].via.str : nullptr;
    if (status == nullptr) {
        gi.dprintf("RS: Received relay message does not have a valid status type.\n");
        *invalid = true;
        return false;
    }


    auto connection_id = (int)arr->ptr[1].via.u64;
    edict_t* ent = vrx_relay_find_client(connection_id);

    if (!ent) {
        gi.dprintf("RS: CharacterLoaded for unknown connection %d\n", connection_id);
        return true;
    }

    pstats_t pst = {0};
    skills_t sk = {0};
    playertransfer_t pt = {
        &pst,
        &sk
    };
    if (!msgpack_streq(status, "Ok")) {
        if (msgpack_streq(status, "WrongPassword")) {
            gi.cprintf(ent, PRINT_HIGH, "The password is incorrect.\n");
        } else if (msgpack_streq(status, "CharacterNotFound")) {
            gi.cprintf(ent, PRINT_HIGH, "Creating a new character!\n");
            vrx_notify_character_load_completion(ent, &pt);
        } else if (msgpack_streq(status, "CharacterLocked")) {
            gi.cprintf(ent, PRINT_HIGH, "Character is locked because it is being used elsewhere. If this is incorrect, contact an admin.\n");
        }
        return true;
    }

    if (arr->ptr[3].type == MSGPACK_OBJECT_NIL) {
        gi.cprintf(ent, PRINT_HIGH, "Character failed to load from relay (Not authorized or not found, or internal error).\n");
        return true;
    }

    if (!msgpack_unpack_skills(&arr->ptr[3], &sk)) {
        gi.cprintf(ent, PRINT_HIGH, "Failed to unpack character data from relay.\n");
        return false;
    }

    vrx_notify_character_load_completion(ent, &sk);
    return true;
}

qboolean vrx_relay_try_setowner_result(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    if (!msgpack_streq(type, "SetOwnerResult"))
        return false;

    if (arr->size < 3) {
        *invalid = true;
        return false;
    }

    auto connection_id = arr->ptr[1].type == MSGPACK_OBJECT_POSITIVE_INTEGER ? &arr->ptr[1].via.u64 : nullptr;
    auto result = arr->ptr[2].type == MSGPACK_OBJECT_STR ? &arr->ptr[2].via.str : nullptr;
    auto new_owner = arr->ptr[3].type == MSGPACK_OBJECT_STR ? &arr->ptr[3].via.str : nullptr;

    if (result == nullptr) {
        *invalid = true;
        return false;
    }

    auto ent = vrx_relay_find_client(*connection_id);
    if (!ent) {
        return true;
    }

    event_owner_error_t evt = {0};
    evt.ent = ent;
    evt.connection_id = ent->gds.connection_id;

    if (!msgpack_streq(result, "Ok")) {
        if (msgpack_streq(result, "CharacterNotFound")) {
            vrx_notify_owner_nonexistent(&evt);
        } else if (msgpack_streq(result, "WrongPassword")) {
            vrx_notify_owner_bad_password(&evt);
        } else if (msgpack_streq(result, "OwnerAlreadySet")) {
            gi.cprintf(nullptr, PRINT_HIGH, "Owner already set.\n");
        }
        return true;
    }

    if (new_owner == nullptr) {
        gi.dprintf("RS: Received relay message does not have a valid new owner field.\n");
        *invalid = true;
        return false;
    }

    // limit the damage if for some reason size is > owner name's
    size_t len = min(sizeof(evt.owner_name), new_owner->size);
    char nulowner[len + 1];
    memcpy(nulowner, new_owner->ptr, len);
    nulowner[len] = '\0';

    strncpy(evt.owner_name, new_owner->ptr, sizeof evt.owner_name);

    // thanks strncpy.
    evt.owner_name[sizeof evt.owner_name - 1] = '\0';

    vrx_notify_owner_success(&evt);
    return true;
}


stash_page_event_t * create_stash_page_event(edict_t *ent, int pagenum, msgpack_object_array *items_arr) {
    stash_page_event_t* evt = vrx_malloc(sizeof(stash_page_event_t), TAG_GAME);
    evt->ent = ent;
    evt->gds_connection_id = ent->gds.connection_id;
    evt->gds_owner_id = 0; // Relay doesn't use owner_id in the same way
    evt->pagenum = pagenum;

    int num_items_to_copy = sizeof(evt->page) / sizeof(item_t);
    if (items_arr->size < num_items_to_copy)
        num_items_to_copy = items_arr->size;

    memset(evt->page, 0, sizeof(evt->page));

    for (int i = 0; i < num_items_to_copy; i++) {
        msgpack_object* item_obj = &items_arr->ptr[i];
        if (item_obj->type != MSGPACK_OBJECT_NIL) {
            msgpack_unpack_item(item_obj, &evt->page[i]);
        }
    }
    return evt;
}

qboolean vrx_relay_try_stash_page(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    if (!msgpack_streq(type, "StashPageResult"))
        return false;

    if (arr->size < 4) {
        *invalid = true;
        return false;
    }

    int connection_id = (int)arr->ptr[1].via.i64;
    edict_t* ent = vrx_relay_find_client(connection_id);

    if (!ent) return true;

    int pagenum = (int)arr->ptr[2].via.i64;
    msgpack_object_array* items_arr = arr->ptr[3].type == MSGPACK_OBJECT_ARRAY ? &arr->ptr[3].via.array : nullptr;
    if (items_arr == nullptr) {
        *invalid = true;
        return false;
    }

    stash_page_event_t *evt = create_stash_page_event(ent, pagenum, items_arr);

    vrx_notify_open_stash(evt);
    vrx_free(evt);
    
    return true;
}

qboolean vrx_relay_try_stash_open_result(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    if (!msgpack_streq(type, "StashOpenResult"))
        return false;

    if (arr->size < 4) {
        *invalid = true;
        return false;
    }

    uint64_t connection_id = arr->ptr[1].via.u64;
    edict_t* ent = vrx_relay_find_client(connection_id);

    if (!ent) return true;

    auto status = arr->ptr[2].type == MSGPACK_OBJECT_STR ? &arr->ptr[2].via.str : nullptr;

    if (status == nullptr) {
        *invalid = true;
        return false;
    }

    if (!msgpack_streq(status, "Ok")) {
        if (msgpack_streq(status, "StashLocked")) {
            stash_event_t evt = {
                .ent = ent,
                .gds_connection_id = connection_id,
            };

            vrx_notify_stash_locked(&evt);
            return true;
        }
    }

    auto items_arr = arr->ptr[3].type == MSGPACK_OBJECT_ARRAY ? &arr->ptr[3].via.array : nullptr;
    if (items_arr == nullptr) {
        *invalid = true;
        return false;
    }

    stash_page_event_t* evt = create_stash_page_event(ent, 0, items_arr);

    vrx_notify_open_stash(evt);
    vrx_free(evt);
    
    return true;
}

qboolean vrx_relay_try_stash_event(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    // xxx: these are server-to-relay commands
    if (msgpack_streq(type, "StashOpen") || msgpack_streq(type, "StashClose") || msgpack_streq(type, "StashCloseById")) {
        // Just log it for now
        gi.dprintf("RS: Received stash event %.*s\n", (int)type->size, type->ptr);
        return true;
    }

    if (!msgpack_streq(type, "StashTakeResult")) {
        return false;
    }

    if (arr->size < 3) {
        *invalid = true;
        return false;
    }

    auto connection_id = arr->ptr[1].type == MSGPACK_OBJECT_POSITIVE_INTEGER ? &arr->ptr[1].via.u64 : nullptr;
    auto item = arr->ptr[2].type == MSGPACK_OBJECT_ARRAY ? &arr->ptr[2] : nullptr;

    if (item == nullptr || connection_id == nullptr) {
        *invalid = true;
        return false;
    }

    auto ent = vrx_relay_find_client(*connection_id);

    if (!ent) return true;

    item_t _item;
    if (!msgpack_unpack_item(item, &_item)) {
        gi.dprintf("RS: Failed to unpack item from relay.\n");
        return false;
    }

    stash_taken_event_t evt = {
        .ent = ent,
        .gds_connection_id = *connection_id,
        .taken = _item,
    };

    memcpy(evt.requester, ent->client->pers.netname, sizeof(evt.requester));
    vrx_notify_stash_taken(&evt);

    return false;
}

qboolean vrx_relay_try_authorized(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid) {
    const char* COMMAND_AUTHORIZE = "Authorize";
    const char* COMMAND_AUTHORIZE_SUCCESS = "Authorized";

    if (!msgpack_streq(type, COMMAND_AUTHORIZE)) {
        return false;
    }

    const auto result = arr->ptr[1].type == MSGPACK_OBJECT_ARRAY ? &arr->ptr[1].via.array : nullptr;
    if (result == nullptr) {
        gi.dprintf("RS: Received relay message does not have a valid message field.\n");
        *invalid = true;
        return false;
    }

    if (result->size != 1) {
        gi.dprintf("RS: unexpected authorization result; result type length is != 1: %d\n", result->size);
        *invalid = true;
        return false;
    }

    auto result_type = result->ptr[0].type == MSGPACK_OBJECT_STR ? &result->ptr[0].via.str : nullptr;
    if (result_type == nullptr) {
        gi.dprintf("RS: unexpected authorization result; result type is not a string");
        *invalid = true;
        return false;
    }

    if (msgpack_streq(result_type, COMMAND_AUTHORIZE_SUCCESS)) {
        relay_authorized = true;
        gi.dprintf("RS: Relay server is authorized.\n");
        return true;
    } else {
        gi.dprintf("RS: Relay server is not authorized. Make sure your key is correct.\n");
        return false;
    }

    return true;
}
