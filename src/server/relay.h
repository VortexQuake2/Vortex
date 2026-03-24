//
// Created by dahum on 30-10-2024.
//

#ifndef RELAY_H
#define RELAY_H

#include <msgpack.h>

void vrx_relay_connect();
void vrx_relay_disconnect();
void vrx_relay_message(const char* message);
void vrx_relay_recv();
qboolean vrx_relay_is_connected();
void vrx_relay_notify_client_begin(const char* name);
void vrx_relay_notify_client_disconnected(const char* name);
void vrx_relay_notify_spawn_entities(const char* name);
bool vrx_relay_send_sbuffer(msgpack_sbuffer* sbuf);


// stash impl
void vrx_setup_relay_stash_io();

#endif //RELAY_H
