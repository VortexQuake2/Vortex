#include "g_local.h"

#ifdef CMD_USEHASH

#include "../../vendor/fnv.h"
#endif

void Cmd_IdentifyPlayer (edict_t *ent);
void Cmd_SetMasterPassword_f (edict_t *ent);
void Cmd_SetOwner_f (edict_t *ent);
void Cmd_TransCredits(edict_t *ent);
void Cmd_AdminCmd (edict_t *ent);
void Cmd_BuildLaser (edict_t *ent);
void Cmd_Thrust_f (edict_t *ent);
void Cmd_Overload_f (edict_t *ent);
void Cmd_HolyFreeze(edict_t *ent);
void Cmd_WritePos_f (edict_t *ent);
void Cmd_Rune_f(edict_t *ent);
void Cmd_Forcewall(edict_t *ent);
void ForcewallOff(edict_t *player);
void ArmageddonSpell(edict_t *ent);
void BuildMiniSentry (edict_t *ent);
void TeleportPlayer (edict_t *player);
void SpawnTestEnt (edict_t *ent);
void TeleportForward (edict_t *ent);
void lasersight_off (edict_t *ent);
void cmd_SentryGun(edict_t *ent);
void PlayerToParasite (edict_t *ent);
void Cmd_DetPipes_f (edict_t *ent);
void Cmd_ExplodingArmor_f (edict_t *ent);
void Cmd_StaticField_f(edict_t *ent);
void Cmd_SpawnMagmine_f (edict_t *ent);
void Cmd_ExplodingArmor_f (edict_t *ent);
void Cmd_Togglesecondary_f (edict_t *ent);
void Cmd_Spike_f (edict_t *ent);
void Cmd_BuildProxyGrenade (edict_t *ent);
void Cmd_Napalm_f (edict_t *ent);
void Cmd_PlayerToTank_f (edict_t *ent);
void Cmd_AutoCannon_f (edict_t *ent);
void Cmd_BlessedHammer_f (edict_t *ent);
void Cmd_WormHole_f (edict_t *ent);
void Cmd_SpikeGrenade_f (edict_t *ent);
void Cmd_Detector_f (edict_t *ent);
void Cmd_Conversion_f (edict_t *ent);
void Cmd_Deflect_f (edict_t *ent);
void Cmd_Antigrav_f (edict_t *ent);
void Cmd_TossEMP (edict_t *ent);
void Cmd_Plasmabolt_f (edict_t *ent);
void Cmd_TossMirv (edict_t *ent);
void Cmd_Healer_f (edict_t *ent);
void Cmd_Spiker_f (edict_t *ent);
void Cmd_Obstacle_f (edict_t *ent);
//void Cmd_box_f(edict_t *ent); //lepi
void Cmd_Gasser_f (edict_t *ent);
void Cmd_Raise_Skeleton_f(edict_t* ent);
void Cmd_Golem_f(edict_t* ent);
void Cmd_TossSpikeball (edict_t *ent);
//void Cmd_FireAcid_f (edict_t *ent);
void Cmd_Cocoon_f (edict_t *ent);
void Cmd_Meditate_f (edict_t *ent);
void Cmd_CreateLaserPlatform_f (edict_t *ent);
void Cmd_LaserTrap_f (edict_t *ent);
void Cmd_HolyGround_f (edict_t *ent);
void Cmd_UnHolyGround_f (edict_t *ent);
void Cmd_Purge_f (edict_t *ent);
void Cmd_Boomerang_f (edict_t *ent);
qboolean autoaim_findtarget (edict_t *ent);
void Cmd_AddNode_f (edict_t *ent);
void Cmd_DeleteNode_f (edict_t *ent);
void Cmd_DeleteAllNodes_f (edict_t *ent);
void Cmd_SaveNodes_f (edict_t *ent);
void Cmd_LoadNodes_f (edict_t *ent);
void Cmd_ComputeNodes_f (edict_t *ent);
void Cmd_ToggleShowGrid (edict_t *ent);
void Cmd_AddLink_f(edict_t* ent);
void Cmd_DeleteLink_f(edict_t* ent);
void Cmd_SelfDestruct_f(edict_t *self);
void Grenade_Explode (edict_t *ent);
void Cmd_CorpseExplode(edict_t *ent);
void Cmd_HellSpawn_f (edict_t *ent);
void Cmd_Caltrops_f (edict_t *ent);
//void Cmd_fmedi_f(edict_t *ent);
void Cmd_PrintCommandList(edict_t *ent);
void Cmd_Mirror_f(edict_t* ent);
void Cmd_BlinkStrike_f(edict_t* self);
void Cmd_ExplodingBarrel_f(edict_t* ent);
void Cmd_Stash_f(edict_t* ent);
void Cmd_LifeTap(edict_t* ent);
void Cmd_ShowPlinks_f(edict_t* ent);
void Cmd_AI_AddNode_f(edict_t* ent);
void Cmd_AI_RemoveNode_f(edict_t* ent);

#define CommandTotal sizeof(commands) / sizeof(gameCommand_s)

static qboolean Cmd_ParseInteger(const char *token, int *value)
{
	char *end;
	long parsed;

	if (!token || !*token)
		return false;

	parsed = strtol(token, &end, 10);
	if (!end || *end)
		return false;

	*value = (int)parsed;
	return true;
}

void Cmd_Immortal_f(edict_t* ent)
{
	const char *msg;

	if (!ent || !ent->client || !ent->myskills.administrator)
		return;

	ent->flags ^= FL_IMMORTAL;
	if (!(ent->flags & FL_IMMORTAL))
		msg = "immortal OFF\n";
	else
		msg = "immortal ON\n";

	safe_cprintf(ent, PRINT_HIGH, msg);
}

static qboolean Cmd_SpawnParseDroneType(enum dronespawn_t *drone_type)
{
	const char *arg;
	int value;

	if (gi.argc() < 2)
		return false;

	arg = gi.argv(1);

	if (!Q_strcasecmp(arg, "drone_mtype") || !Q_strcasecmp(arg, "mtype"))
	{
		if (gi.argc() < 3 || !Cmd_ParseInteger(gi.argv(2), &value))
			return false;

		return vrx_drone_spawn_type_from_mtype(value, drone_type);
	}

	if (!Q_strcasecmp(arg, "drone_type") || !Q_strcasecmp(arg, "dronespawn") || !Q_strcasecmp(arg, "ds"))
	{
		if (gi.argc() < 3 || !Cmd_ParseInteger(gi.argv(2), &value))
			return false;
		if (value < DS_GUNNER || value > DS_HEAVY_GUNNER)
			return false;

		*drone_type = (enum dronespawn_t)value;
		return true;
	}

	return vrx_parse_drone_spawn_type(arg, drone_type);
}

static qboolean Cmd_FindDroneSpawnPoint(edict_t *ent, edict_t *drone, vec3_t spawn_origin)
{
	trace_t tr;
	vec3_t forward, start, end, origin, from_player;
	float radius;
	int i;

	AngleVectors(ent->client->v_angle, forward, NULL, NULL);
	VectorCopy(ent->s.origin, start);
	start[2] += ent->viewheight;
	VectorMA(start, 8192, forward, end);

	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT | CONTENTS_MONSTERCLIP);
	if (tr.fraction == 1.0f)
	{
		VectorMA(start, 160, forward, origin);
	}
	else
	{
		VectorCopy(tr.endpos, origin);
		for (i = 0; i < 3; i++)
		{
			if (tr.plane.normal[i] > 0)
				origin[i] -= drone->mins[i] * tr.plane.normal[i];
			else
				origin[i] += drone->maxs[i] * -tr.plane.normal[i];
		}
	}

	radius = sqrtf((drone->maxs[0] - drone->mins[0]) * (drone->maxs[0] - drone->mins[0])
		+ (drone->maxs[1] - drone->mins[1]) * (drone->maxs[1] - drone->mins[1])) + 8.0f;

	for (i = 0; i < 10; i++)
	{
		tr = gi.trace(origin, drone->mins, drone->maxs, origin, drone, MASK_MONSTERSOLID);
		if (!tr.allsolid && !tr.startsolid && !(tr.contents & MASK_MONSTERSOLID))
		{
			VectorCopy(origin, spawn_origin);
			return true;
		}

		VectorMA(origin, -radius, forward, origin);
		VectorSubtract(origin, start, from_player);
		if (DotProduct(from_player, forward) < 0)
			break;
	}

	return false;
}

void Cmd_Spawn_f(edict_t* ent)
{
	enum dronespawn_t drone_type;
	edict_t *drone;
	vec3_t spawn_origin, to_player;

	if (!ent || !ent->client || !ent->myskills.administrator || ent->deadflag == DEAD_DEAD)
		return;

	if (gi.argc() < 2 || !Cmd_SpawnParseDroneType(&drone_type))
	{
		safe_cprintf(ent, PRINT_HIGH, "Usage: spawn <drone_name|drone_mtype> | spawn drone_mtype <mtype> | spawn drone_type <type>\n");
		safe_cprintf(ent, PRINT_HIGH, "Examples: spawn tank64, spawn heavy_gunner, spawn drone_mtype 53\n");
		return;
	}

	drone = vrx_create_drone_from_ent(G_Spawn(), world, drone_type, true, false, 0);
	if (!drone)
	{
		safe_cprintf(ent, PRINT_HIGH, "Failed to spawn drone.\n");
		return;
	}

	if (!Cmd_FindDroneSpawnPoint(ent, drone, spawn_origin))
	{
		safe_cprintf(ent, PRINT_HIGH, "Couldn't find a suitable spawn location.\n");
		M_Remove(drone, false, false);
		return;
	}

	VectorCopy(spawn_origin, drone->s.origin);
	VectorCopy(spawn_origin, drone->s.old_origin);
	VectorSubtract(ent->s.origin, spawn_origin, to_player);
	drone->s.angles[YAW] = vectoyaw(to_player);
	drone->ideal_yaw = drone->s.angles[YAW];
	drone->enemy = ent;
	gi.linkentity(drone);

	safe_cprintf(ent, PRINT_HIGH, "Spawned drone mtype %d.\n", drone->mtype);
}

const gameCommand_s commands[] =
{
	{ "medic", 			Cmd_PlayerToMedic_f },
	{ "autocannon", 	Cmd_AutoCannon_f },
	{ "blessedhammer", 	Cmd_BlessedHammer_f },
	{ "armorbomb", 		Cmd_ExplodingArmor_f },
	{ "vrxmenu", 		OpenGeneralMenu },
	{ "proxy", 			Cmd_BuildProxyGrenade },
	{ "napalm", 		Cmd_Napalm_f },
	{ "staticfield", 		Cmd_StaticField_f },
	{ "antigrav", 		Cmd_Antigrav_f },
	{ "masterpw", 		Cmd_SetMasterPassword_f },
	{ "owner", 			Cmd_SetOwner_f },
	{ "whois", 			OpenWhoisMenu },
	{ "bless", 			Cmd_Bless },
	{ "heal", 			Cmd_Healing },
	{ "cacodemon", 		Cmd_PlayerToCacodemon_f },
	{ "flyer", 			Cmd_PlayerToFlyer_f },
	{ "mutant", 		Cmd_PlayerToMutant_f },
	{ "brain", 			Cmd_PlayerToBrain_f },
	{ "tank", 			Cmd_PlayerToTank_f },
	{ "hellspawn", 		Cmd_HellSpawn_f },
	{ "supplystation", 	Cmd_CreateSupplyStation_f },
	{ "decoy", 			Cmd_Decoy_f },
	{ "stash", 		Cmd_Stash_f },
	{ "curse", 			Cmd_Curse },
	{ "amnesia", 		Cmd_Amnesia },
	{ "weaken", 		Cmd_Weaken },
	{ "lifedrain", 		Cmd_LifeDrain },
	{ "ampdamage", 		Cmd_AmpDamage },
	{ "lifetap", 		Cmd_LifeTap },
	{ "selfdestruct",	Cmd_SelfDestruct_f },
	{ "ally", 			ShowAllyMenu },
	{ "magmine",		Cmd_SpawnMagmine_f },
	{ "spike",			Cmd_Spike_f },
	{ "minisentry", 	Cmd_MiniSentry_f },
	{ "parasite", 		Cmd_PlayerToParasite_f },
	{ "teleport_fwd", 	TeleportForward },
	{ "admincmd", 		Cmd_AdminCmd },
	{ "transfercredits",Cmd_TransCredits },
	{ "forcewall", 		Cmd_Forcewall },
    { "forcewall_off", 	ForcewallOff },
	{ "laser", 			Cmd_BuildLaser },
    { "sentry", 		cmd_SentryGun },
	{ "lasersight", 	Cmd_LaserSight_f },
	{ "flashlight",     FL_toggle  },
	{ "monster", 		Cmd_Drone_f },
	{ "detpipes", 		Cmd_DetPipes_f  },
	{ "vrxinfo", 		OpenMyinfoMenu },
	{ "vrxarmory", 		vrx_armory_open_menu },
	{ "vrxrespawn", 	OpenRespawnWeapMenuFirstPage },
	{ "thrust",         Cmd_Thrust_f  },
	{ "vote", 			vrx_vote_cmd },
	{ "wormhole",	    Cmd_WormHole_f },
	{ "update",		    vrx_normalize_abilities},
	{ "berserker",	    Cmd_PlayerToBerserk_f },
	// { "medicpack",		 Cmd_fmedi_f }, //lepi
	{ "caltrops",	    Cmd_Caltrops_f },
	{ "spikegrenade",   Cmd_SpikeGrenade_f },
	{ "detector",	    Cmd_Detector_f },
	{ "convert",	    Cmd_Conversion_f },
	{ "deflect",	    Cmd_Deflect_f },
	{ "scanner",	    Toggle_Scanner },
	{ "emp",		    Cmd_TossEMP },
	{ "plasmabolt",	    Cmd_Plasmabolt_f },
	{ "mirv",		    Cmd_TossMirv },
	{ "healer",		    Cmd_Healer_f },
	{ "spiker",		    Cmd_Spiker_f },
	// { "box",				Cmd_box_f },
	{ "obstacle",	    Cmd_Obstacle_f },
	{ "gasser",		    Cmd_Gasser_f },
	{ "spore",		    Cmd_TossSpikeball },
	//{ "acid",		    Cmd_FireAcid_f },
	{ "cocoon",		    Cmd_Cocoon_f },
	{ "skeleton",		Cmd_Raise_Skeleton_f },
	{ "golem",			Cmd_Golem_f },
	{ "meditate"   ,	Cmd_Meditate_f },
	{ "overload",	    Cmd_Overload_f },
	{ "laserplatform",  Cmd_CreateLaserPlatform_f },
	//{ "lasertrap",	    Cmd_LaserTrap_f },
	{ "holyground",	    Cmd_HolyGround_f },
	{ "unholyground",   Cmd_UnHolyGround_f },
	{ "purge",		    Cmd_Purge_f },
	{ "boomerang",	    Cmd_Boomerang_f },

	// drone AI
	{ "loadnodes",	    Cmd_LoadNodes_f },
	{ "savenodes",	    Cmd_SaveNodes_f },
	{ "deletenode",	    Cmd_DeleteNode_f },
	{ "addnode",	    Cmd_AddNode_f },
	{ "deleteallnodes", Cmd_DeleteAllNodes_f },
	{ "computenodes",   Cmd_ComputeNodes_f },
	{ "showgrid",	    Cmd_ToggleShowGrid },
	{"addlink", Cmd_AddLink_f},
	{"dellink", Cmd_DeleteLink_f},
	// bot AI
	{ "showplinks",		Cmd_ShowPlinks_f },
	{ "aiaddnode",		Cmd_AI_AddNode_f },
	{ "airemovenode",	Cmd_AI_RemoveNode_f },

	// more vortex
	{ "writepos",	    Cmd_WritePos_f },
	{ "rune",		    Cmd_Rune_f },
	{ "vrxid",		    Cmd_IdentifyPlayer },
	{ "vrxcommands",    Cmd_PrintCommandList },
	{ "upgrade_ability",OpenUpgradeMenu },
	// { "spell_stealammo",Cmd_AmmoStealer_f },
	// { "ammosteal",		Cmd_AmmoStealer_f },
	{ "salvation",		Cmd_Salvation },
	{ "aura_salvation",	Cmd_Salvation },
	{ "spell_boost",	Cmd_BoostPlayer },
	{ "boost",	Cmd_BoostPlayer },
	{ "detonatebody", Cmd_CorpseExplode },
	{ "spell_corpseexplode", Cmd_CorpseExplode },
	{ "aura_holyfreeze", Cmd_HolyFreeze },
	{ "holyfreeze", Cmd_HolyFreeze },
	{ "togglesecondary", Cmd_Togglesecondary_f },
	{ "blinkstrike", Cmd_BlinkStrike_f },
	{ "barrel", Cmd_ExplodingBarrel_f }
};

#ifdef CMD_USEHASH
#define MAXCOMMANDS 20000

gameCommand_s hashedList[MAXCOMMANDS];

void TestHash()
{
	int count = 0, i, ccount = CommandTotal;
	for (i = 0; i < MAXCOMMANDS; i++)
	{
		if (hashedList[i].Function)
			count++;
	}
	if (count != ccount)
	{
		gi.dprintf("warning: hashing failed for commands.\n");
	}
}

void InitHash()
{
	int i;
	static qboolean initialized = false;

	if (initialized)
		return;

	gi.dprintf("Initializing command list.. ");

	for (i = 0; i < CommandTotal; i++)
	{
		const unsigned int index = fnv_32a_str(commands[i].FunctionName, FNV1_32A_INIT) % (MAXCOMMANDS);
		memcpy(&hashedList[index], &commands[i], sizeof(gameCommand_s));
	}

	gi.dprintf("Done.\n");

	TestHash();
	initialized = true;
}

qboolean VortexCommand(char* command, edict_t* ent)
{
	unsigned int index;

	if (G_IsSpectator(ent))
		return false;

	index = fnv_32a_str(command, FNV1_32A_INIT) % (MAXCOMMANDS);

	if (!hashedList[index].FunctionName || !hashedList[index].Function)
		return false;

	if (!Q_strcasecmp(hashedList[index].FunctionName, command)) // we found it
	{

		hashedList[index].Function(ent);
		return true;
	}
	
	return false;
};
#else

qboolean VortexCommand(char* command, edict_t* ent)
{
	int index;

	if (G_IsSpectator(ent))
		return false;

	for (index = 0; index < CommandTotal; index++)
	{
		if (!Q_stricmp(commands[index].FunctionName, command)) // we found it
		{
			commands[index].Function(ent);
			return true;
		}
	}
	
	return false;
};

#endif

static qboolean initialized = false;

void Cmd_PrintCommandList(edict_t *ent)
{
	static char bigstr[8192]; // Hopefully we don't need more than this.
	int i;

	if (!initialized)
	{
		bigstr[0] = '\0';
		strcat(bigstr, "Command List: \n");
		strcat(bigstr, "=============\n");
		for (i = 0; i < (sizeof(commands) / sizeof(gameCommand_s)); i++)
		{
			strcat(bigstr, commands[i].FunctionName);
			strcat(bigstr, "\n");
		}
		initialized = true;
	}

	safe_cprintf(ent, PRINT_LOW, "%s", bigstr);
}
