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



///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_Respawn (edict_t *self)
{
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
	if(strlen(skin) == 0)
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
	mval = cost;\
	{\
		ent->myskills.abilities[sk].level += mval;\
		ent->myskills.abilities[sk].current_level += mval;\
		ent->myskills.speciality_points -= mval;\
	}

void BOT_SoldierAssignSkills(edict_t *ent)
{
	int mval = 10;
	
	skill(STRENGTH, 5);
	skill(HASTE, 2);
	skill(RESISTANCE, 5);
	skill(VITALITY, 5);
	skill(GRAPPLE_HOOK, 1);

	skill(HASTE, 2);
	
	skill(STRENGTH, 5);
	skill(VITALITY, 5);
	skill(RESISTANCE, 5);
	skill(HASTE, 1);
	skill(GRAPPLE_HOOK, 2);

	skill(STRENGTH, 5);
	skill(VITALITY, 5);
	skill(RESISTANCE, 5);
}

void BOT_VortexAssignSkills(edict_t *ent)
{
	switch (ent->myskills.class_num)
	{
	default:
		BOT_SoldierAssignSkills(ent);
	}
}

//==========================================
// BOT_DMClass_JoinGame
// put the bot into the game.
//==========================================
void BOT_DMClass_JoinGame (edict_t *ent, char *team_name)
{
	char *s;
	//int rnd = CLASS_PALADIN;

	gi.dprintf("BOT_DMClass_JoinGame\n");

	if ( !BOT_JoinCTFTeam(ent, team_name) )
		gi.bprintf (PRINT_HIGH,  "[BOT] %s joined the game.\n",
		ent->client->pers.netname);

	ent->think = AI_Think;
	ent->nextthink = level.time + FRAMETIME;

	// az: Vortex stuff
    vrx_disable_abilities(ent);

	ent->myskills.level = AveragePlayerLevel();
	ent->myskills.speciality_points = ent->myskills.level * 2;

	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");

	/*while (1) // except the knight, any class.
	{
		rnd = GetRandom(1, CLASS_MAX);
		if (rnd != CLASS_PALADIN && rnd != CLASS_POLTERGEIST)
			break;
	}

	ent->myskills.class_num = rnd;*/
	ent->myskills.class_num = CLASS_SOLDIER; 
	ent->myskills.respawn_weapon = GetRandom(1, 11);

	ent->client->pers.spectator = false;
	ent->client->resp.spectator = false;

    vrx_add_respawn_items(ent);
    vrx_add_respawn_weapon(ent, ent->myskills.respawn_weapon);

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
}

//==========================================
// BOT_StartAsSpectator
//==========================================
void BOT_StartAsSpectator (edict_t *ent)
{
	gi.dprintf("BOT_StartAsSpectator\n");

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
void BOT_SpawnBot (char *team, char *name, char *skin, char *userinfo)
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
	bot->nextthink = level.time + (int)(random() * 6.0);
	if( ctf->value && team != NULL )
	{
		if( !Q_stricmp( team, "blue" ) )
			bot->think = BOT_JoinBlue;
		else if( !Q_stricmp( team, "red" ) )
			bot->think = BOT_JoinRed;
	}
	
	AI_EnemyAdded (bot); // let the ai know we added another
}


///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void BOT_RemoveBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	//gi.dprintf("BOT_RemoveBot\n");
	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			//gi.dprintf("found %s\n", bot->classname);
			if(bot->ai.is_bot && (strcmp(bot->client->pers.netname,name)==0 || strcmp(name,"all")==0))
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

	if(!freed)	
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
	
	//ACESP_SaveBots(); // Save them again
}