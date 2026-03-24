#ifndef VORTEX_RELAY_HANDLERS_H
#define VORTEX_RELAY_HANDLERS_H

#include "g_local.h"
#include <msgpack.h>

// relay server constants
extern const size_t HEADER_SIZE;
extern const uint32_t MAGIC;
extern const char* COMMAND_RELAY;

// global variables
extern int vrx_relay_socket;
extern qboolean relay_authorized;

// helper functions
msgpack_object_str* msgpack_get_map_string(msgpack_object_map* map, const char* key);
int msgpack_streq(msgpack_object_str* str, const char* cmp);

// handler functions
edict_t* vrx_relay_find_client(int connection_id);
qboolean vrx_relay_try_message_relay(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_character_loaded(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_stash_page(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_stash_open_result(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_stash_event(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_authorized(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);
qboolean vrx_relay_try_setowner_result(msgpack_object_str *type, msgpack_object_array *arr, qboolean* invalid);

typedef enum {
    RESULT_INVALID, // "we got an invalid message"
    RESULT_NEED_MORE_DATA, // "we need more data to parse the message"
    RESULT_CONTINUE, // "we got a message successfully, but continue parsing"
    RESULT_SUCCESS // "we got a message successfully, but stop parsing."
} relay_parse_result_t;

#endif
