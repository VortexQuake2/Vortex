#include "g_local.h"

void ShowVoteMapMenu(edict_t *ent, int pagenum, int mapmode);

//************************************************************************************************
//		**VOTING UTILS**
//************************************************************************************************


votes_t currentVote = {false, 0, 0, "", ""};
int numVotes = 0, numVoteNo;
char* text1 = NULL, *text2 = NULL, *smode = NULL;
uint64_t voteTimeLeft = 0; // az: changed to frame-based because imprecision led to funky behaviour
qboolean voteRunning = false;
edict_t *voter;
static char strBuffer[1024];

//************************************************************************************************

#ifndef OLD_VOTE_SYSTEM
qboolean V_VoteInProgress()
{
	return voteRunning;
}
#endif

void V_VoteReset() {
	int i = 0;
	edict_t *e;
	numVotes = 0;
	numVoteNo = 0;
	voteTimeLeft = 0;
	voteRunning = false;

	memset (&currentVote, 0, sizeof(currentVote));

	for_each_player(e, i)
	{
		e->client->resp.HasVoted = false;
	}
}

void V_ChangeMap(v_maplist_t *maplist, int mapindex, int gamemode)
{
	//char buf[8];
	int timelimit;
	int fraglimit;

	vrx_lua_event("on_change_map");

	//Change the mode
	switch(gamemode)
	{
		case MAPMODE_PVP:
		{
			fraglimit = vrx_lua_get_int("pvp_fraglimit", 50);
			// player versus player
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "0");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", va("%d", fraglimit)); // vrxcl 5.0: lua fraglimits
			gi.cvar_set("timelimit", "0");
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//gi.cvar_set("dm_monsters", "0");				
		}
		break;
		case MAPMODE_PVM:
		{
			// player versus monsters
			timelimit = vrx_lua_get_variable("pvm_timelimit", 10)+pregame_time->value/60;
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "1");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//if (dm_monsters->value < 8)
			//	gi.cvar_set("dm_monsters", "8");
			//else
			//	gi.cvar_set("dm_monsters", itoa(ActivePlayers(), buf, 10));
		}
		break;
		case MAPMODE_DOM:
		{
			timelimit = vrx_lua_get_variable("dom_timelimit", 10)+pregame_time->value/60;
			// domination mode
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "1");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "0");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//gi.cvar_set("dm_monsters", "0");
		}
		break;
		case MAPMODE_CTF:
		{
			timelimit = vrx_lua_get_variable("ctf_timelimit", 10)+pregame_time->value/60;
			// ctf mode
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "1");
			gi.cvar_set("pvm", "0");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//gi.cvar_set("dm_monsters", "0");
		}
		break;
		case MAPMODE_FFA:
		{
			timelimit = vrx_lua_get_variable("ffa_timelimit", 15)+pregame_time->value/60;
			fraglimit = vrx_lua_get_int("ffa_fraglimit", 100);
			// free for all mode
			gi.cvar_set("ffa", "1");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "0");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", va("%d", fraglimit));
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//if (dm_monsters->value < 4)
			//	gi.cvar_set("dm_monsters", "4");
			//else
			//	gi.cvar_set("dm_monsters", itoa(ActivePlayers(), buf, 10));
		}
		break;
		case MAPMODE_INV:
		{
			timelimit = vrx_lua_get_variable("inv_timelimit", 20)+pregame_time->value/60;
			// invasion mode
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "1");
			gi.cvar_set("invasion", "1");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//gi.cvar_set("dm_monsters", "4");
		}
		break;
		case MAPMODE_INH:
		{
			timelimit = vrx_lua_get_variable("inh_timelimit", 25)+pregame_time->value/60;
			// invasion mode - hard
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "1");
			gi.cvar_set("invasion", "2");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("timelimit", va("%d", timelimit));
			gi.cvar_set("trading", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			//gi.cvar_set("dm_monsters", "4");
		}
		break;
		case MAPMODE_TRA:
		{
			gi.cvar_set("ffa", "0");
			gi.cvar_set("domination", "0");
			gi.cvar_set("ctf", "0");
			gi.cvar_set("pvm", "0");
			gi.cvar_set("invasion", "0");
			gi.cvar_set("fraglimit", "0");
			gi.cvar_set("trading", "1");
			gi.cvar_set("timelimit", "0");
			gi.cvar_set("hw", "0");
			gi.cvar_set("tbi", "0");
			break;
		}
		case MAPMODE_VHW: // vortex holy wars
			{
				timelimit = vrx_lua_get_variable("vhw_timelimit", 10)+pregame_time->value/60;
				gi.cvar_set("ffa", "0");
				gi.cvar_set("domination", "0");
				gi.cvar_set("ctf", "0");
				gi.cvar_set("pvm", "0");
				gi.cvar_set("invasion", "0");
				gi.cvar_set("fraglimit", "0");
				gi.cvar_set("trading", "0");
				gi.cvar_set("timelimit", va("%d", timelimit));
				gi.cvar_set("hw", "1");
				gi.cvar_set("tbi", "0");
				break;
			}
		case MAPMODE_TBI:
			{
				timelimit = vrx_lua_get_variable("dts_timelimit", 15)+pregame_time->value/60;
				gi.cvar_set("ffa", "0");
				gi.cvar_set("domination", "0");
				gi.cvar_set("ctf", "0");
				gi.cvar_set("pvm", "0");
				gi.cvar_set("invasion", "0");
				gi.cvar_set("fraglimit", "0");
				gi.cvar_set("trading", "0");
				gi.cvar_set("timelimit", va("%d", timelimit));
				gi.cvar_set("hw", "0");
				gi.cvar_set("tbi", "1");
			}
		break;
	}

	//Reset player votes
	V_VoteReset();

	gi.dprintf("Votes have been reset.\n");

	//Change the map
	if (mapindex < maplist->nummaps) // Assume everyone has proper maplists? Nope -az
	{
		strcpy(level.nextmap, maplist->maps[mapindex].name);
		level.r_monsters = maplist->maps[mapindex].monsters;//4.5
		gi.dprintf("Next map is: %s\n", level.nextmap);
	}else
	{
		gi.dprintf("Invalid map index (%d) defaulting to current map.\n", mapindex);
		strcpy(level.nextmap, level.mapname);
	}
	if (vrx_get_joined_players() == 0)
		ExitLevel();
}

//************************************************************************************************

v_maplist_t *GetMapList(int mode)
{
	if(vrx_lua_get_int("UseLuaMaplists", 0))
		vrx_load_map_list(mode); // reload the map list (lua conditional maplists)

	//Change the map
	switch(mode)
	{
	case MAPMODE_PVP:	return &maplist_PVP;
	case MAPMODE_DOM:	return &maplist_DOM;
	case MAPMODE_PVM:	return &maplist_PVM;
	case MAPMODE_CTF:	return &maplist_CTF;
	case MAPMODE_FFA:	return &maplist_FFA;
	case MAPMODE_INV:	return &maplist_INV;
	case MAPMODE_TRA:   return &maplist_TRA; // vrxchile 2.5: trading mode maplist
	case MAPMODE_INH:	return &maplist_INH; // vrxchile 2.6: invasion hard mode
	case MAPMODE_VHW:	return &maplist_VHW; // vrxchile 3.0: vortex holy wars mode
	case MAPMODE_TBI:	return &maplist_TBI; // vrxchile 3.4: destroy the spawn mode
	default:
		gi.dprintf("ERROR in GetMapList(). Incorrect map mode. (%d)\n", mode);
		return 0;
	}
}


//************************************************************************************************

void AddVote(edict_t *ent, int mode, int mapnum)
{
	v_maplist_t *maplist = GetMapList(mode);

	if (!maplist)
		return;

	if(V_VoteInProgress())
		return;
	
  
	//check for valid choice
	int players = vrx_get_joined_players();
	if (mode && (maplist->nummaps > mapnum))
	{
		if (maplist->maps[mapnum].min_players > players)
		{
			gi.cprintf(ent, PRINT_HIGH, "The map '%s' requires at least %d players.\n", maplist->maps[mapnum].name, maplist->maps[mapnum].min_players);
			return;
		}

		if (maplist->maps[mapnum].max_players < players && maplist->maps[mapnum].max_players > 0)
		{
			gi.cprintf(ent, PRINT_HIGH, "The map '%s' requires that there are at most %d players.\n", maplist->maps[mapnum].name, maplist->maps[mapnum].max_players);
			return;
		}

		char tempBuffer[1024];
		char tempBuffer2[1024];
		//Add the vote
		voter = ent;
		numVotes = 1;
		numVoteNo = 0;
		ent->client->resp.HasVoted = true;
		strcpy(currentVote.ip, ent->client->pers.current_ip);
		strcpy(currentVote.name, ent->myskills.player_name);
		currentVote.mapindex = mapnum;
		currentVote.mode = mode;
		currentVote.used = true;
		voteTimeLeft = level.framenum + qf2sf(900);
		voteRunning = true;


		Com_sprintf (tempBuffer, 1024, "%s started a vote for ", ent->myskills.player_name);
		switch(mode)
		{
			case MAPMODE_PVP:	smode =  "Player vs. Player (PvP) ";	break;
			case MAPMODE_PVM:	smode =  "Player vs. Monster (PvM) ";	break;
			case MAPMODE_DOM:	smode =  "Domination (DOM) ";			break;
			case MAPMODE_CTF:	smode =  "Capture The Flag (CTF) ";	break;
			case MAPMODE_FFA:	smode =  "Free For All (FFA) ";		break;
			case MAPMODE_INV:	smode =  "Invasion ";				break;
			case MAPMODE_TRA:	smode =  "Trading ";					break;
			case MAPMODE_INH:	smode =  "Invasion (Hard mode) ";	break;
			case MAPMODE_VHW:	smode =  "Vortex HolyWars ";			break;
			case MAPMODE_TBI:   smode =  "Destroy the Spawn ";		break;
		}
		Com_sprintf (tempBuffer2, 1024, "%s%son %s\n", tempBuffer, smode, maplist->maps[mapnum].name);

		text1 = HiPrint(va("%s", smode));
		text2 = HiPrint(va("%s", maplist->maps[mapnum].name));
		Com_sprintf (strBuffer, 1024, "vote in progress: %son %s\n", text1, text2);
		V_Free(text1); V_Free(text2);
		
		
		gi.configstring(CS_GENERAL+MAX_CLIENTS+1, strBuffer);

		G_PrintGreenText(tempBuffer2);
		gi.sound(&g_edicts[0], CHAN_VOICE, gi.soundindex("misc/comp_up.wav"), 1, ATTN_NONE, 0);

        uint64_t timeRem = (voteTimeLeft-level.framenum) / (uint64_t)sv_fps->value;
		gi.bprintf (PRINT_HIGH, "Please place your vote by typing 'vote yes' or 'vote no' within the next %u seconds.\n", timeRem);

	}
	else 
		gi.dprintf("Error in AddVote(): map number = %d, mode number = %d\n", mapnum, mode);
}

#define CHANGE_NOW		1	// change the map/mode immediately
#define CHANGE_LATER	2	// change the map/mode at the completion of vote duration

int V_VoteDone ()
{
	int players = vrx_get_joined_players();

	if (players < 1)
		return 0;

	if (numVotes >= 0.75*players)
		return CHANGE_NOW;

	if (numVotes > 0.5*players)
		return CHANGE_LATER;

	return 0;
}

//************************************************************************************************

//Finds the index of the map with the most votes.
//Returns: -1 if there is no winning map (a tie)
int FindBestMap(int mode)
{
	//if (numVotes > 0.5*ActivePlayers())
	//	return currentVote.mapindex;
	if (V_VoteDone())
		return currentVote.mapindex;
	return -1;
}

//************************************************************************************************


int V_AttemptModeChange(qboolean endlevel)
{


#if FORCE_PVP_WITH_A_LOT_OF_PLAYERS
	int max_players, players = ActivePlayers();

	//4.4 forcibly switch to PvP if we are in Invasion/PvM and there are many players
	max_players = 0.25 * maxclients->value;
	if (max_players < 4)
		max_players = 4;
	if (players >= max_players && (pvm->value || invasion->value) 
		&& (currentVote.mode == MAPMODE_PVM || currentVote.mode == MAPMODE_INV))
	{
		gi.bprintf(PRINT_HIGH, "Forcing switch to FFA because there are too many players!\n");
		return MAPMODE_FFA;
	}
#endif
	// did the vote pass?
	if (V_VoteDone())
		return currentVote.mode;

	// vote failed
	return 0;
}



void RunVotes ()
{
	if (!voteRunning)
		return;

	if (voteTimeLeft == level.framenum+qf2sf(600))
		gi.bprintf (PRINT_CHAT, "One minute left to place your vote.\n");
	else if (voteTimeLeft == level.framenum+qf2sf(300))
		gi.bprintf (PRINT_CHAT, "Thirty seconds left to place your vote.\n");
	else if (voteTimeLeft == level.framenum+qf2sf(100))
		gi.bprintf (PRINT_CHAT, "Ten seconds left to place your vote.\n");
	else if (voteTimeLeft <= level.framenum)
	{
		// Finish vote
		// Did we reach a majority?
		if (V_VoteDone())
		{
			// Tell everyone
			G_PrintGreenText("A majority was reached! Vote passed!\n");
			voteRunning = false;

			//Change the map
			// az note: internally, V_ChangeMap will reset the votes!
			EndDMLevel();
		}
		else
		{
			gi.bprintf (PRINT_CHAT, "Vote failed.\n");
			V_VoteReset();

			voter->client->resp.VoteTimeout = level.time + 20;
			voter = NULL;
		}
	}
	else
	{	
		if (V_VoteDone() == CHANGE_NOW)
		{
			// Tell everyone
			gi.sound(&g_edicts[0], CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NONE, 0);
			G_PrintGreenText("A majority was reached! Vote passed!\n");
			//Change the map
			EndDMLevel();
		}
	}

}


//************************************************************************************************
//		**VOTE MAP SELECT MENU**
//************************************************************************************************

void ShowVoteMapMenu_handler(edict_t *ent, int option)
{
	if (option == 66666)
	{
		closemenu(ent);
		return;
	}
	//Multi-page navigation

	if (option > 20000)
	{
		int mode = (option / 1000) - 20;
		int nextpage = option % 1000;		//page number we will end up in (page 0 = main menu)
		if (nextpage != 0)
			ShowVoteMapMenu(ent, nextpage, mode);
		else
			ShowVoteModeMenu(ent);
	}
	else
	{
		int mode = option / 1000;
		int mapnum = option % 1000;
		v_maplist_t *maplist = GetMapList(mode);

		if (!maplist) return;

		//Admins directly control the map change
		if (ent->myskills.administrator && adminctrl->value) // IF the cvar is enabled.
		{
			VortexEndLevel();
			V_ChangeMap(maplist, mapnum-1, mode);
			ExitLevel();
			return;
		}

		if (!strcmp(maplist->maps[mapnum-1].name, level.mapname))
		{
			safe_cprintf(ent, PRINT_HIGH, "Can't vote for current map!\n");
			return;
		}

		//Add the player's vote
		AddVote(ent, mode, mapnum-1);
	}
}

//************************************************************************************************

void ShowVoteMapMenu(edict_t *ent, int pagenum, int mapmode)
{
	int i;
	v_maplist_t *maplist = GetMapList(mapmode);
	qboolean EndOfList = false;
	
	if (!maplist) return;

	//Usual menu stuff
	 if (!ShowMenu(ent))
        return;
	clearmenu(ent);

	//Print header
	addlinetomenu(ent, "Vote for map:", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);

	for (i = 10 * (pagenum-1); i < 10 * pagenum; ++i)
	{
		char buf[20];
		if (i >= maplist->nummaps)
		{
			EndOfList = true;
			break;
		}
		strcpy(buf, maplist->maps[i].name);

		char min_max_str[20] = {0};
		if (maplist->maps[i].min_players > 1 && maplist->maps[i].max_players == 0)
			Com_sprintf(min_max_str, 20,"Min. %2d", maplist->maps[i].min_players);
		if (maplist->maps[i].min_players > 1 && maplist->maps[i].max_players > 0)
			Com_sprintf(min_max_str, 20, "%d to %d", maplist->maps[i].min_players, maplist->maps[i].max_players);
		if (maplist->maps[i].min_players <= 1 && maplist->maps[i].max_players > 0)
			Com_sprintf(min_max_str,  20, "Max. %2d", maplist->maps[i].max_players);


		addlinetomenu(ent, va(" %-14.14s %s", buf, min_max_str), i+1+(mapmode * 1000));
	}

	//Menu footer
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, " ", 0);

	//Set current line
	if (i % 10 != 0) ent->client->menustorage.currentline = 6 + (i % 10);
	else ent->client->menustorage.currentline = 16;

	if ((!EndOfList) && (i != maplist->nummaps))
	{
		addlinetomenu(ent, " Next", (20000 + pagenum + 1) + (mapmode * 1000)); //ex: pvp, page 3 = 11004
	}
	else
	{
		addlinetomenu(ent, " ", 0);
		++ent->client->menustorage.currentline;
	}
	
	addlinetomenu(ent, " Previous", (20000 + pagenum - 1) + (mapmode * 1000));	//ex: pvm, page 3 = 12002
	addlinetomenu(ent, " Exit", 66666);

	//Set handler
	setmenuhandler(ent, ShowVoteMapMenu_handler);

	//Display the menu
	showmenu(ent);
}

//************************************************************************************************
//		**VOTE MODE SELECT MENU**
//************************************************************************************************

void ShowVoteModeMenu_handler(edict_t *ent, int option)
{
	if (option == 66666)
	{
		closemenu(ent);
		return;
	}

	//Select a mode, and pick a map
	ShowVoteMapMenu(ent, 1, option);	//page = 1, gamemode = option
}

//************************************************************************************************

void KillMyVote (edict_t *ent)
{
	if (ent->client->resp.HasVoted) // voted?
	{
		ent->client->resp.HasVoted = false;
		if (ent->client->resp.voteType == 1) // voted yes?
			numVotes--;
		else
			numVoteNo--; // voted no..
	}
}

qboolean ThereIsOneLevelTen()
{
	int i;
	for (i = 1; i <= maxclients->value; i++)
	{
		edict_t* cl = g_edicts+i;
		if (cl->client && !G_IsSpectator(cl) && cl->inuse)
			if (cl->myskills.level >= 10)
				return true;
	}
	return false;
}

void ShowVoteModeMenu(edict_t *ent)
{

	int	players, lastline=6, min_players;

	char *cmd2 = gi.argv(1);

	//Voting enabled?
	if (!voting->value)
		return;

	// don't allow non-admin voting during pre-game to allow players time to connect
	if (!ent->myskills.administrator && (level.time < 5.0)) // allow 5 seconds for players to connect
	{
		safe_cprintf(ent, PRINT_HIGH, "Please allow time for other players to connect.\n");
		return;
	}
	if (ent->client->resp.VoteTimeout > level.time)
		return;

	if (cmd2[0] == 'y' || cmd2[0] == 'Y')
	{
		if (!currentVote.mode)
			safe_cprintf(ent, PRINT_HIGH, "There is no vote taking place at this time.\n");
		else
		{
			// Did we already vote?
			if (ent->client->resp.HasVoted)
			{
				safe_cprintf(ent, PRINT_HIGH, "You already placed your vote.\n");
				return; // GTFO PLZ
			}
			ent->client->resp.HasVoted = true;
			numVotes++;
			
			G_PrintGreenText(va("%s voted Yes.\n", ent->client->pers.netname));
		}
		return;
	}
	else if (cmd2[0] == 'n' || cmd2[0] == 'N')
	{
		if (!currentVote.mode)
			safe_cprintf(ent, PRINT_HIGH, "There is no vote taking place at this time.\n");
		else
		{
			// Did we already vote?
			if (ent->client->resp.HasVoted)
			{
				safe_cprintf(ent, PRINT_HIGH, "You already placed your vote.\n");
				return; // GTFO PLZ
			}
			numVoteNo++;
			ent->client->resp.HasVoted = true;
			gi.bprintf (PRINT_CHAT, "%s voted No.\n", ent->client->pers.netname);
		}
		return;
	}
	// Bring open the menu
	else if (currentVote.mode)
	{
		// TO DO
		return;
	}

	//Usual menu stuff
	 if (!ShowMenu(ent))
        return;
	clearmenu(ent);
	
	//Print header
	addlinetomenu(ent, "Vote for game mode:", MENU_GREEN_CENTERED);
	addlinetomenu(ent, " ", 0);

//GHz START
	players = vrx_get_joined_players();
	// pvm and invasion are only available when there are few players on the server
#ifdef FORCE_PVP_WITH_A_LOT_OF_PLAYERS
	if (0.33 * maxclients->value < 4)
		min_players = 4;
	else
		min_players = 0.25 * maxclients->value;
#else
		min_players = 6;
#endif

	if (players > 1) // how could you ever play pvp alone? -az (bots not functional yet)
		addlinetomenu(ent, " Player vs. Player", MAPMODE_PVP);

	addlinetomenu(ent, " Free For All", MAPMODE_FFA);

	if (players < min_players)
	{
		addlinetomenu(ent, " Player vs. Monster", MAPMODE_PVM);
		lastline++;
	}

		// vrx chile 2.5: trading mode
		if (tradingmode_enabled->value)
		{
			addlinetomenu(ent, " Trading", MAPMODE_TRA);
			lastline++;
		}

	// invasion mode
	if (players < min_players)
	{
		if (invasion_enabled->value)
		{
			addlinetomenu(ent, " Invasion", MAPMODE_INV);
			lastline++;
		}
	}

	if (invasion_enabled->value && (ThereIsOneLevelTen() || vrx_get_alive_players() >= min_players) )
	{
		addlinetomenu(ent, " Invasion (Hard mode)", MAPMODE_INH);
		lastline++;
	}
	
	// domination available when there are at least 4 players
	if (players >= 8)
	{
		addlinetomenu(ent, " Domination", MAPMODE_DOM);
		lastline++;
	}
	// CTF and vhw and tbi available when there are at least 4 players
	if (players >= 4)
	{
		addlinetomenu(ent, " CTF", MAPMODE_CTF);
		lastline++;

		if (gi.cvar("vhw_enabled", "1", 0)->value)
		{
			addlinetomenu(ent, " Vortex Holywars", MAPMODE_VHW);
			lastline++;
		}

		// az: nobody plays DTS.
		/*addlinetomenu(ent, " Destroy The Spawn", MAPMODE_TBI);
		lastline++;*/
	}

//GHz END
	//Menu footer
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, " ", 0);
	addlinetomenu(ent, " Exit", 66666);

	//Set handler
	setmenuhandler(ent, ShowVoteModeMenu_handler);

	//Set current line
	ent->client->menustorage.currentline = lastline;

	//Display the menu
	showmenu(ent);
}

