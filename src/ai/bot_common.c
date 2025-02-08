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

*/

#include "g_local.h"
#include "ai_local.h"

//ACE



//==========================================
// BOT_Commands
// Special command processor
//==========================================
qboolean BOT_Commands(edict_t *ent)
{
	return false;
}

typedef struct {
	char* name;
	char* class;
} bot_selection_t;

bot_selection_t botlist[] = {
	{"robotico", "soldier"},
	{"faildozer", "soldier"},
	{"T-800", "soldier"},
	{"terminator", "soldier"},
	{"jailbroken", "mage"},
	{"Yandexter", "mage"},
	{"Taybot", "mage"},
	{"twitbot", "mage"},
	{"Bender", "knight"},
	{"reject", "knight"},
	{"junkbot", "knight"},
	{"R2-D2", "knight"},
	{"Optimus Prime", "knight"},
	{"WALL-E", "vampire"},
	{"Johnny 5", "vampire"},
	{"C-3PO", "vampire"},
	{"Roboflop", "vampire"},
	{"ED-209", "necromancer"},
	{"HAL 9000", "necromancer"},
	{"Roomba", "necromancer"},
	{"Eufy", "necromancer"}
};

#define LIST_SIZE (sizeof(botlist) / sizeof(botlist[0]))

int selected_indices[LIST_SIZE];  // Track used indices
int remaining = LIST_SIZE;        // Counter for remaining unique selections

bot_selection_t get_random_unique_entry() {
	if (remaining == 0) {
		// Reset selection tracking
		remaining = LIST_SIZE;
	}

	int index;
	int unique = 0;

	// Find an unused index
	while (!unique) {
		index = rand() % LIST_SIZE;
		unique = 1;
		for (int i = 0; i < (LIST_SIZE - remaining); i++) {
			if (selected_indices[i] == index) {
				unique = 0;
				break;
			}
		}
	}

	// Store the used index
	selected_indices[LIST_SIZE - remaining] = index;
	remaining--;

	return botlist[index];
}

char *bot_names[] = 
		{
			"robotico",
			"faildozer",
			"terminator",
			"jailbroken",
			"Yandexter",
			"Taybot",
			"twitbot",
			"Bender",
			"reject",
			"junkbot",
			"R2-D2",
			"Optimus Prime",
			"WALL-E",
			"Johnny 5",
			"C-3PO",
			"Roboflop",
			"ED-209",
			"HAL 9000"
		};

void BOT_AutoSpawn(void)
{
	int num_bots = bot_autospawn->value;
	char* name;
	if (!nav.loaded)
		return;
	if (num_bots < 1)
		return;
	// game modes currently unsupported
	if (ctf->value || domination->value || tbi->value || hw->value)
		return;
	for (int i = 0; i < num_bots; i++)
	{
		bot_selection_t e = get_random_unique_entry();
		//name = bot_names[GetRandom(0, sizeof(bot_names - 1))];
		name = e.name;
		BOT_SpawnBot(NULL, name, NULL, NULL, e.class);
	}
}

//==========================================
// BOT_ServerCommand
// Special server command processor
//==========================================
qboolean BOT_ServerCommand (void)
{
	char	*cmd, *name=NULL;

	cmd = gi.argv (1);

	if (!bot_enable->value)
		return false;
	//return false; // az: unused...

	if (ctf->value)
	{
		if (gi.argc() > 3)
			name = gi.argv(3);
	}
	else
	{
		if (gi.argc() > 2)
			name = gi.argv(2);
	}

	if (!name || strlen(name) < 5)
	{
		name = bot_names[GetRandom(0, sizeof(bot_names - 1))];
	}

	if( !Q_stricmp (cmd, "addbot") )
	{ 
		if(ctf->value) // team, name, class
			BOT_SpawnBot(gi.argv(2), gi.argv(3), NULL, NULL, gi.argv(4));//BOT_SpawnBot(gi.argv(2), gi.argv(3), gi.argv(4), NULL);
		else // name, class
			BOT_SpawnBot(NULL, name, NULL, NULL, gi.argv(3));//BOT_SpawnBot ( NULL, name, gi.argv(3), NULL );
	}	
	// removebot
    else if( !Q_stricmp (cmd, "removebot") )
    	BOT_RemoveBot(gi.argv(2));

	else if( !Q_stricmp (cmd, "editnodes") )
		AITools_InitEditnodes();

	else if( !Q_stricmp (cmd, "makenodes") )
		AITools_InitMakenodes();

	else if( !Q_stricmp (cmd, "savenodes") )
		AITools_SaveNodes();

	else if( !Q_stricmp (cmd, "addbotroam") )
		AITools_AddBotRoamNode();

	else if( !Q_stricmp (cmd, "addmonster") )
    	M_default_Spawn ();

	else if (!Q_stricmp(cmd, "aidebug"))
		AIDebug_ToogleBotDebug();

	else
		return false;

	return true;
}


//==========================================
// AI_BotObituary
// Bots can't use stock obituaries cause server doesn't print from them
//==========================================
void AI_BotObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
/*	int			mod;
	char		message[64];
	char		message2[64];
	qboolean	ff;

	if (coop->value && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;
	
	if ( deathmatch->value || coop->value )
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		
		GS_Obituary ( self, G_PlayerGender ( self ), attacker, mod, message, message2 );
		
		// duplicate message at server console for logging
		if ( attacker && attacker->client ) 
		{
			if ( attacker != self ) 
			{		// regular death message
				if ( deathmatch->value ) {
					if( ff )
						attacker->client->resp.score--;
					else
						attacker->client->resp.score++;
				}
				
				self->enemy = attacker;
				
				if( dedicated->value )
					G_Printf ( "%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2 );
				else
				{	//bot
					G_PrintMsg (NULL, PRINT_HIGH, "%s%s %s %s%s\n",
						self->client->pers.netname,
						S_COLOR_WHITE,
						message,
						attacker->client->pers.netname,
						message2);
				}
				
			} else {			// suicide
				
				if( deathmatch->value )
					self->client->resp.score--;
				
				self->enemy = NULL;
				
				if( dedicated->value )
					G_Printf ( "%s %s\n", self->client->pers.netname, message );
				else
				{	//bot
					G_PrintMsg (NULL, PRINT_HIGH, "%s%s %s\n",
						self->client->pers.netname,
						S_COLOR_WHITE,
						message );
				}
			}
			
		} else {		// wrong place, suicide, etc.
			
			if( deathmatch->value )
				self->client->resp.score--;
			
			self->enemy = NULL;
			
			if( dedicated->value )
				G_Printf( "%s %s\n", self->client->pers.netname, message );
			else
			{	//bot
				G_PrintMsg (NULL, PRINT_HIGH, "%s%s %s\n",
					self->client->pers.netname,
					S_COLOR_WHITE,
					message );
			}
		}
	}
*/
}


///////////////////////////////////////////////////////////////////////
// These routines are bot safe print routines, all id code needs to be 
// changed to these so the bots do not blow up on messages sent to them. 
// Do a find and replace on all code that matches the below criteria. 
//
// (Got the basic idea from Ridah)
//	
//  change: safe_cprintf to safe_cprintf
//  change: gi.bprintf to safe_bprintf
//  change: gi.centerprintf to safe_centerprintf
// 
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Debug print, could add a "logging" feature to print to a file
///////////////////////////////////////////////////////////////////////
void debug_printf(char *fmt, ...)
{
	int     i;
	char	bigbuffer[0x10000];
	int		len;
	va_list	argptr;
	edict_t	*cl_ent;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, PRINT_MEDIUM, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->ai.is_bot)
			continue;

		safe_cprintf(cl_ent,  PRINT_MEDIUM, bigbuffer);
	}

}

///////////////////////////////////////////////////////////////////////
// botsafe cprintf
///////////////////////////////////////////////////////////////////////
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
	char	bigbuffer[0x400];
	va_list		argptr;
	int len;

	if (ent && (!ent->inuse || ent->ai.is_bot))
		return;

	if (ent && !ent->client) // az: it happens.
		return;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.cprintf(ent, printlevel, bigbuffer);
	
}

///////////////////////////////////////////////////////////////////////
// botsafe centerprintf
///////////////////////////////////////////////////////////////////////
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
	char	bigbuffer[0x0400];
	va_list		argptr;
	int len;

	if (!ent)
	    return;

	if (!ent->inuse || ent->ai.is_bot)
		return;

	if (!ent->client)
		return;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	
	gi.centerprintf(ent, bigbuffer);
	
}

///////////////////////////////////////////////////////////////////////
// botsafe bprintf
///////////////////////////////////////////////////////////////////////
void safe_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char	bigbuffer[0x0400];
	int		len;
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, printlevel, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->ai.is_bot)
			continue;

		safe_cprintf(cl_ent, printlevel, bigbuffer);
	}
}

//GHz: for extended (spammy) debugging...
void AI_DebugPrintf (char* fmt, ...)
{
	char	bigbuffer[0x400];
	va_list		argptr;
	int len;
	
	if (!AIDevel.debugMode)
		return;
	va_start(argptr, fmt);
	len = vsprintf(bigbuffer, fmt, argptr);
	va_end(argptr);

	gi.dprintf(bigbuffer);

}
