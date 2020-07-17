#include "g_local.h"

void Weapon_Blaster (edict_t *ent);
void Weapon_GrenadeLauncher (edict_t *ent);
void Weapon_RocketLauncher (edict_t *ent);
void Weapon_HyperBlaster (edict_t *ent);
void Weapon_BFG (edict_t *ent);

#define AUTOAIM_TARGET_RADIUS		1024

qboolean autoaim_findtarget (edict_t *ent)
{
	edict_t *target = NULL;

	while ((target = findclosestreticle(target, ent, AUTOAIM_TARGET_RADIUS)) != NULL)
	{
		if (!G_ValidTarget(ent, target, true))
			continue;
		if (!nearfov(ent, target, 0, 20))
			continue;
		ent->enemy = target;
		return true;
	}
	return false;
}

void autoaim_getfiringparameters (edict_t *ent, int *speed, qboolean *rocket)
{
	if (ent->mtype)
	{
		switch(ent->mtype)
		{
		case MORPH_MUTANT: *speed = 500; break;
		case MORPH_MEDIC: *speed = 1500; break;
		case MORPH_FLYER: ent->client->weapon_mode?*speed=0:2000; break;
		case MORPH_CACODEMON: *speed = 950; *rocket = true; break;
		}
	}
	else if (PM_PlayerHasMonster(ent))
	{
		if ((ent->owner->mtype == P_TANK) && !ent->client->weapon_mode)
		{
			*speed = 650+30*ent->owner->monsterinfo.level;
			*rocket = true;
		}
	}
	else if (ent->client->pers.weapon)
	{
		if (ent->client->pers.weapon->weaponthink == Weapon_Blaster)
		{
			*speed = BLASTER_INITIAL_SPEED + BLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_BLASTER].mods[2].current_level;
		}
		else if (ent->client->pers.weapon->weaponthink == Weapon_GrenadeLauncher)
		{
			*speed = GRENADELAUNCHER_INITIAL_SPEED + GRENADELAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_GRENADELAUNCHER].mods[2].current_level;
			if (ent->enemy->groundentity)
				*rocket = true;
		}
		else if (ent->client->pers.weapon->weaponthink == Weapon_RocketLauncher)
		{
			*speed = ROCKETLAUNCHER_INITIAL_SPEED + ROCKETLAUNCHER_ADDON_SPEED * ent->myskills.weapons[WEAPON_ROCKETLAUNCHER].mods[2].current_level;
			*rocket = true;
		}
		else if (ent->client->pers.weapon->weaponthink == Weapon_HyperBlaster)
		{
			*speed = HYPERBLASTER_INITIAL_SPEED + HYPERBLASTER_ADDON_SPEED * ent->myskills.weapons[WEAPON_HYPERBLASTER].mods[2].current_level;
		}
		else if (ent->client->pers.weapon->weaponthink == Weapon_BFG)
		{
			*speed = BFG10K_INITIAL_SPEED + BFG10K_ADDON_SPEED * ent->myskills.weapons[WEAPON_BFG10K].mods[2].current_level;
			*rocket = true;
		}
	}
}

void autoaim_lockontarget (edict_t *ent)
{
	int			i, speed = 0;
	qboolean	rocket = false;
	vec3_t		forward, start;

	autoaim_getfiringparameters(ent, &speed, &rocket);

	//gi.dprintf("%d %s\n", speed, rocket?"true":"false");

	// calculate angle to enemy
	MonsterAim(ent, -1, speed, rocket, 0, forward, start);
	VectorNormalize(forward);
	vectoangles(forward, forward);
	if (forward[PITCH] < -180)
		forward[PITCH] += 360;

	// set view angles to target
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	for (i = 0 ; i < 3 ; i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(forward[i] - ent->client->resp.cmd_angles[i]);
	VectorCopy(forward, ent->client->ps.viewangles);
	VectorCopy(forward, ent->client->v_angle);
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
}

void V_AutoAim (edict_t *player)
{
	if (!player->client)
		return;
	if (player->myskills.administrator < 999)
		return;
	if (G_IsSpectator(player))
		return;

	//gi.dprintf("V_AutoAim()\n");

	// we have no previous target
	if (!player->enemy)
	{
		if (autoaim_findtarget(player))
		{
			//gi.dprintf("found target (%s)\n", player->enemy->classname?player->enemy->classname:"null");
			autoaim_lockontarget(player);
		}
	}
	// we've got an invalid target
	else if (!G_ValidTarget(player, player->enemy, true) || !infront(player, player->enemy))
	{
		//gi.dprintf("have invalid target");

		// try to find another target
		if (autoaim_findtarget(player))
			autoaim_lockontarget(player);
		else
		// couldn't find one, so we're done
			player->enemy = NULL;
	}
	// we have a valid target
	else
	{
		//gi.dprintf("have valid target\n");
		autoaim_lockontarget(player);
	}
}
/*
void LockOnTarget (edict_t *player)
{
	int			speed = 0;
	qboolean	rocket = false;
	int			i;
	vec3_t		forward, start;

	if (player->deadflag == DEAD_DEAD)
		return;
	if (que_typeexists(player->curses, CURSE_FROZEN))
		return;
	if (!G_ValidTarget(player, player->enemy, true))
		return;
	if (!infront(player, player->enemy))
		return;

	if (player->mtype)
	{
		switch(player->mtype)
		{
		case MORPH_MUTANT: speed = 500; break;
		case MORPH_MEDIC: speed = 1500; break;
		case MORPH_FLYER: player->client->weapon_mode?speed=0:2000; break;
		case MORPH_CACODEMON: speed = 950; rocket = true; break;
		}
	}
	else if (PM_PlayerHasMonster(player))
	{
		if ((player->owner->mtype = P_TANK) && !player->client->weapon_mode)
		{
			speed = 650+30*player->owner->monsterinfo.level;
			rocket = true;
		}
	}
	else if (player->client->pers.weapon)
	{
		if (player->client->pers.weapon->weaponthink == Weapon_Blaster)
		{
			speed = BLASTER_INITIAL_SPEED + BLASTER_ADDON_SPEED * player->myskills.weapons[WEAPON_BLASTER].mods[2].current_level;
		}
		else if (player->client->pers.weapon->weaponthink == Weapon_GrenadeLauncher)
		{
			speed = GRENADELAUNCHER_INITIAL_SPEED + GRENADELAUNCHER_ADDON_SPEED * player->myskills.weapons[WEAPON_GRENADELAUNCHER].mods[2].current_level;
			if (player->enemy->groundentity)
				rocket = true;
		}
		else if (player->client->pers.weapon->weaponthink == Weapon_RocketLauncher)
		{
			speed = ROCKETLAUNCHER_INITIAL_SPEED + ROCKETLAUNCHER_ADDON_SPEED * player->myskills.weapons[WEAPON_ROCKETLAUNCHER].mods[2].current_level;
			rocket = true;
		}
		else if (player->client->pers.weapon->weaponthink == Weapon_HyperBlaster)
		{
			speed = HYPERBLASTER_INITIAL_SPEED + HYPERBLASTER_ADDON_SPEED * player->myskills.weapons[WEAPON_HYPERBLASTER].mods[2].current_level;
		}
		else if (player->client->pers.weapon->weaponthink == Weapon_BFG)
		{
			speed = BFG10K_INITIAL_SPEED + BFG10K_ADDON_SPEED * player->myskills.weapons[WEAPON_BFG10K].mods[2].current_level;
			rocket = true;
		}
	}

	//GHz: Calculate angle to enemy
	MonsterAim(player, 1, speed, rocket, 0, forward, start);
	VectorNormalize(forward);
	vectoangles(forward, forward);
	if (forward[PITCH] < -180)
		forward[PITCH] += 360;

	//GHz: Set view angles to target
	player->client->ps.pmove.pm_type = PM_FREEZE;
	for (i = 0 ; i < 3 ; i++)
		player->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(forward[i] - player->client->resp.cmd_angles[i]);
	VectorCopy(forward, player->client->ps.viewangles);
	VectorCopy(forward, player->client->v_angle);
	player->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
}
*/