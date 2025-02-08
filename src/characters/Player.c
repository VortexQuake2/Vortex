#include "g_local.h"

#ifndef PLAYER_C
#define PLAYER_C

// returns true if the player should be affected by newbie protection
qboolean vrx_is_newbie_basher(const edict_t *player) {
	qboolean levelAboveAverage = player->myskills.level > newbie_protection->value * AveragePlayerLevel();
	qboolean isHighLevel = player->myskills.level >= 8;
	qboolean isNotPVM = !(pvm->value != 0 || invasion->value != 0);
	qboolean situationRequestsProtection = newbie_protection->value && player->client && (vrx_get_joined_players(true) > 1);
	qboolean levelQualifies = isHighLevel && levelAboveAverage;

	return (situationRequestsProtection && isNotPVM && levelQualifies);
}

qboolean vrx_is_playing_too_much(edict_t *ent)
{
	//Char played today?
	if( Q_strncasecmp(ent->myskills.last_played, CURRENT_DATE, strlen(CURRENT_DATE)) == 0)
	{
		//Has char been playing too long?
		if(ent->myskills.playingtime > (MAX_HOURS * 3600) )	//Playing time in seconds?
			return true;
	}
	else
	{
		//Reset playing time for today
		ent->myskills.total_playtime += (ent->myskills.playingtime / 60);
		strcpy(ent->myskills.last_played, CURRENT_DATE);
		ent->myskills.playingtime = 0;
	}

	return false;
}


void vrx_create_new_character(edict_t *ent)
{
	ent->myskills.next_level = vrx_get_points_tnl(ent->myskills.level);
	ent->myskills.respawn_weapon = 7;

	Q_strncpy (ent->myskills.password,
               vrx_encrypt_string(Info_ValueForKey(ent->client->pers.userinfo, "vrx_password"), false), sizeof(ent->myskills.password) - 1);

	strcpy(ent->myskills.member_since, va("%s at %s", CURRENT_DATE, CURRENT_TIME));
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
	Q_strncpy (chkpassword2, vrx_encrypt_string(ent->myskills.password, true), sizeof(chkpassword2) - 1);

	// check if userinfo password matches master password
	// does not accept password if it contains '@' because it might be an email address
	// remove this requirement after a reset
	if ((strlen(ent->myskills.email) > 0) && !strstr(ent->myskills.email, "@") 
		&& !Q_stricmp(chkpassword, ent->myskills.email))
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

	if (!strcmp(ent->myskills.player_name, "Player"))
		return -8; // lol
	
	return 0;	//success
}

void fixInvalidPlayerData(edict_t *ent)
{
	int i;
	gitem_t *item=itemlist;

	for (i=0; i<game.num_items; i++, item++)
		ent->client->pers.inventory[ITEM_INDEX(item)] = ent->myskills.inventory[ITEM_INDEX(item)];
	ent->client->pers.inventory[flag_index] = 0;

	vrx_update_all_character_maximums(ent);

	if (ent->myskills.credits < 0)
		ent->myskills.credits = 0;
}

#endif