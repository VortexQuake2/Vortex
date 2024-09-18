#include "g_local.h"
#include "../gamemodes/ctf.h"
#include "characters/io/v_characterio.h"

#define VOTE_MAP	1
#define	VOTE_MODE	2

qboolean loc_CanSee (edict_t *targ, edict_t *inflictor);

void vrx_check_for_levelup(edict_t *ent, qboolean print_message);
void Cmd_Armory_f(edict_t*ent, int selection);
//Function prototypes required for this .c file:
void OpenDOMJoinMenu (edict_t *ent);

void ChaseCam(edict_t *ent)
{
	if (ent->client->menustorage.menu_active)
		menu_close(ent, true);

	if (ent->client->chase_target) 
	{
		DisableChaseCam(ent);
		return;
	}

	GetChaseTarget(ent);
}

int ClassNum(edict_t *ent, int team); //GHz
int HighestLevelPlayer(void); //GHz

void player_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(self, other, plane, surf);
}

void vrx_start_reign(edict_t *ent)
{
	int		i;
	gitem_t *item=itemlist;

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.spectator = false;
	ent->client->pers.spectator = false;
	ent->client->ps.stats[STAT_SPECTATOR] = 0;
	PutClientInServer (ent);

	average_player_level = AveragePlayerLevel();
	ent->health = ent->myskills.current_health;

//	if(savemethod->value == 1) // binary .vrx style saving
//		for (i=0; i<game.num_items; i++, item++) // reload inventory.
//			ent->client->pers.inventory[ITEM_INDEX(item)] = ent->myskills.inventory[ITEM_INDEX(item)];

	ent->client->pers.inventory[flag_index] = 0; // NO FLAG FOR YOU!!!
	ent->client->pers.inventory[red_flag_index] = 0;
	ent->client->pers.inventory[blue_flag_index] = 0;
	//4.2 remove techs
	ent->client->pers.inventory[resistance_index] = 0;
	ent->client->pers.inventory[strength_index] = 0;
	ent->client->pers.inventory[haste_index] = 0;
	ent->client->pers.inventory[regeneration_index] = 0;
	ent->touch = player_touch;
    vrx_update_all_character_maximums(ent);

    vrx_add_respawn_items(ent);
		
	// add a teleportation effect
	ent->s.event = EV_PLAYER_TELEPORT;
	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;

	if (ent->myskills.boss > 0)
	{
		gi.bprintf(PRINT_HIGH, "A level %d boss known as %s starts %s reign.\n", 
			ent->myskills.boss, ent->client->pers.netname, GetPossesiveAdjective(ent) );
	}
	else if (vrx_is_newbie_basher(ent) && !ptr->value && !domination->value)
	{
		gi.bprintf(PRINT_HIGH, "A level %d mini-boss known as %s begins %s domination.\n", 
		ent->myskills.level, ent->client->pers.netname, GetPossesiveAdjective(ent) );
		gi.bprintf(PRINT_HIGH, "Kill %s, and every non-boss gets a reward!\n", ent->client->pers.netname);
		safe_cprintf(ent, PRINT_HIGH, "You are currently a mini-boss. You will receive no point loss, but cannot war.\n");
	}
	else
	{
		gi.bprintf( PRINT_HIGH, "%s starts %s reign.\n", ent->client->pers.netname, GetPossesiveAdjective(ent) );
	}

	vrx_normalize_abilities(ent);
    V_UpdatePlayerTalents(ent);

	//Set the player's name
	strcpy(ent->myskills.player_name, ent->client->pers.netname);

	if (level.time < pregame_time->value && !trading->value) {
		safe_centerprintf(ent, "This map is currently in pre-game\nPlease warm up, upgrade and\naccess the Armory now\n");
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}

	if (trading->value) // Red shell for trading mode.
	{
		safe_centerprintf(ent, "Welcome to trading mode\nBuy stuff and trade runes freely.\nVote for another mode to start playing.\n");
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}
    vrx_write_to_logfile(ent, "Logged in.\n");

#ifndef GDS_NOMULTITHREADING
	ent->gds_thread_status = 0;
#endif

    vrx_commit_character(ent, false); // Do we need to?
    vrx_assign_character_skin(ent, Info_ValueForKey(ent->client->pers.userinfo, "skin"));
    G_StuffPlayerCmds(ent, va("exec %s.cfg", ent->client->pers.netname));
}

qboolean CanJoinGame(edict_t *ent, int returned)
{
	switch(returned)
	{
	case -1:	//bad password
		safe_cprintf(ent, PRINT_HIGH, "Access denied. Incorrect password.\n");
		return false;
	case -2:	//below minimum level
		safe_cprintf(ent, PRINT_HIGH, "You have to be at least level %d to play on this server.\n", ((int)min_level->value));
		return false;
	case -3:	//above maximum level
		safe_cprintf(ent, PRINT_HIGH, "You have to be level %d or below to play here.\n", ((int)max_level->value));
		if (strcmp(reconnect_ip->string, "0") != 0)
		{
			safe_cprintf(ent, PRINT_HIGH, "You are being sent to an alternate server where you can play.\n");
			stuffcmd(ent, va("connect %s\n", reconnect_ip->string));
		}
		return false;
	case -4:	//invalid player name
		safe_cprintf(ent, PRINT_HIGH, "Your name must be greater than 2 characters long.\n");
		return false;
	case -5:	//playing too much
		safe_cprintf(ent, PRINT_HIGH, "Can't join: %d hour play-time limit reached.\n", MAX_HOURS);
		safe_cprintf(ent, PRINT_HIGH, "Please try a different character, or try again tommorow.\n");
		return false;
/*	case -6:	//newbie basher can't play
		safe_cprintf(ent, PRINT_HIGH, "Unable to join: The current maximum level is %d.\n", NEWBIE_BASHER_MAX);
		safe_cprintf(ent, PRINT_HIGH, "Please return at a later time, or try a different character.\n");
		gi.dprintf("INFO: %s exceeds maximum level allowed by server (level %d)!", ent->client->pers.netname, NEWBIE_BASHER_MAX);
		return false;
*/
	case -7:	//boss can't play
		safe_cprintf(ent, PRINT_HIGH, "Unable to join: Bosses are not allowed unless the server is at least half capacity.\n");
		safe_cprintf(ent, PRINT_HIGH, "Please come back at a later time, or try a different character.\n");
		return false;
	case -8: // stupid name
		safe_cprintf(ent, PRINT_HIGH, "Unable to join with default name \"Player\" - Don't use this name. We won't be able to know who are you.\n");
		return false;
	default:	//passed every check
		return true;
	}
}

void OpenModeMenu(edict_t *ent)
{
	if (ptr->value)
	{
		OpenPTRJoinMenu(ent);
		return;
	}

	if (domination->value)
	{
		OpenDOMJoinMenu(ent);
		return;
	}

	if (ctf->value)
	{
		CTF_OpenJoinMenu(ent);
		return;
	}

	if (ent->myskills.class_num == 0)
	{
		OpenClassMenu(ent, 1); //GHz
		return;
	}

    vrx_start_reign(ent);
}

void JoinTheGame (edict_t *ent)
{
	if (ent->client->menustorage.menu_active)
	{
		menu_close(ent, true);
		return;
	}

    if (vrx_char_io.is_loading(ent))
        return;

    if (vrx_char_io.handle_status) {
        /*
         * We're in multithreaded mode, so we can't go into the mode menu right away.
         * this is kind of silly, sorry, but
         * because vrx_load_player will very likely always
         * be true in this circumstance where
         * we're using multithreading, since it only
         * validates that it's been successfully added to the
         * queue, we mustn't do the rest of the procedure.
         * handle_status will take care of it.
         */
        vrx_load_player(ent);
    } else {
        if (vrx_load_player(ent) == false)
            vrx_create_new_character(ent);

        if (!CanJoinGame(ent, vrx_get_login_status(ent)))
            return;

        OpenModeMenu(ent); // This implies the game will start.
    }
}

qboolean StartClient(edict_t *ent)
{
	if (ent->client->pers.spectator && !level.intermissiontime) 
	{
		OpenJoinMenu(ent);
		return true;
	}
	return false;
}

void motdmenu_handler (edict_t *ent, int option)
{
    switch (option)
    {
        case 1:
            JoinTheGame(ent);
            break;
        case 2:
            OpenJoinMenu(ent);
    }
}

void OpenMOTDMenu (edict_t *ent)
{
    if (!menu_can_show(ent))
        return;
    menu_clear(ent);

    //				    xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)

    menu_add_line(ent, "Message of the Day", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);    
    menu_add_line(ent, "Join us on Discord!", MENU_WHITE_CENTERED);
    menu_add_line(ent, "discord.gg/bX7Updq", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, "Rules", MENU_GREEN_CENTERED);
    menu_add_line(ent, "- No racism, sexism, or", 0);
    menu_add_line(ent, "targeted harassment.", MENU_WHITE_CENTERED);
    menu_add_line(ent, "- Do not exploit bugs.", 0);
    menu_add_line(ent, "- Be kind when discussing", 0);
    menu_add_line(ent, "balancing issues.", MENU_WHITE_CENTERED);
    menu_add_line(ent, "- Don't be an asshole.", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, " ", 0);
    menu_add_line(ent, "Accept", 1);
    menu_add_line(ent, "Back", 2);

    menu_set_handler(ent, motdmenu_handler);
    ent->client->menustorage.currentline = 15;
    menu_show(ent);
}

void joinmenu_handler (edict_t *ent, int option)
{
	switch (option)
	{
    case 1:
        OpenMOTDMenu(ent); break;
	case 2: 
		ChaseCam(ent); break;
	case 3: 
		menu_close(ent, true); break;
	}
}

void OpenJoinMenu (edict_t *ent)
{
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//				    xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)

	menu_add_line(ent, "Vortex Revival", MENU_GREEN_CENTERED);
	menu_add_line(ent, va("vrxcl v%s", VRX_VERSION), MENU_GREEN_CENTERED);
	menu_add_line(ent, "http://q2vortex.com", MENU_WHITE_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Kill players and monsters", 0);
    menu_add_line(ent, "for EXP to earn levels.", 0);
    menu_add_line(ent, "Every level you receive", 0);
    menu_add_line(ent, "ability and weapon points", 0);
    menu_add_line(ent, "to become stronger!", 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Maintained by", MENU_GREEN_CENTERED);
	menu_add_line(ent, "The Vortex Revival Team", MENU_GREEN_CENTERED);
	menu_add_line(ent, "github.com/zardoru/vrxcl", MENU_WHITE_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Start your reign", 1);
	menu_add_line(ent, "Toggle chasecam", 2);
	menu_add_line(ent, "Exit", 3);

	menu_set_handler(ent, joinmenu_handler);
	ent->client->menustorage.currentline = 16;
	menu_show(ent);
}

void myinfo_handler (edict_t *ent, int option)
{
	menu_close(ent, true);
}

void OpenMyinfoMenu (edict_t *ent)
{
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//				    xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
    menu_add_line(ent, va("%s (%s)", ent->client->pers.netname,
                          vrx_get_class_string(ent->myskills.class_num)), MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, va("Level:        %d", ent->myskills.level), 0);
	menu_add_line(ent, va("Experience:   %d", ent->myskills.experience), 0);
	menu_add_line(ent, va("Next Level:   %d", (ent->myskills.next_level-ent->myskills.experience)+ent->myskills.nerfme), 0);
	menu_add_line(ent, va("Credits:      %d", ent->myskills.credits), 0);
	if (ent->myskills.shots > 0)
		menu_add_line(ent, va("Hit Percent:  %d%c", (int)(100*((float)ent->myskills.shots_hit/ent->myskills.shots)), '%'), 0);
	else
		menu_add_line(ent, "Hit Percent:  --", 0);
	menu_add_line(ent, va("Frags:        %d", ent->myskills.frags), 0);
	menu_add_line(ent, va("Fragged:      %d", ent->myskills.fragged), 0);
	if (ent->myskills.fragged > 0)
		menu_add_line(ent, va("Frag Percent: %d%c", (int)(100*((float)ent->myskills.frags/ent->myskills.fragged)), '%'), 0);
	else
		menu_add_line(ent, "Frag Percent: --", 0);
	menu_add_line(ent, va("Played Hrs:   %.1f", (float)ent->myskills.playingtime/3600), 0);
#ifndef REMOVE_RESPAWNS
	menu_add_line(ent, va("Respawns: %d", ent->myskills.weapon_respawns), 0);
#endif
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Exit", 1);

	menu_set_handler(ent, myinfo_handler);
		ent->client->menustorage.currentline = 13;
	menu_show(ent);
}

void respawnmenu_handler (edict_t *ent, int option)
{
	if (option == 99)
	{
		menu_close(ent, true);
		return;
	}

	ent->myskills.respawn_weapon = option;
}

char *GetRespawnString (edict_t *ent)
{
	switch (ent->myskills.respawn_weapon)
	{
	case 1: return "Sword";
	case 2: return "Shotgun";
	case 3: return "Super Shotgun";
	case 4: return "Machinegun";
	case 5: return "Chaingun";
	case 6: return "Grenade Launcher";
	case 7: return "Rocket Launcher";
	case 8: return "Hyperblaster";
	case 9: return "Railgun";
	case 10: return "BFG10k";
	case 11: return "Hand Grenades";
	case 12: return "20mm Cannon";
	case 13: return "Blaster";
	default: return "Unknown";
	}
}

void OpenRespawnWeapMenu(edict_t *ent)
{
    if (!menu_can_show(ent))
        return;

    if (ent->myskills.class_num == CLASS_KNIGHT) {
        safe_cprintf(ent, PRINT_HIGH, "Knights are restricted to a sabre.\n");
        return;
    }

    if (vrx_is_morphing_polt(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "You can't pick a respawn weapon.\n");
        return;
    }

	menu_clear(ent);

	//				xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Pick a respawn weapon.", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Sword", 1);
	menu_add_line(ent, "Shotgun", 2);
	menu_add_line(ent, "Super Shotgun", 3);
	menu_add_line(ent, "Machinegun", 4);
	menu_add_line(ent, "Chaingun", 5);
	menu_add_line(ent, "Hand Grenades", 11);
	menu_add_line(ent, "Grenade Launcher", 6);
	menu_add_line(ent, "Rocket Launcher", 7);
	menu_add_line(ent, "Hyperblaster", 8);
	menu_add_line(ent, "Railgun", 9);
	menu_add_line(ent, "BFG10k", 10);
	menu_add_line(ent, "20mm Cannon", 12);
	menu_add_line(ent, "Blaster", 13);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, va("Respawn: %s", GetRespawnString(ent)), 0);
	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Exit", 99);

	menu_set_handler(ent, respawnmenu_handler);
		ent->client->menustorage.currentline = 3;
	menu_show(ent);
}

void classmenu_handler (edict_t *ent, int option)
{
	int page_num = (option / 1000);
	int page_choice = (option % 1000);
	int i;

	if ((page_num == 1) && (page_choice == 1))
	{
		OpenJoinMenu(ent);
		return;
	}
	else if (page_num > 0) {
        if (page_choice == 2)    //next
            OpenClassMenu(ent, page_num + 1);
        else if (page_choice == 1)    //prev
            OpenClassMenu(ent, page_num - 1);
        return;
    }
        //don't cause an invalid class to be created (option 99 is checked as well)
    else if ((option >= CLASS_MAX) || (option < 1)) {
        menu_close(ent, true);
        return;
    }

	// assign new players a team if the start level is low
	// otherwise, deny them access until the next game
	if (ctf->value && (level.time > pregame_time->value))
	{
		if (start_level->value > 1)
		{
			safe_cprintf(ent, PRINT_HIGH, "Can't join in the middle of a CTF game.\n");
			return;
		}

		// assign a random team
		ent->teamnum = GetRandom(1, 2);
	}

    ent->myskills.experience = 0;
	for (i = 0; i < start_level->value; ++i)
	{
        ent->myskills.experience += vrx_get_points_tnl(i);
	}

	ent->myskills.class_num = option;
    vrx_assign_abilities(ent);
    vrx_set_talents(ent);
	vrx_prestige_init(ent);
	ent->myskills.weapon_respawns = 100;

    gi.dprintf("INFO: %s created a new %s!\n",
               ent->client->pers.netname,
               vrx_get_class_string(ent->myskills.class_num));

    vrx_write_to_logfile(ent,
                         va("%s created a %s.\n",
                            ent->client->pers.netname,
                            vrx_get_class_string(ent->myskills.class_num)));

    vrx_check_for_levelup(ent, false);
    vrx_update_all_character_maximums(ent);
    vrx_add_respawn_weapon(ent, ent->myskills.respawn_weapon);
    vrx_add_respawn_items(ent);

    vrx_reset_weapon_maximums(ent);

	// FIXME: we should do a better job of selecting a team OR block them from joining (temp make non-spec to make savechar() happy)
	// we need to select a team if we're past pre-game time in CTF or Domination modes
	if ((ctf->value || domination->value) && (level.time > pregame_time->value))
		ent->teamnum = GetRandom(1, 2);

    vrx_start_reign(ent);
    vrx_save_character(ent, false);
}

void OpenClassMenu (edict_t *ent, int page_num)
{
	int i;

	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	//				xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Please select your", MENU_GREEN_CENTERED);
	menu_add_line(ent, "character class:", MENU_GREEN_CENTERED);
    menu_add_line(ent, " ", 0);

    for (i = ((page_num - 1) * 11); i < (page_num * 11); ++i) {
        if (i < CLASS_MAX - 1)
            menu_add_line(ent, va("%s", vrx_get_class_string(i + 1)), i + 1);
        else menu_add_line(ent, " ", 0);
    }

    menu_add_line(ent, " ", 0);
    if (i < CLASS_MAX - 1) menu_add_line(ent, "Next", (page_num * 1000) + 2);
    menu_add_line(ent, "Back", (page_num * 1000) + 1);
    menu_add_line(ent, "Exit", 99);

    menu_set_handler(ent, classmenu_handler);
    if (CLASS_MAX - 1 > 11)
        ent->client->menustorage.currentline = 15;
    else ent->client->menustorage.currentline = 5 + i;
    menu_show(ent);
}

void masterpw_handler (edict_t *ent, int option)
{
	menu_close(ent, true);
}

void OpenMasterPasswordMenu (edict_t *ent)
{
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	menu_add_line(ent, "Master Password", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);

	if (strcmp(ent->myskills.email, ""))
	{
		menu_add_line(ent, "A master password has", 0);
		menu_add_line(ent, "already been set and can't", 0);
		menu_add_line(ent, "be changed!", 0);
		menu_add_line(ent, " ", 0);
		menu_add_line(ent, " ", 0);
		menu_add_line(ent, " ", 0);
	}
	else
	{
		// open prompt for password
		stuffcmd(ent, "messagemode\n");

		menu_add_line(ent, "Please enter a master", 0);
		menu_add_line(ent, "password above. If you", 0);
		menu_add_line(ent, "forget your normal", 0);
		menu_add_line(ent, "password, you can use", 0);
		menu_add_line(ent, "this one to recover your", 0);
		menu_add_line(ent, "character.", 0);
	}

	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Close", 1);

	menu_set_handler(ent, masterpw_handler);
	ent->client->menustorage.currentline = 10;
	ent->client->menustorage.menu_index = MENU_MASTER_PASSWORD;
	menu_show(ent);
}

void generalmenu_handler (edict_t *ent, int option)
{
	switch (option)
	{
	case 1: OpenUpgradeMenu(ent); break;
	case 2: OpenWeaponUpgradeMenu(ent, 0); break;
	case 3: OpenTalentUpgradeMenu(ent, 0); break;
	case 4: OpenRespawnWeapMenu(ent); break;
	case 5: OpenMasterPasswordMenu(ent); break;
	case 6: OpenMyinfoMenu(ent); break;
	case 7: OpenArmoryMenu(ent); break;
	case 8: ShowInventoryMenu(ent, 0, false); break;
	case 9: ShowAllyMenu(ent); break;
	case 10: ShowTradeMenu(ent); break;
	case 11: ShowVoteModeMenu(ent); break;
	case 12: ShowHelpMenu(ent, 0); break;
	case 13: Cmd_Armory_f(ent, 31); break;
	case 20: vrx_prestige_open_menu(ent); break;
	default: menu_close(ent, true);
	}
}

void OpenGeneralMenu (edict_t *ent)
{
	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

		//				xxxxxxxxxxxxxxxxxxxxxxxxxxx (max length 27 chars)
	menu_add_line(ent, "Welcome to Vortex!", MENU_GREEN_CENTERED);
	menu_add_line(ent, "Please choose a sub-menu.", MENU_GREEN_CENTERED);
	menu_add_line(ent, " ", 0);
	
	if (!ent->myskills.speciality_points)
		menu_add_line(ent, "Upgrade abilities", 1);
	else
		menu_add_line(ent, va("Upgrade abilities (%d)", ent->myskills.speciality_points), 1);
	
	if (!vrx_is_morphing_polt(ent))
	{
		if (!ent->myskills.weapon_points)
			menu_add_line(ent, "Upgrade weapons", 2);
		else
			menu_add_line(ent, va("Upgrade weapons (%d)", ent->myskills.weapon_points), 2);
	}
	if (!ent->myskills.talents.talentPoints)
		menu_add_line(ent, "Upgrade talents", 3);
	else
        menu_add_line(ent, va("Upgrade talents (%d)", ent->myskills.talents.talentPoints), 3);

	// az: only supported on sqlite. i'm lazy.
	if (vrx_char_io.type == SAVEMETHOD_SQLITE) {
		int prestigePotential = vrx_prestige_get_upgrade_points(ent->myskills.experience);
		if (prestigePotential)
			menu_add_line(ent, va("Prestige %d (%d)", ent->myskills.prestige.total, prestigePotential), 20);
		else
			menu_add_line(ent, va("Prestige %d", ent->myskills.prestige.total), 20);
	}

    menu_add_line(ent, " ", 0);
    if (!vrx_is_morphing_polt(ent) &&
        ent->myskills.class_num != CLASS_KNIGHT)
        menu_add_line(ent, "Set respawn weapon", 4);

	if (ent->myskills.email[0] == '\0')
		menu_add_line(ent, "Set master password", 5);


    menu_add_line(ent, "Show character info", 6);
    menu_add_line(ent, "Access the armory", 7);
    menu_add_line(ent, "Access your items", 8);

    if (!invasion->value) // az: don't need this there.
        menu_add_line(ent, "Form alliance", 9);

    menu_add_line(ent, "Trade items", 10);
    menu_add_line(ent, "Vote for map/mode", 11);
    menu_add_line(ent, "Help", 12);

#ifndef REMOVE_RESPAWNS
    if (pregame_time->value > level.time || trading->value) // we in pregame? you can buy respawns
    {
        if (ent->myskills.class_num != CLASS_KNIGHT && !vrx_is_morphing_polt(ent)) // A class that needs respawns?
            menu_add_line(ent, va("Buy Respawns (%d)", ent->myskills.weapon_respawns), 13);
    }
#endif

	menu_set_handler(ent, generalmenu_handler);
	ent->client->menustorage.currentline = 4;
	menu_show(ent);
}

char *G_GetTruncatedIP (edict_t *player);

char *GetTeamString (edict_t *ent)
{
	if (ent->teamnum == 1)
		return "Red";
	if (ent->teamnum == 2)
		return "Blue";
    if (V_GetNumAllies(ent) > 0)
		return "Allied";
	return "None";
}

char *GetStatusString (edict_t *ent)
{
	if (ent->health < 1)
		return "Dead";
	if (ent->flags & FL_CHATPROTECT)
		return "Chat Protect";
	if (ent->client->idle_frames > qf2sf(300))
		return "Idle";
	return "Normal";
}
	
void OpenWhoisMenu (edict_t *ent)
{
	edict_t *player;

	if (!menu_can_show(ent))
        return;
	menu_clear(ent);

	if ((player = FindPlayerByName(gi.argv(1))) == NULL)
	{
		safe_cprintf(ent, PRINT_HIGH, "Couldn't find player \"%s\".\n", gi.argv(1));
		return;
	}

	if (G_IsSpectator(player))
		return;

	menu_add_line(ent, "Whois Information", MENU_GREEN_CENTERED);
	menu_add_line(ent, "", 0);

	// print name and class
    menu_add_line(ent, va("%s (%s)", player->client->pers.netname,
                          vrx_get_class_string(player->myskills.class_num)), MENU_GREEN_CENTERED);

	// print IP address
	if (ent->myskills.administrator)
		menu_add_line(ent, player->client->pers.current_ip, MENU_GREEN_CENTERED);
	else
		menu_add_line(ent, G_GetTruncatedIP(player), MENU_GREEN_CENTERED);

	menu_add_line(ent, "", 0);

	menu_add_line(ent, va("Admin:        %s", player->myskills.administrator?"Yes":"No"), 0);
	menu_add_line(ent, va("Owner:        %s", player->myskills.owner), 0);
	menu_add_line(ent, va("Status:       %s", GetStatusString(player)), 0);
	menu_add_line(ent, va("Team:         %s", GetTeamString(player)), 0);
	menu_add_line(ent, va("Level:        %d", player->myskills.level), 0);
	menu_add_line(ent, va("Experience:   %d", player->myskills.experience), 0);
	menu_add_line(ent, va("Hit Percent:  %d%c", (int)(100*((float)player->myskills.shots_hit/player->myskills.shots)), '%'), 0);
	menu_add_line(ent, va("Frag Percent: %d%c", (int)(100*((float)player->myskills.frags/player->myskills.fragged)), '%'), 0);
	menu_add_line(ent, va("Played Hours: %.1f", (float)player->myskills.playingtime/3600), 0);


	menu_add_line(ent, " ", 0);
	menu_add_line(ent, "Exit", 1);

	menu_set_handler(ent, masterpw_handler);//FIXME: is this safe?
		ent->client->menustorage.currentline = 16;
	menu_show(ent);
}
