
#pragma once

#include "g_local.h"

void mock_init();
cvar_t* mock_cvar(const char* name, const char* value);
void mock_free_cvar(cvar_t* cvar);

edict_t* mock_player();
void mock_player_free(edict_t* player);
