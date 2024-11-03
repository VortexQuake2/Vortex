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

char *bot_names[] = 
		{
			"BOTty",
			"BOTon",
			"BOTeo",
			"BOTas",
			"BOTes",
			"BOTarengo",
			"BOTalocado",
			"BOTeonado"
		};


//==========================================
// BOT_ServerCommand
// Special server command processor
//==========================================
qboolean BOT_ServerCommand (void)
{
	char	*cmd, *name;

	cmd = gi.argv (1);

	//return false; // az: unused...

	if (!ctf->value)
		name = gi.argv(2);

	if (strlen(name) < 5)
	{
		name = bot_names[GetRandom(0, sizeof bot_names - 1)];
	}

	if( !Q_stricmp (cmd, "addbot") )
	{ 
		if(ctf->value) // name, skin, team
			BOT_SpawnBot ( gi.argv(2), gi.argv(3), gi.argv(4), NULL );
		else // name, skin
			BOT_SpawnBot ( NULL, name, gi.argv(3), NULL );
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
