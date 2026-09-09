#include "g_local.h"

#ifndef PLAYER_C
#define PLAYER_C

// returns true if the player should be affected by newbie protection
qboolean vrx_is_newbie_basher(const edict_t *player) {
	const qboolean levelAboveAverage = player->myskills.level > newbie_protection->value * AveragePlayerLevel();
	const qboolean isHighLevel = player->myskills.level >= 8;
	const qboolean isNotPVM = !(pvm->value != 0 || invasion->value != 0);
	const qboolean situationRequestsProtection = newbie_protection->value && player->client && (vrx_get_joined_players(true) > 1);
	const qboolean levelQualifies = isHighLevel && levelAboveAverage;

	return (situationRequestsProtection && isNotPVM && levelQualifies);
}

qboolean vrx_is_playing_too_much(edict_t *ent)
{
	//Char played today?
	if( Q_strncasecmp(ent->client->resp.pstats.last_played, CURRENT_DATE, strlen(CURRENT_DATE)) == 0)
	{
		//Has char been playing too long?
		if(ent->client->resp.pstats.playingtime > (MAX_HOURS * 3600) )	//Playing time in seconds?
			return true;
	}
	else
	{
		//Reset playing time for today
		ent->client->resp.pstats.total_playtime += (ent->client->resp.pstats.playingtime / 60);
		strcpy(ent->client->resp.pstats.last_played, CURRENT_DATE);
		ent->client->resp.pstats.playingtime = 0;
	}

	return false;
}


void vrx_create_new_character(edict_t *ent)
{
	memset(&ent->myskills, 0, sizeof(ent->myskills));
	ent->myskills.next_level = vrx_get_points_tnl(ent->myskills.level);
	ent->myskills.respawn_weapon = 7;

	Q_strncpy (ent->client->resp.pstats.password,
               vrx_encrypt_string(Info_ValueForKey(ent->client->pers.userinfo, "vrx_password"), false), sizeof(ent->client->resp.pstats.password) - 1);

	strcpy(ent->client->resp.pstats.member_since, va("%s at %s", CURRENT_DATE, CURRENT_TIME));
}

void vrx_initialize_player_class(edict_t *ent, int option) {
	vrx_create_new_character(ent);
	ent->myskills.experience = 0;
	for (int i = 0; i < start_level->value; ++i)
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
}


//Returns true if the player is able to join the game.
int vrx_get_login_status(edict_t *ent)
{
	char chkpassword[24];
	char chkpassword2[24];
	qboolean masterPasswordMatch=false;

	//password check
	Q_strncpy (chkpassword, Info_ValueForKey (ent->client->pers.userinfo, "vrx_password"), sizeof(chkpassword)-1);

	//strcpy(chkpassword2, CryptPassword(ent->myskills.password) );
	Q_strncpy (chkpassword2, vrx_encrypt_string(ent->client->resp.pstats.password, true), sizeof(chkpassword2) - 1);

	// check if userinfo password matches master password
	if (strlen(ent->client->resp.pstats.masterpw) > 0
		&& !Q_stricmp(chkpassword, ent->client->resp.pstats.masterpw))
		masterPasswordMatch = true;

	// compare normal and master password against userinfo value
	if (Q_stricmp(chkpassword, chkpassword2) && !masterPasswordMatch)
	{
		//gi.dprintf("'%s' '%s'\n", chkpassword, chkpassword2);
		return -1;	//bad password
	}

	//Minimum level requirements
	if (ent->myskills.level < min_level->value)
		return -2;	//below minimum level

	if (ent->myskills.level > max_level->value)
		return -3;	//above maximum level

	if (strlen(ent->client->pers.netname) < 3)
		return -4;	//invalid player name
	
	if (vrx_is_playing_too_much(ent))
		return -5;	//playing too much
	
	
	if (!invasion->value || !pvm->value || (ffa->value && vrx_get_joined_players(false) > 1))
		if (newbie_protection->value && vrx_is_newbie_basher(ent))
			return -6;	//newbie basher can't play on non pvm modes
	
	
	if (ent->myskills.boss && vrx_get_joined_players(false) < (0.5*maxclients->value)
        && !trading->value && (!pvm->value || !invasion->value) && vrx_is_newbie_basher(ent)) // trading, pvm or invasion modes means the boss actually can play.
		return -7; //boss can't play

	if (!strcmp(ent->client->resp.pstats.player_name, "Player"))
		return -8; // lol
	
	return 0;	//success
}

void fixInvalidPlayerData(edict_t *ent)
{
	int i;
	gitem_t *item=itemlist;

	for (i=0; i<game.num_items; i++, item++)
		ent->client->pers.inventory[ITEM_INDEX(item)] = ent->client->resp.pstats.inventory[ITEM_INDEX(item)];
	ent->client->pers.inventory[flag_index] = 0;

	vrx_update_all_character_maximums(ent);

	if (ent->myskills.credits < 0)
		ent->myskills.credits = 0;
}

#endif