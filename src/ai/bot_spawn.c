/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

#include "g_local.h"
#include "ai_local.h"


//===============================================================
//
//				BOT SPAWN
//
//===============================================================

void BOT_DebugInventory(edict_t* self)
{
	int i;
	gitem_t* it;
	for (i = 0; i < MAX_ITEMS; i++)
	{
		if (!self->client->pers.inventory[i])
			continue;
		it = &itemlist[i];
		//if (it->flags & IT_WEAPON)
		//	gi.dprintf("%s\n", GetWeaponString(self->client->pers.inventory[i]));
		//else
			gi.dprintf("%s: %d\n", it->pickup_name, self->client->pers.inventory[i]);
	}
}

///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_Respawn (edict_t *self)
{
	int Windex;
	gitem_t* it;
	//gi.dprintf("BOT_Respawn\n");
	CopyToBodyQue (self);

	PutClientInServer (self);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;

	AI_ResetWeights(self);
	AI_ResetNavigation(self);

	Windex = vrx_WeapIDtoWeapIndex(self->myskills.respawn_weapon);
	it = &itemlist[Windex];
	//gi.dprintf("bot REspawned in game, weapon: %s\n", it->pickup_name);
	//BOT_DebugInventory(self);
}


///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
edict_t *BOT_FindFreeClient (void)
{
	edict_t *bot;
	int	i;
	int max_count=0;
	
	// This is for the naming of the bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;
		
		if(bot->count > max_count)
			max_count = bot->count;
	}

	// Check for free spot
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (!bot->inuse)
			break;
	}

	bot->count = max_count + 1; // Will become bot name...

	if (bot->inuse)
		bot = NULL;
	
	return bot;
}

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
void BOT_SetName(edict_t *bot, char *name, char *skin, char *team)
{
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];

	// Set the name for the bot.
	// name
	if(strlen(name) == 0)
		sprintf(bot_name,"Bot%d",bot->count);
	else
		strcpy(bot_name,name);

	// skin
	if(!skin || strlen(skin) == 0)
	{
		// randomly choose skin 
		float rnd = random();
		if(rnd  < 0.05)
			sprintf(bot_skin,"female/athena");
		else if(rnd < 0.1)
			sprintf(bot_skin,"female/brianna");
		else if(rnd < 0.15)
			sprintf(bot_skin,"female/cobalt");
		else if(rnd < 0.2)
			sprintf(bot_skin,"female/ensign");
		else if(rnd < 0.25)
			sprintf(bot_skin,"female/jezebel");
		else if(rnd < 0.3)
			sprintf(bot_skin,"female/jungle");
		else if(rnd < 0.35)
			sprintf(bot_skin,"female/lotus");
		else if(rnd < 0.4)
			sprintf(bot_skin,"female/stiletto");
		else if(rnd < 0.45)
			sprintf(bot_skin,"female/venus");
		else if(rnd < 0.5)
			sprintf(bot_skin,"female/voodoo");
		else if(rnd < 0.55)
			sprintf(bot_skin,"male/cipher");
		else if(rnd < 0.6)
			sprintf(bot_skin,"male/flak");
		else if(rnd < 0.65)
			sprintf(bot_skin,"male/grunt");
		else if(rnd < 0.7)
			sprintf(bot_skin,"male/howitzer");
		else if(rnd < 0.75)
			sprintf(bot_skin,"male/major");
		else if(rnd < 0.8)
			sprintf(bot_skin,"male/nightops");
		else if(rnd < 0.85)
			sprintf(bot_skin,"male/pointman");
		else if(rnd < 0.9)
			sprintf(bot_skin,"male/psycho");
		else if(rnd < 0.95)
			sprintf(bot_skin,"male/razor");
		else 
			sprintf(bot_skin,"male/sniper");
	}
	else
		strcpy(bot_skin,skin);

	// initialise userinfo
	memset (userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey (userinfo, "name", bot_name);
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!
	Info_SetValueForKey(userinfo, "ip", "127.0.0.1");

	ClientConnect (bot, userinfo);

//	ACESP_SaveBots(); // make sure to save the bots
}

//==========================================
// BOT_NextCTFTeam
// Get the emptier CTF team
//==========================================
int	BOT_NextCTFTeam()
{
	int	i;
	int	onteam1 = 0;
	int	onteam2 = 0;
	edict_t		*self;

	// Only use in CTF games
	if (!ctf->value)
		return 0;

	for (i = 0; i < game.maxclients + 1; i++)
	{
		self = g_edicts +i + 1;
		if (self->inuse && self->client)
		{
			if (self->teamnum == CTF_TEAM1)
				onteam1++;
			else if (self->teamnum == CTF_TEAM2)
				onteam2++;
		}
	}

	if (onteam1 > onteam2)
		return (2);
	else if (onteam2 >= onteam1)
		return (1);

	return (1);
}

//==========================================
// BOT_JoinCTFTeam
// Assign a team for the bot
//==========================================
qboolean BOT_JoinCTFTeam (edict_t *ent, char *team_name)
{
	char	*s;
	int		team;
//	edict_t	*event;


	if (ent->teamnum != CTF_NOTEAM)
		return false;
	
	// find what ctf team
	if ((team_name !=NULL) && (strcmp(team_name, "blue") == 0))
		team = CTF_TEAM2;
	else if ((team_name !=NULL) && (strcmp(team_name, "red") == 0))
		team = CTF_TEAM1;
	else
		team = BOT_NextCTFTeam();

	if (team == CTF_NOTEAM)
		return false;
	
	//join ctf team
	ent->svflags &= ~SVF_NOCLIENT;
	//ent->client->resp.ctf_state = 1;//0?
	ent->teamnum = team;
	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	// az todo: assign skin
	// CTFAssignSkin(ent, s);

	PutClientInServer(ent);

	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;

	/* // az: fixme
	Com_Printf ( "%s joined the %s team.\n",
		ent->client->pers.netname, CTFTeamName(ent->teamnum));
		*/

	return true;
}

#define skill(sk,cost) if(cost > ent->myskills.speciality_points)\
	mval = ent->myskills.speciality_points;\
	{\
		ent->myskills.abilities[sk].level += mval;\
		ent->myskills.abilities[sk].current_level += mval;\
		ent->myskills.speciality_points -= mval;\
	}

void BOT_UpgradeSkill(edict_t* ent, int ability_index, int amount)
{
	if (!ent->ai.is_bot)
		return;
	if (amount > ent->myskills.speciality_points)
		amount = ent->myskills.speciality_points;
	ent->myskills.abilities[ability_index].level += amount;
	ent->myskills.abilities[ability_index].current_level += amount;
	ent->myskills.speciality_points -= amount;
}

void BOT_SoldierAssignSkills(edict_t *ent)
{
	BOT_UpgradeSkill(ent, STRENGTH, 10);
	BOT_UpgradeSkill(ent, CREATE_QUAD, 1);
	BOT_UpgradeSkill(ent, EMP, 10);
	BOT_UpgradeSkill(ent, RESISTANCE, 5);
	BOT_UpgradeSkill(ent, VITALITY, 5);
	BOT_UpgradeSkill(ent, RESISTANCE, 5);
	BOT_UpgradeSkill(ent, STRENGTH, 5);
	BOT_UpgradeSkill(ent, RESISTANCE, 5);
	BOT_UpgradeSkill(ent, CREATE_INVIN, 1);
	BOT_UpgradeSkill(ent, VITALITY, 5);
	BOT_UpgradeSkill(ent, NAPALM, 10);
	BOT_UpgradeSkill(ent, MIRV, 10);
}

void BOT_MageAssignSkills(edict_t* ent)
{
	BOT_UpgradeSkill(ent, MAGICBOLT, 1);
	BOT_UpgradeSkill(ent, POWER_REGEN, 3); // clvl 2
	BOT_UpgradeSkill(ent, MAGICBOLT, 9);
	BOT_UpgradeSkill(ent, POWER_REGEN, 2); // clvl 8
	BOT_UpgradeSkill(ent, METEOR, 10); // clvl 13
	BOT_UpgradeSkill(ent, NOVA, 10);
	BOT_UpgradeSkill(ent, MAGICBOLT, 5);
	BOT_UpgradeSkill(ent, METEOR, 5);
	BOT_UpgradeSkill(ent, NOVA, 5);
	BOT_UpgradeSkill(ent, LIGHTNING, 10);
	BOT_UpgradeSkill(ent, FIREBALL, 10);
	BOT_UpgradeSkill(ent, LIGHTNING_STORM, 10);
}

void BOT_KnightAssignSkills(edict_t* ent)
{
	BOT_UpgradeSkill(ent, PLASMA_BOLT, 1);
	BOT_UpgradeSkill(ent, ARMOR_REGEN, 1);
	BOT_UpgradeSkill(ent, REGENERATION, 1);
	BOT_UpgradeSkill(ent, ARMOR_UPGRADE, 10); // clvl 2-7
	BOT_UpgradeSkill(ent, REGENERATION, 4);
	BOT_UpgradeSkill(ent, VITALITY, 3); // clvl 10
	BOT_UpgradeSkill(ent, ARMOR_REGEN, 4); // clvl 11-12
	BOT_UpgradeSkill(ent, PLASMA_BOLT, 4); // clvl 13-14
	BOT_UpgradeSkill(ent, POWER_REGEN, 2); // clvl 15
	BOT_UpgradeSkill(ent, ARMOR_REGEN, 5);
	BOT_UpgradeSkill(ent, REGENERATION, 5); // clvl 20
	BOT_UpgradeSkill(ent, PLASMA_BOLT, 5);
	BOT_UpgradeSkill(ent, POWER_SHIELD, 10);
}

void BOT_VampireAssignSkills(edict_t* ent)
{
	BOT_UpgradeSkill(ent, VAMPIRE, 5);
	BOT_UpgradeSkill(ent, GHOST, 5); // clvl 5
	BOT_UpgradeSkill(ent, BLINKSTRIKE, 5);
	BOT_UpgradeSkill(ent, POWER_REGEN, 1); // clvl 8
	BOT_UpgradeSkill(ent, VITALITY, 4); // clvl 10
	BOT_UpgradeSkill(ent, GHOST, 5);
	BOT_UpgradeSkill(ent, VAMPIRE, 5); // clvl 15
	BOT_UpgradeSkill(ent, BLINKSTRIKE, 5);
	BOT_UpgradeSkill(ent, VITALITY, 5);
	BOT_UpgradeSkill(ent, PLAGUE, 10); // clvl 25
}

void BOT_NecroAssignSkills(edict_t* ent)
{
	BOT_UpgradeSkill(ent, POWER_REGEN, 4); // clvl 2
	BOT_UpgradeSkill(ent, SKELETON, 10); // clvl 7
	BOT_UpgradeSkill(ent, POWER_REGEN, 2); // clvl 8
	BOT_UpgradeSkill(ent, AMP_DAMAGE, 10); // clvl 13
	BOT_UpgradeSkill(ent, SKELETON, 5); // clvl 16
	BOT_UpgradeSkill(ent, STATIC_FIELD, 10); // clvl 21
	BOT_UpgradeSkill(ent, AMP_DAMAGE, 5); // clvl 23
}

void BOT_UpgradeWeapon(edict_t* ent, int weapID)
{
	int i, w, max;

	//gi.dprintf("%d weapon points\n", ent->myskills.weapon_points);

	// converts weapon ID stored in respawn_weapon to weapon index needed for weapon upgrades
	// this is confusing because it doesn't exactly align with the values needed for inventory
	/*
	if (weapID == 1)
		w = WEAPON_SWORD;
	else if (weapID == 11)
		w = WEAPON_HANDGRENADE;
	else if (weapID == 13)
		w = WEAPON_BLASTER;
	else
		w = weapID - 1;*/
	w = AI_RespawnWeaponToWeapIndex(weapID);
	for (i = 0; i < MAX_WEAPONMODS;++i)
	{
		while (ent->myskills.weapon_points > 0)
		{
			max = ent->myskills.weapons[w].mods[i].soft_max;
			//gi.dprintf("soft max: %d level: %d\n", max, ent->myskills.weapons[weapon_index].mods[i].current_level);
			if (ent->myskills.weapons[w].mods[i].current_level < max)
			{
				ent->myskills.weapons[w].mods[i].current_level++;
				ent->myskills.weapon_points--;
				continue;
			}
			break;
		}
		//int Windex = vrx_WeapIDtoWeapIndex(weapID);
		//gitem_t* it = &itemlist[Windex];
		//gi.dprintf("%s (%d) %s upgraded to level %d (%d%%)\n", it->pickup_name, w, GetModString(w,i), 
		//	ent->myskills.weapons[w].mods[i].current_level, V_WeaponUpgradeVal(ent,w));
	}
	ent->ai.status.weaponWeights[w] = 0.01 * V_WeaponUpgradeVal(ent, w);
}

void BOT_UpgradeWeapons(edict_t* ent)
{
	//BOT_UpgradeWeapon(ent, vrx_WeapIDtoWeapIndex(ent->myskills.respawn_weapon));
	BOT_UpgradeWeapon(ent, ent->myskills.respawn_weapon);
}

// randomly selects a class-appropriate respawn weapon
void BOT_SelectRespawnWeapon(edict_t* ent)
{
	int soldierWeapons[] = { 2,3,4,5,7,8,12 };
	int necroWeapons[] = { 7,8,9 };
	int mageWeapons[] = { 2,4,5,9,12 }; // medium-long range hitscan preferred
	int vampireWeapons[] = { 1,2,3,4,5 }; // short range preferred
	int* weaponArray;
	int weaponCount, randomIndex;

	switch (ent->myskills.class_num)
	{
	case CLASS_KNIGHT:
		// knights can only use sword
		ent->myskills.respawn_weapon = 1;
		return;
	case CLASS_SOLDIER: 
		weaponArray = soldierWeapons;
		weaponCount = sizeof(soldierWeapons) / sizeof(soldierWeapons[0]);
		break;
	case CLASS_MAGE:
		weaponArray = mageWeapons;
		weaponCount = sizeof(mageWeapons) / sizeof(mageWeapons[0]);
		break;
	case CLASS_VAMPIRE:
		weaponArray = vampireWeapons;
		weaponCount = sizeof(vampireWeapons) / sizeof(vampireWeapons[0]);
		break;
	case CLASS_NECROMANCER:
		weaponArray = necroWeapons;
		weaponCount = sizeof(necroWeapons) / sizeof(necroWeapons[0]);
		break;
	default:
		ent->myskills.respawn_weapon = GetRandom(1, 13);
		return;
	}

	//weaponCount = sizeof(&weaponArray) / sizeof(&weaponArray[0]);
	randomIndex = GetRandom(0, weaponCount-1);
	int i = weaponArray[randomIndex];
	//gi.dprintf("%s: weaponCount: %d randomIndex: %d selected: %d\n", __func__, weaponCount, randomIndex, i);
	ent->myskills.respawn_weapon = i;
}

void BOT_VortexAssignSkills(edict_t *ent)
{
	switch (ent->myskills.class_num)
	{
	case CLASS_SOLDIER: BOT_SoldierAssignSkills(ent); break;
	case CLASS_MAGE: BOT_MageAssignSkills(ent); break;
	case CLASS_KNIGHT: BOT_KnightAssignSkills(ent); break;
	case CLASS_VAMPIRE: BOT_VampireAssignSkills(ent); break;
	case CLASS_NECROMANCER: BOT_NecroAssignSkills(ent); break;
	default:
		BOT_SoldierAssignSkills(ent);
	}
}
//==========================================
// BOT_DMClass_Touchdown
// called when the bot touches the ground after jumping
//==========================================
void BOT_Touchdown(edict_t* self)
{
	vec3_t jump_start, jump_end, v;

	gi.dprintf("groundentity:%s ", self->groundentity ? "true" : "false");
	gi.dprintf("is_step:%s ", self->ai.is_step ? "true" : "false");
	gi.dprintf("was_step:%s\n", self->ai.was_step ? "true" : "false");

	VectorCopy(self->monsterinfo.spot1, jump_start);
	jump_start[2] = 0;
	VectorCopy(self->s.origin, jump_end);
	jump_end[2] = 0;
	VectorSubtract(jump_end, jump_start, v);

	gi.dprintf("jump landed, dist: %f\n", VectorLength(v));
	gi.sound(self, CHAN_VOICE, gi.soundindex("world/land.wav"), 1, ATTN_IDLE, 0);
}
//==========================================
// BOT_DMClass_JoinGame
// put the bot into the game.
//==========================================
void BOT_DMClass_JoinGame (edict_t *ent, char *team_name)
{
	char *s;
	//int rnd = CLASS_PALADIN;

	//gi.dprintf("%s called for %s\n", __func__, ent->ai.pers.netname);

	if ( !BOT_JoinCTFTeam(ent, team_name) )
		gi.bprintf (PRINT_HIGH,  "[BOT] %s joined the game.\n",
		ent->client->pers.netname);

	ent->think = AI_Think;
	ent->nextthink = level.time + FRAMETIME;
	//ent->monsterinfo.touchdown = BOT_Touchdown;//GHz: for testing

	// az: Vortex stuff
    vrx_disable_abilities(ent);

	ent->myskills.level = AveragePlayerLevel();
	ent->myskills.speciality_points = ent->myskills.level * 2;
	ent->myskills.weapon_points = ent->myskills.level * 4;//GHz

	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");

	/*while (1) // except the knight, any class.
	{
		rnd = GetRandom(1, CLASS_MAX);
		if (rnd != CLASS_PALADIN && rnd != CLASS_POLTERGEIST)
			break;
	}

	ent->myskills.class_num = rnd;*/
	if (!ent->myskills.class_num)
	{
		float r = random();
		if (r < 0.2)
			ent->myskills.class_num = CLASS_SOLDIER;
		else if (r < 0.4)
			ent->myskills.class_num = CLASS_KNIGHT;
		else if (r < 0.6)
			ent->myskills.class_num = CLASS_VAMPIRE;
		else if (r < 0.8)
			ent->myskills.class_num = CLASS_NECROMANCER;
		else
			ent->myskills.class_num = CLASS_MAGE;
	}
	// for respawn_weapon index values, see vrx_WeapIDtoWeapIndex
	//if (random() > 0.8)
	//	ent->myskills.respawn_weapon = 9;//GetRandom(1, 11);
	//else
	//	ent->myskills.respawn_weapon = 1;//GetRandom(1, 11);
	//if (ent->myskills.class_num == CLASS_KNIGHT)
	//	ent->myskills.respawn_weapon = 1;
	//else
	//	ent->myskills.respawn_weapon = GetRandom(1, 13);
	BOT_SelectRespawnWeapon(ent);//GHz
	vrx_reset_weapon_maximums(ent);//GHz
	BOT_UpgradeWeapons(ent);//GHz

	ent->client->pers.spectator = false;
	ent->client->resp.spectator = false;

    vrx_add_respawn_items(ent);//GHz: isn't this redundant with PutClientInServer() below?
    vrx_add_respawn_weapon(ent, ent->myskills.respawn_weapon);//GHz: isn't this redundant with PutClientInServer() below?

    vrx_assign_abilities(ent);
    vrx_set_talents(ent);
	ent->myskills.streak = 0;

	BOT_VortexAssignSkills(ent);

	//join game
	ent->movetype = MOVETYPE_WALK;
	ent->solid = SOLID_BBOX;
	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->ps.gunindex = 0;

	PutClientInServer(ent);

	if (!KillBox (ent))
	{	// could't spawn in?
	}
	gi.linkentity (ent);

	int Windex = vrx_WeapIDtoWeapIndex(ent->myskills.respawn_weapon);
	gitem_t *it = &itemlist[Windex];
	//gi.dprintf("bot spawned in game, weapon: %s\n", it->pickup_name);
}

//==========================================
// BOT_StartAsSpectator
//==========================================
void BOT_StartAsSpectator (edict_t *ent)
{
	//gi.dprintf("BOT_StartAsSpectator\n");

	// start as 'observer'
	ent->movetype = MOVETYPE_NOCLIP;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->teamnum = CTF_NOTEAM;
	ent->client->ps.gunindex = 0;
	ent->client->pers.spectator = true;
	ent->client->resp.spectator = true;
	gi.linkentity (ent);
}


//==========================================
// BOT_JoinGame
// 3 for teams and such
//==========================================
void BOT_JoinBlue (edict_t *ent)
{
	BOT_DMClass_JoinGame( ent, "blue" );
}
void BOT_JoinRed (edict_t *ent)
{
	BOT_DMClass_JoinGame( ent, "red" );
}
void BOT_JoinGame (edict_t *ent)
{
	BOT_DMClass_JoinGame( ent, NULL );
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_SpawnBot (char *team, char *name, char *skin, char *userinfo, char *classname)
{
	edict_t	*bot;

	if( !nav.loaded ) {
		Com_Printf("Can't spawn bots without a valid navigation file\n");
		return;
	}
	
	bot = BOT_FindFreeClient ();
	
	if (!bot)
	{
//		safe_bprintf (PRINT_MEDIUM, "Server is full, increase Maxclients.\n");
		return;
	}

	//init the bot
	bot->inuse = true;
	bot->ai.is_bot = true;
	bot->yaw_speed = 100;

	// To allow bots to respawn
	if(userinfo == NULL)
		BOT_SetName(bot, name, skin, team);
	else
		ClientConnect (bot, userinfo);
	bot->ai.is_bot = true;//GHz: because this gets set to 'false' in ClientConnect
	G_InitEdict (bot);
	InitClientResp (bot->client);

	PutClientInServer(bot);
	BOT_StartAsSpectator (bot);

	//skill
	bot->ai.pers.skillLevel = (int)(random()*MAX_BOT_SKILL);
	if (bot->ai.pers.skillLevel > MAX_BOT_SKILL)	//fix if off-limits
		bot->ai.pers.skillLevel =  MAX_BOT_SKILL;
	else if (bot->ai.pers.skillLevel < 0)
		bot->ai.pers.skillLevel =  0;

	BOT_DMclass_InitPersistant(bot);
	AI_ResetWeights(bot);
	AI_ResetNavigation(bot);

	bot->think = BOT_JoinGame;

	// players should join before bots
	float delay = 0;
	if (vrx_get_joined_players(false) < 1)
		delay = 10;
	bot->nextthink = level.time + (GetRandom(10, 100) * FRAMETIME) + delay;

	if( ctf->value && team != NULL )
	{
		if( !Q_stricmp( team, "blue" ) )
			bot->think = BOT_JoinBlue;
		else if( !Q_stricmp( team, "red" ) )
			bot->think = BOT_JoinRed;
	}
	
	AI_EnemyAdded (bot); // let the ai know we added another

	// set character class, if specified
	if (classname)
	{
		if (!Q_stricmp(classname, "soldier"))
			bot->myskills.class_num = CLASS_SOLDIER;
		else if (!Q_stricmp(classname, "poltergeist"))
			bot->myskills.class_num = CLASS_POLTERGEIST;
		else if (!Q_stricmp(classname, "vampire"))
			bot->myskills.class_num = CLASS_VAMPIRE;
		else if (!Q_stricmp(classname, "mage"))
			bot->myskills.class_num = CLASS_MAGE;
		else if (!Q_stricmp(classname, "engineer"))
			bot->myskills.class_num = CLASS_ENGINEER;
		else if (!Q_stricmp(classname, "knight"))
			bot->myskills.class_num = CLASS_KNIGHT;
		else if (!Q_stricmp(classname, "cleric"))
			bot->myskills.class_num = CLASS_CLERIC;
		else if (!Q_stricmp(classname, "necromancer"))
			bot->myskills.class_num = CLASS_NECROMANCER;
		else if (!Q_stricmp(classname, "shaman"))
			bot->myskills.class_num = CLASS_SHAMAN;
		else if (!Q_stricmp(classname, "alien"))
			bot->myskills.class_num = CLASS_ALIEN;
		else if (!Q_stricmp(classname, "weaponmaster"))
			bot->myskills.class_num = CLASS_WEAPONMASTER;
	}
	//gi.dprintf("BOT_SpawnBot: weapon: %s\n", GetWeaponString(bot->myskills.respawn_weapon));
}


///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void BOT_RemoveBot(char *name)
{
	int i;
	qboolean freed = false;
	qboolean all = false;
	edict_t *bot;

	if (strcmp(name, "all") == 0)
		all = true;
	//gi.dprintf("BOT_RemoveBot\n");
	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			//gi.dprintf("found %s\n", bot->classname);
			if(bot->ai.is_bot && (all || strcmp(bot->client->pers.netname,name)==0))
			{
				bot->health = 0;
				player_die (bot, bot, bot, 100000, vec3_origin);
				// don't even bother waiting for death frames
				bot->deadflag = DEAD_DEAD;
				bot->inuse = false;
				freed = true;
				AI_EnemyRemoved (bot);
				safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
			}
		}
	}

	if(!all && !freed)	
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
	
	//ACESP_SaveBots(); // Save them again
}

edict_t* CTF_GetFlagBaseEnt(int teamnum);

// returns all bots back to their base
// teamnum is optional--use 0 for all bots/teams
void BOT_ReturnToBase(int teamnum)
{
	edict_t* bot;
	for (int i = 0; i < maxclients->value; i++)
	{
		bot = g_edicts + i + 1;
		// find live bots
		if (bot->inuse && bot->ai.is_bot && bot->health > 1)
		{
			edict_t* base = NULL;
			// find base entity
			if (ctf->value && (!teamnum || bot->teamnum == teamnum))
				base = CTF_GetFlagBaseEnt(bot->teamnum);
			else if (INVASION_OTHERSPAWNS_REMOVED)
				base = INV_SelectPlayerSpawnPoint(bot);
			if (!base || !base->inuse)
				continue;

			// if we're already close to base, no need to return to it!
			if (entdist(bot, base) < 1024)
				continue;

			// use tball to return to base immediately, if possible, otherwise pathfind home
			if (!BOT_DMclass_UseTball(bot, true))
			{
				int goal_node, current_node;

				// find the node closest to the bot
				if ((current_node = AI_FindClosestReachableNode(bot->s.origin, bot, NODE_DENSITY * 2, NODE_ALL)) == -1)
					continue;
				// this is our starting point
				bot->ai.current_node = current_node;

				// now find the node closest to the base
				if ((goal_node = AI_FindClosestReachableNode(base->s.origin, base, NODE_DENSITY * 4, NODE_ALL)) != -1)
				{
					// get the bot moving
					if (bot->ai.state != BOT_STATE_MOVEATTACK)
						bot->ai.state = BOT_STATE_MOVE;
					bot->ai.tries = 0;	// Reset the count of how many times we tried this goal

					if (AIDevel.debugChased && bot_showlrgoal->value)
						safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: RETURNING TO BASE @ node %d!\n", bot->ai.pers.netname, goal_node);
					//gi.dprintf("**** BASE UNDER ATTACK: RETURNING %s TO BASE! ****", bot->ai.pers.netname);
					AI_SetGoal(bot, goal_node, true);
					bot->goalentity = base;// used in AI_PickShortRangeGoal to prevent overriding LR goal pathfinding/movement
				}
			}
			//else
			//	gi.dprintf("**** BASE UNDER ATTACK: %s TELEPORTED BACK TO BASE! ****", bot->ai.pers.netname);
		}
	}
}