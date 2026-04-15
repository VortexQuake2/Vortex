//
// Created by zardoru on 01-03-26.
//

#include "game.h"
#include "q_recompat.h"

#include <stdarg.h>

#include "bg_local.h"
#include "g_local.h"


void vrx_repro_shim(game_import_t *_gi);

repro_import_t gire;

void vrx_repro_getgameapi(const repro_import_t *pr, game_import_t *_gi) {
    memcpy(&gire, pr, sizeof(repro_import_t));
    vrx_repro_shim(_gi);
}

#define VA_PRELUDE(x) va_list argptr;\
    va_start(argptr, fmt);\
    int len = vsnprintf(NULL, 0, fmt, argptr);\
    char _##x[len+1];\
	va_end(argptr);\
	va_start(argptr, fmt);\
    vsnprintf(_##x, len + 1, fmt, argptr);\
    _##x[len]='\0';\
    va_end(argptr);

void	shim_bprintf (const int printlevel, const char *fmt, ...) {
    VA_PRELUDE(msg)
    gire.Loc_Print(nullptr, printlevel | PRINT_BROADCAST, _msg, nullptr, 0);
}

void	shim_dprintf (const char *fmt, ...) {
	VA_PRELUDE(msg)
    gire.Com_Print(_msg);
}
void	shim_cprintf (const edict_t *ent, const int printlevel, const char *fmt, ...) {
    VA_PRELUDE(msg)
	gire.Loc_Print(ent, printlevel, _msg, nullptr, 0);
    // gire.Client_Print(ent, printlevel, _msg);
}
void	shim_centerprintf (const edict_t *ent,  const char *fmt, ...) {
    VA_PRELUDE(msg)
	gire.Loc_Print(ent, PRINT_CENTER, _msg, nullptr, 0);
    //gire.Center_Print(ent, _msg);
}

void shim_error(const char* fmt, ...) {
    VA_PRELUDE(msg)
    gire.Com_Error(_msg);
}


// clear out one-offs
void repro_prep_frame () {
	for (uint32_t i = 0; i < globals.num_edicts; i++)
		g_edicts[i].s.event = EV_NONE;

	// for (auto player : active_players())
	// 	player->client->ps.stats[STAT_HIT_MARKER] = 0;
	//
	// globals.server_flags &= ~SERVER_FLAG_INTERMISSION;
	//
	// if ( level.intermissiontime ) {
		// globals.server_flags |= SERVER_FLAG_INTERMISSION;
	// }
}

char* repro_write_game_json(bool autosave, size_t *out_size) { return nullptr; }
void repro_read_game_json(const char* json) { /* stub */ }
char* repro_write_level_json(bool autosave, size_t *out_size) { return nullptr; }
void repro_read_level_json(const char* json) {}

bool IsSlotIgnored(const edict_t *slot, edict_t **ignore, const size_t num_ignore)
{
	for (size_t i = 0; i < num_ignore; i++)
		if (slot == ignore[i])
			return true;

	return false;
}

edict_t *ClientChooseSlot_Any(edict_t **ignore, const size_t num_ignore)
{
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) && !game.clients[i].pers.connected)
			return globals.edicts + i + 1;

	return nullptr;
}

// inline edict_t *ClientChooseSlot_Coop(const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore)
// {
// 	char name[MAX_INFO_VALUE] = { 0 };
// 	gi.Info_ValueForKey(userinfo, "name", name, sizeof(name));
//
// 	// the host should always occupy slot 0, some systems rely on this
// 	// (CHECK: is this true? is it just bots?)
// 	{
// 		size_t num_players = 0;
//
// 		for (size_t i = 0; i < game.maxclients; i++)
// 			if (IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
// 				num_players++;
//
// 		if (!num_players)
// 		{
// 			gi.Com_PrintFmt("coop slot {} is host {}+{}\n", 1, name, social_id);
// 			return globals.edicts + 1;
// 		}
// 	}
//
// 	// grab matches from players that we have connected
// 	using match_type_t = int32_t;
// 	enum {
// 		MATCH_USERNAME,
// 		MATCH_SOCIAL,
// 		MATCH_BOTH,
//
// 		MATCH_TYPES
// 	};
//
// 	struct {
// 		edict_t *slot = nullptr;
// 		size_t total = 0;
// 	} matches[MATCH_TYPES];
//
// 	for (size_t i = 0; i < game.maxclients; i++)
// 	{
// 		if (IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
// 			continue;
//
// 		char check_name[MAX_INFO_VALUE] = { 0 };
// 		gi.Info_ValueForKey(game.clients[i].pers.userinfo, "name", check_name, sizeof(check_name));
//
// 		bool username_match = game.clients[i].pers.userinfo[0] &&
// 			!strcmp(check_name, name);
//
// 		bool social_match = social_id && game.clients[i].pers.social_id[0] &&
// 			!strcmp(game.clients[i].pers.social_id, social_id);
//
// 		match_type_t type = (match_type_t) 0;
//
// 		if (username_match)
// 			type |= MATCH_USERNAME;
// 		if (social_match)
// 			type |= MATCH_SOCIAL;
//
// 		if (!type)
// 			continue;
//
// 		matches[type].slot = globals.edicts + i + 1;
// 		matches[type].total++;
// 	}
//
// 	// pick matches in descending order, only if the total matches
// 	// is 1 in the particular set; this will prefer to pick
// 	// social+username matches first, then social, then username last.
// 	for (int32_t i = 2; i >= 0; i--)
// 	{
// 		if (matches[i].total == 1)
// 		{
// 			gi.Com_PrintFmt("coop slot {} restored for {}+{}\n", (ptrdiff_t) (matches[i].slot - globals.edicts), name, social_id);
//
// 			// spawn us a ghost now since we're gonna spawn eventually
// 			if (!matches[i].slot->inuse)
// 			{
// 				matches[i].slot->s.modelindex = MODELINDEX_PLAYER;
// 				matches[i].slot->solid = SOLID_BBOX;
//
// 				G_InitEdict(matches[i].slot);
// 				matches[i].slot->classname = "player";
// 				InitClientResp(matches[i].slot->client);
// 				spawn_from_begin = true;
// 				PutClientInServer(matches[i].slot);
// 				spawn_from_begin = false;
//
// 				// make sure we have a known default
// 				matches[i].slot->svflags |= SVF_PLAYER;
//
// 				matches[i].slot->sv.init = false;
// 				matches[i].slot->classname = "player";
// 				matches[i].slot->client->pers.connected = true;
// 				matches[i].slot->client->pers.spawned = true;
// 				P_AssignClientSkinnum(matches[i].slot);
// 				gi.linkentity(matches[i].slot);
// 			}
//
// 			return matches[i].slot;
// 		}
// 	}
//
// 	// in the case where we can't find a match, we're probably a new
// 	// player, so pick a slot that hasn't been occupied yet
// 	for (size_t i = 0; i < game.maxclients; i++)
// 		if (!IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) && !game.clients[i].pers.userinfo[0])
// 		{
// 			gi.Com_PrintFmt("coop slot {} issuing new for {}+{}\n", i + 1, name, social_id);
// 			return globals.edicts + i + 1;
// 		}
//
// 	// all slots have some player data in them, we're forced to replace one.
// 	edict_t *any_slot = ClientChooseSlot_Any(ignore, num_ignore);
//
// 	gi.Com_PrintFmt("coop slot {} any slot for {}+{}\n", !any_slot ? -1 : (ptrdiff_t) (any_slot - globals.edicts), name, social_id);
//
// 	return any_slot;
// }


edict_t *repro_choose_client_slot(const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, const size_t num_ignore, bool cinematic)
{
	// coop and non-bots is the only thing that we need to do special behavior on
	// az: no coop supported yet
	// if (!cinematic && coop->integer && !isBot)
	// 	return ClientChooseSlot_Coop(userinfo, social_id, isBot, ignore, num_ignore);

	// just find any free slot
	return ClientChooseSlot_Any(ignore, num_ignore);
}

void *repro_get_extension(const char *name)
{
	return nullptr;
}

void repro_bot_set_weapon(edict_t *botEdict, int weaponIndex, bool instantSwitch)
{
}

void repro_bot_trigger_edict(edict_t *botEdict, edict_t *edict)
{
}

void repro_bot_use_item(edict_t *botEdict, int32_t itemID)
{
}

int32_t repro_bot_get_item_id(const char *classname)
{
	return -1;
}

void repro_edict_force_look_at_point(edict_t *edict, const vec3_t point)
{
}

bool repro_bot_picked_up_item(edict_t *botEdict, edict_t *itemEdict)
{
	return false;
}

bool repro_visible_to_player(edict_t* ent, edict_t* player)
{
	return true;
	// return !ent->item_picked_up_by[player->s.number - 1];
}


// az: compared to og kex, we move this to levels
const shadow_light_data_t *repro_get_shadow_light_data(const int32_t entity_number)
{
	for (int32_t i = 0; i < level.shadow_lights.count; i++)
	{
		if (level.shadow_lights.info[i].entity_number == entity_number)
			return &level.shadow_lights.info[i].shadowlight;
	}

	return nullptr;
}

int shim_boxedicts(vec3_t mins, vec3_t maxs, edict_t **list, const int32_t maxcount, const enum solidity_area_t areatype) {
	return gire.BoxEdicts(mins, maxs, list, maxcount, areatype, nullptr, nullptr);
}

qboolean shim_inPVS(vec3_t p1, vec3_t p2) {
	return gire.inPVS(p1, p2, false);
}

qboolean shim_inPHS(vec3_t p1, vec3_t p2) {
	return gire.inPHS(p1, p2, false);
}

void shim_SetAreaPortalState(const int portalnum, const int state) {
	gire.SetAreaPortalState(portalnum, state);
}

int shim_AreasConnected(const int area1, const int area2) {
	return gire.AreasConnected(area1, area2);
}

void shim_multicast(vec3_t origin, const multicast_t to) {
	gire.multicast(origin, to, false);
}

void shim_unicast(edict_t* origin, const qboolean reliable) {
	static int32_t key = 1;
	gire.unicast(origin, reliable, key++);
}

void vrx_repro_shim(game_import_t *_gi) {
    _gi->bprintf = shim_bprintf;
    _gi->dprintf = shim_dprintf;
    _gi->cprintf = shim_cprintf;
    _gi->centerprintf = shim_centerprintf;
    _gi->sound = gire.sound;
    _gi->positioned_sound = gire.positioned_sound;

    _gi->configstring = gire.configstring;

    _gi->error = shim_error;

    _gi->modelindex = gire.modelindex;
    _gi->soundindex = gire.soundindex;
    _gi->imageindex = gire.imageindex;

    _gi->setmodel = gire.setmodel;

    _gi->trace = gire.trace;
    _gi->pointcontents = gire.pointcontents;
    _gi->inPVS = shim_inPVS;
    _gi->inPHS = shim_inPHS;
    _gi->SetAreaPortalState = shim_SetAreaPortalState;
    _gi->AreasConnected = shim_AreasConnected;

    _gi->linkentity = gire.linkentity;
    _gi->unlinkentity = gire.unlinkentity;
    _gi->BoxEdicts = shim_boxedicts;

    _gi->Pmove = Pmove;

    _gi->multicast = shim_multicast;
    _gi->unicast = shim_unicast;

    _gi->WriteChar = gire.WriteChar;
    _gi->WriteByte = gire.WriteByte;
    _gi->WriteShort = gire.WriteShort;
    _gi->WriteLong = gire.WriteLong;
    _gi->WriteFloat = gire.WriteFloat;
    _gi->WriteString = gire.WriteString;
    _gi->WritePosition = gire.WritePosition;
    _gi->WriteDir = gire.WriteDir;
    _gi->WriteAngle = gire.WriteAngle;

    _gi->TagMalloc = gire.TagMalloc;
    _gi->TagFree = gire.TagFree;
    _gi->FreeTags = gire.FreeTags;

    _gi->cvar = gire.cvar;
    _gi->cvar_set = gire.cvar_set;
    _gi->cvar_forceset = gire.cvar_forceset;

    _gi->argc = gire.argc;
    _gi->argv = gire.argv;
    _gi->args = gire.args;

    _gi->AddCommandString = gire.AddCommandString;
    _gi->DebugGraph = gire.DebugGraph;
}



#ifdef VRX_REPRO
void pm_set_viewheight(pmove_t *pm, int viewheight) {
}

#else
void pm_set_viewheight(pmove_t *pm, int viewheight) {
    pm->viewheight = viewheight;
}

int pm_get_viewheight(pmove_t *pm) {
    return pm->viewheight;
}
#endif