#include "g_local.h"

void vrx_change_map(v_maplist_t *maplist, int mapindex, int gamemode) {
    //char buf[8];
    int timelimit;
    int fraglimit;

    vrx_lua_event("on_change_map");

    //Change the mode
    switch (gamemode) {
        case MAPMODE_PVP: {
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
        case MAPMODE_PVM: {
            // player versus monsters
            timelimit = vrx_lua_get_variable("pvm_timelimit", 10) + pregame_time->value / 60;
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
        case MAPMODE_DOM: {
            timelimit = vrx_lua_get_variable("dom_timelimit", 10) + pregame_time->value / 60;
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
        case MAPMODE_CTF: {
            timelimit = vrx_lua_get_variable("ctf_timelimit", 10) + pregame_time->value / 60;
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
        case MAPMODE_FFA: {
            timelimit = vrx_lua_get_variable("ffa_timelimit", 15) + pregame_time->value / 60;
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
        case MAPMODE_INV: {
            timelimit = vrx_lua_get_variable("inv_timelimit", 20) + pregame_time->value / 60;
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
        case MAPMODE_INH: {
            timelimit = vrx_lua_get_variable("inh_timelimit", 25) + pregame_time->value / 60;
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
        case MAPMODE_INHE: {
            timelimit = vrx_lua_get_variable("inhe_timelimit", 60) + pregame_time->value / 60;
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
        }
        break;
        case MAPMODE_TRA: {
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
            timelimit = vrx_lua_get_variable("vhw_timelimit", 10) + pregame_time->value / 60;
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
        case MAPMODE_TBI: {
            timelimit = vrx_lua_get_variable("dts_timelimit", 15) + pregame_time->value / 60;
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
    vrx_vote_reset();

    gi.dprintf("Votes have been reset.\n");

    //Change the map
    if (mapindex < maplist->nummaps) // Assume everyone has proper maplists? Nope -az
    {
        strcpy(level.nextmap, maplist->maps[mapindex].name);
        level.r_monsters = maplist->maps[mapindex].monsters; //4.5
        gi.dprintf("Next map is: %s\n", level.nextmap);
    } else {
        gi.dprintf("Invalid map index (%d) defaulting to current map.\n", mapindex);
        strcpy(level.nextmap, level.mapname);
    }
    if (vrx_get_joined_players(false) == 0)
        ExitLevel();
}


//************************************************************************************************

v_maplist_t *vrx_get_map_list(int mode) {
    if (vrx_lua_get_int("UseLuaMaplists", 0))
        vrx_load_map_list(mode); // reload the map list (lua conditional maplists)

    //Change the map
    switch (mode) {
        case MAPMODE_PVP: return &maplist_PVP;
        case MAPMODE_DOM: return &maplist_DOM;
        case MAPMODE_PVM: return &maplist_PVM;
        case MAPMODE_CTF: return &maplist_CTF;
        case MAPMODE_FFA: return &maplist_FFA;
        case MAPMODE_INV: return &maplist_INV;
        case MAPMODE_TRA: return &maplist_TRA; // vrxchile 2.5: trading mode maplist

        // vrxchile 2.6: invasion hard mode
        case MAPMODE_INHE:
        case MAPMODE_INH: return &maplist_INH;

        case MAPMODE_VHW: return &maplist_VHW; // vrxchile 3.0: vortex holy wars mode
        case MAPMODE_TBI: return &maplist_TBI; // vrxchile 3.4: destroy the spawn mode
        default:
            gi.dprintf("ERROR in GetMapList(). Incorrect map mode. (%d)\n", mode);
            return 0;
    }
}

void vrx_map_vote_success(void *udata) {
    EndDMLevel();
}


void vrx_start_map_vote(edict_t *ent, int mode, int mapnum) {
    v_maplist_t *maplist = vrx_get_map_list(mode);

    if (!maplist)
        return;

    if (vrx_vote_is_in_progress())
        return;


    //check for valid choice
    auto currentVote = vrx_vote_get_current();
    const int players = vrx_get_joined_players(false);
    if (!mode || maplist->nummaps <= mapnum) {
        gi.dprintf(
            "Error in vrx_start_vote(): map number = %d, mode number = %d\n", mapnum, mode);
    }

    if (maplist->maps[mapnum].min_players > players) {
        gi.cprintf(ent, PRINT_HIGH, "The map '%s' requires at least %d players.\n", maplist->maps[mapnum].name,
                   maplist->maps[mapnum].min_players);
        return;
    }

    if (maplist->maps[mapnum].max_players < players && maplist->maps[mapnum].max_players > 0) {
        gi.cprintf(ent, PRINT_HIGH, "The map '%s' requires that there are at most %d players.\n",
                   maplist->maps[mapnum].name, maplist->maps[mapnum].max_players);
        return;
    }

    //Add the vote
    _vrx_start_vote(ent, VT_MAP, vrx_map_vote_success, nullptr);
    currentVote->mapvote.mapindex = mapnum;
    currentVote->mapvote.mode = mode;
    currentVote->mapvote.used = true;

    switch (mode) {
        case MAPMODE_PVP: currentVote->smode = "Player vs. Player (PvP) ";
            break;
        case MAPMODE_PVM: currentVote->smode = "Player vs. Monster (PvM) ";
            break;
        case MAPMODE_DOM: currentVote->smode = "Domination (DOM) ";
            break;
        case MAPMODE_CTF: currentVote->smode = "Capture The Flag (CTF) ";
            break;
        case MAPMODE_FFA: currentVote->smode = "Free For All (FFA) ";
            break;
        case MAPMODE_INV: currentVote->smode = "Invasion ";
            break;
        case MAPMODE_TRA: currentVote->smode = "Trading ";
            break;
        case MAPMODE_INH: currentVote->smode = "Invasion (Hard mode) ";
            break;
        case MAPMODE_INHE: currentVote->smode = "Invasion (Extended hard mode) ";
            break;
        case MAPMODE_VHW: currentVote->smode = "Vortex HolyWars ";
            break;
        case MAPMODE_TBI: currentVote->smode = "Destroy the Spawn ";
            break;
    }

    const auto text1 = HiPrint(va("%s", currentVote->smode));
    const auto text2 = HiPrint(va("%s", maplist->maps[mapnum].name));
    char strBuffer[1024];
    Com_sprintf(strBuffer, 1024, "%son %s\n", text1, text2);
    vrx_free(text1);
    vrx_free(text2);

    vrx_vote_announce(ent, strBuffer);
}

//************************************************************************************************

//Finds the index of the map with the most votes.
//Returns: -1 if there is no winning map (a tie)
int vrx_vote_find_best_map(int mode) {
    auto currentVote = vrx_vote_get_current();

    //if (numVotes > 0.5*ActivePlayers())
    //	return currentVote->mapindex;
    if (vrx_vote_is_done())
        return currentVote->mapvote.mapindex;
    return -1;
}


//************************************************************************************************


int vrx_vote_attempt_mode_change(qboolean endlevel) {
    auto currentVote = vrx_vote_get_current();
    assert(currentVote->votetype == VT_MAP);

#if FORCE_PVP_WITH_A_LOT_OF_PLAYERS
    int max_players, players = ActivePlayers();

    //4.4 forcibly switch to PvP if we are in Invasion/PvM and there are many players
    max_players = 0.25 * maxclients->value;
    if (max_players < 4)
        max_players = 4;
    if (players >= max_players && (pvm->value || invasion->value)
        && (currentVote->mode == MAPMODE_PVM || currentVote->mode == MAPMODE_INV)) {
        gi.bprintf(PRINT_HIGH, "Forcing switch to FFA because there are too many players!\n");
        return MAPMODE_FFA;
    }
#endif
    // did the vote pass?
    if (vrx_vote_is_done())
        return currentVote->mapvote.mode;

    // vote failed
    return 0;
}


//************************************************************************************************
//		**VOTE MAP SELECT MENU**
//************************************************************************************************
void ShowVoteMapMenu_handler(edict_t *ent, int option);
void vrx_vote_show_map_menu(edict_t *ent, int pagenum, int mapmode) {
    int i;
    v_maplist_t *maplist = vrx_get_map_list(mapmode);
    qboolean EndOfList = false;

    if (!maplist) return;

    //Usual menu stuff
    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    //Print header
    menu_add_line(ent, "Vote for map:", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    for (i = 10 * (pagenum - 1); i < 10 * pagenum; ++i) {
        char buf[20];
        if (i >= maplist->nummaps) {
            EndOfList = true;
            break;
        }
        strcpy(buf, maplist->maps[i].name);

        char min_max_str[20] = {0};
        if (maplist->maps[i].min_players > 1 && maplist->maps[i].max_players == 0)
            Com_sprintf(min_max_str, 20, "Min. %2d", maplist->maps[i].min_players);
        if (maplist->maps[i].min_players > 1 && maplist->maps[i].max_players > 0)
            Com_sprintf(min_max_str, 20, "%d to %d", maplist->maps[i].min_players, maplist->maps[i].max_players);
        if (maplist->maps[i].min_players <= 1 && maplist->maps[i].max_players > 0)
            Com_sprintf(min_max_str, 20, "Max. %2d", maplist->maps[i].max_players);


        menu_add_line(ent, va(" %-14.14s %s", buf, min_max_str), i + 1 + (mapmode * 1000));
    }

    //Menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);

    //Set current line
    if (i % 10 != 0) ent->client->menustorage.currentline = 6 + (i % 10);
    else ent->client->menustorage.currentline = 16;

    if ((!EndOfList) && (i != maplist->nummaps)) {
        menu_add_line(ent, " Next", (20000 + pagenum + 1) + (mapmode * 1000)); //ex: pvp, page 3 = 11004
    } else {
        menu_add_line(ent, " ", 0);
        ++ent->client->menustorage.currentline;
    }

    menu_add_line(ent, " Previous", (20000 + pagenum - 1) + (mapmode * 1000)); //ex: pvm, page 3 = 12002
    menu_add_line(ent, " Exit", 66666);

    //Set handler
    menu_set_handler(ent, ShowVoteMapMenu_handler);

    //Display the menu
    menu_show(ent);
}


void ShowVoteMapMenu_handler(edict_t *ent, int option) {
    if (option == 66666) {
        menu_close(ent, true);
        return;
    }
    //Multi-page navigation

    if (option > 20000) {
        const int mode = (option / 1000) - 20;
        const int nextpage = option % 1000; //page number we will end up in (page 0 = main menu)
        if (nextpage != 0)
            vrx_vote_show_map_menu(ent, nextpage, mode);
        else
            vrx_vote_map(ent);
    } else {
        const int mode = option / 1000;
        const int mapnum = option % 1000;
        v_maplist_t *maplist = vrx_get_map_list(mode);

        if (!maplist) return;

        //Admins directly control the map change
        if (ent->myskills.administrator && adminctrl->value) // IF the cvar is enabled.
        {
            VortexEndLevel();
            vrx_change_map(maplist, mapnum - 1, mode);
            ExitLevel();
            return;
        }

        if (!strcmp(maplist->maps[mapnum - 1].name, level.mapname)) {
            safe_cprintf(ent, PRINT_HIGH, "Can't vote for current map!\n");
            return;
        }

        //Add the player's vote
        vrx_start_map_vote(ent, mode, mapnum - 1);
    }
}

//************************************************************************************************
//		**VOTE MODE SELECT MENU**
//************************************************************************************************
void ShowVoteModeMenu_handler(edict_t *ent, int option) {
    if (option == 66666) {
        menu_close(ent, true);
        return;
    }

    //Select a mode, and pick a map
    vrx_vote_show_map_menu(ent, 1, option); //page = 1, gamemode = option
}

qboolean ThereIsOneLevelTen() {
    for (int i = 1; i <= maxclients->value; i++) {
        const edict_t *cl = g_edicts + i;
        if (cl->client && !G_IsSpectator(cl) && cl->inuse)
            if (cl->myskills.level >= 10)
                return true;
    }
    return false;
}

bool vrx_show_mapmode_vote_menu(edict_t *ent, int players, int min_players) {
    int lastline = 6;

    //Usual menu stuff
    if (!menu_can_show(ent))
        return true;
    menu_clear(ent);

    //Print header
    menu_add_line(ent, "Vote for game mode:", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    //GHz START
    players = vrx_get_joined_players(false);
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
        menu_add_line(ent, " Player vs. Player", MAPMODE_PVP);

    menu_add_line(ent, " Free For All", MAPMODE_FFA);

    if (players < min_players) {
        menu_add_line(ent, " Player vs. Monster", MAPMODE_PVM);
        lastline++;
    }

    // vrx chile 2.5: trading mode
    if (tradingmode_enabled->value) {
        menu_add_line(ent, " Trading", MAPMODE_TRA);
        lastline++;
    }

    // invasion mode
    if (players < min_players) {
        if (invasion_enabled->value) {
            menu_add_line(ent, " Invasion", MAPMODE_INV);
            lastline++;
        }
    }

    if (invasion_enabled->value && (ThereIsOneLevelTen() || vrx_get_alive_players() >= min_players)) {
        menu_add_line(ent, " Invasion (Hard)", MAPMODE_INH);
        menu_add_line(ent, " Invasion (Hard extended)", MAPMODE_INHE);
        lastline++;
    }

    // domination available when there are at least 4 players
    if (players >= 8) {
        menu_add_line(ent, " Domination", MAPMODE_DOM);
        lastline++;
    }
    // CTF and vhw and tbi available when there are at least 4 players
    if (players >= 4) {
        menu_add_line(ent, " CTF", MAPMODE_CTF);
        lastline++;

        if (gi.cvar("vhw_enabled", "1", 0)->value) {
            menu_add_line(ent, " Vortex Holywars", MAPMODE_VHW);
            lastline++;
        }

        // az: nobody plays DTS.
        /*menu_add_line(ent, " Destroy The Spawn", MAPMODE_TBI);
        lastline++;*/
    }

    //GHz END
    //Menu footer
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " Exit", 66666);

    //Set handler
    menu_set_handler(ent, ShowVoteModeMenu_handler);

    //Set current line
    ent->client->menustorage.currentline = lastline;

    //Display the menu
    menu_show(ent);
    return false;
}

void vrx_vote_map(edict_t *ent) {
    int players, min_players;

    if (!vrx_can_vote(ent))
        return;

    if (vrx_show_mapmode_vote_menu(ent, players, min_players)) return;
}
