#include "g_local.h"
#include "v_characterio.h"
#include <sys/stat.h>

// az begin

#ifdef _WIN32
#pragma warning ( disable : 4090 ; disable : 4996 )
#include <direct.h>
#endif

// ===  CVARS  ===
cvar_t *savemethod;

int CountAbilities(edict_t *player)
{
	int i;
	int count = 0;
	for (i = 0; i < MAX_ABILITIES; ++i)
	{
		if (!player->myskills.abilities[i].disable)
			++count;
	}
	return count;
}

//************************************************

int FindAbilityIndex(int index, edict_t *player)
{
    int i;
	int count = 0;
	for (i = 0; i < MAX_ABILITIES; ++i)
	{
		if (!player->myskills.abilities[i].disable)
		{
			++count;
			if (count == index)
				return i;
		}
	}
	return -1;	//just in case something messes up
}

//************************************************
//************************************************

int CountWeapons(edict_t *player)
{
	int i;
	int count = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (V_WeaponUpgradeVal(player, i) > 0)
			count++;
	}
	return count;
}

//************************************************

int FindWeaponIndex(int index, edict_t *player)
{
	int i;
	int count = 0;
	for (i = 0; i < MAX_WEAPONS; ++i)
	{
		if (V_WeaponUpgradeVal(player, i) > 0)
		{
			count++;
			if (count == index)
				return i;
		}
	}
	return -1;	//just in case something messes up
}

//************************************************
//************************************************

int CountRunes(edict_t *player)
{
	int count = 0;
	int i;

	for (i = 0; i < MAX_VRXITEMS; ++i)
	{
		if (player->myskills.items[i].itemtype != ITEM_NONE)
			++count;
	}
	return count;
}

//************************************************

int FindRuneIndex(int index, edict_t *player)
{
	int i;
	int count = 0;

	for (i = 0; i < MAX_VRXITEMS; ++i)
	{
		if (player->myskills.items[i].itemtype != ITEM_NONE)
		{
			++count;
			if (count == index)
			{
				return i;
			}
		}
	}
	return -1;	//just in case something messes up
}

//***********************************************************************
//		Save player to file
//		Max value of a signed int32 (4 bytes) = 2147483648
//		That should be plenty for vrx	-doomie
//***********************************************************************

qboolean vrx_commit_character(edict_t *ent, qboolean unlock)
{
	//Make sure this is a client
	if (!ent->client)
	{
		gi.dprintf("ERROR: entity not a client!! (%s)\n",ent->classname);
		return false;
	}

	if (G_IsSpectator(ent))
	{
		gi.dprintf("Warning: Tried to save a spectator's stats.\n");
		return false;
	}

	if(debuginfo->value)
		gi.dprintf("savePlayer called to save: %s\n", ent->client->pers.netname);

    if (!unlock)
	    return vrx_char_io.save_player(ent);
    else
        return vrx_char_io.save_close_player(ent);
}

#ifdef WIN32
#endif

void CreateDirIfNotExists(char *path)
{
	struct stat s;
	if (stat(path, &s))
	{
#ifdef WIN32
		if (_mkdir(path) != 0)
		{
			gi.dprintf("Error creating missing directory %s.\n", path);
		}else
			gi.dprintf("Created directory %s.\n", path);
#else
		mkdir(path, S_IWUSR);
#endif
	}
}

//***********************************************************************
//		open player from file
//***********************************************************************

qboolean vrx_load_player(edict_t *ent)
{
	int		i;

	//Make sure this is a client
	if (!ent->client)
	{
		gi.dprintf("ERROR: entity not a client!! (%s)\n",ent->classname);
		return false;
	}

	if(debuginfo->value)
		gi.dprintf("vrx_load_player called to open: %s\n", ent->client->pers.netname);

	//Reset the player's skills_t
	memset(&ent->myskills,0,sizeof(skills_t));

	if (vrx_char_io.character_exists && // if the function is set and
        !vrx_char_io.character_exists(ent)) // the character does not exist...
        return false;

	//disable all abilities
	for (i = 0; i < MAX_ABILITIES; ++i)
		ent->myskills.abilities[i].disable = true;

	return vrx_char_io.load_player(ent);
}

