#include "g_local.h"
#include "../characters/io/v_characterio.h"

// settings.h
const char *s1;
const char *s2;

// g_local.h
gitem_t	*Fdi_GRAPPLE;
gitem_t	*Fdi_BLASTER;
gitem_t *Fdi_SHOTGUN;
gitem_t *Fdi_SUPERSHOTGUN;
gitem_t *Fdi_MACHINEGUN;
gitem_t *Fdi_CHAINGUN;
gitem_t *Fdi_GRENADES;
gitem_t *Fdi_GRENADELAUNCHER;
gitem_t *Fdi_ROCKETLAUNCHER;
gitem_t *Fdi_HYPERBLASTER;
gitem_t *Fdi_RAILGUN;
gitem_t *Fdi_BFG;
gitem_t *Fdi_PHALANX;
gitem_t *Fdi_BOOMER;
gitem_t *Fdi_TRAP;
gitem_t *Fdi_20MM;

gitem_t *Fdi_SHELLS;
gitem_t *Fdi_BULLETS;
gitem_t *Fdi_CELLS;
gitem_t *Fdi_ROCKETS;
gitem_t *Fdi_SLUGS;
gitem_t *Fdi_MAGSLUGS;
gitem_t *Fdi_TBALL;
gitem_t	*Fdi_POWERCUBE;

int headindex;
int	skullindex;


cvar_t				*bot_showpath;
cvar_t				*bot_showcombat;
cvar_t				*bot_showsrgoal;
cvar_t				*bot_showlrgoal;
cvar_t				*bot_debugmonster;

field_t fields[] = {
	{ "classname", FOFS(classname), F_LSTRING },
	{ "origin", FOFS(s.origin), F_VECTOR },
	{ "model", FOFS(model), F_LSTRING },
	{ "spawnflags", FOFS(spawnflags), F_INT },
	{ "speed", FOFS(speed), F_FLOAT },
	{ "accel", FOFS(accel), F_FLOAT },
	{ "decel", FOFS(decel), F_FLOAT },
	{ "target", FOFS(target), F_LSTRING },
	{ "targetname", FOFS(targetname), F_LSTRING },
	{ "pathtarget", FOFS(pathtarget), F_LSTRING },
        // { "deathtarget", FOFS(deathtarget), F_LSTRING },
	{ "killtarget", FOFS(killtarget), F_LSTRING },
        // { "combattarget", FOFS(combattarget), F_LSTRING },
	{ "message", FOFS(message), F_LSTRING },
	{ "team", FOFS(team), F_LSTRING },
	{ "wait", FOFS(wait), F_FLOAT },
	{ "delay", FOFS(delay), F_FLOAT },
	{ "random", FOFS(random), F_FLOAT },
	{ "move_origin", FOFS(move_origin), F_VECTOR },
	{ "move_angles", FOFS(move_angles), F_VECTOR },
	{ "style", FOFS(style), F_INT },
	{ "count", FOFS(count), F_INT },
	{ "health", FOFS(health), F_INT },
	{ "sounds", FOFS(sounds), F_INT },
	{ "light", 0, F_IGNORE },
	{ "dmg", FOFS(dmg), F_INT },
	{ "angles", FOFS(s.angles), F_VECTOR },
	{ "angle", FOFS(s.angles), F_ANGLEHACK },
	{ "mass", FOFS(mass), F_INT },
	{ "volume", FOFS(volume), F_FLOAT },
	{ "attenuation", FOFS(attenuation), F_FLOAT },
	{ "map", FOFS(map), F_LSTRING },
	{"wf_team", FOFS(teamnum), F_INT },

	//arena
	{ "arena", FOFS(arena), F_INT },
	//end arena
	// temp spawn vars -- only valid when the spawn function is called
	{ "lip", STOFS(lip), F_INT, FFL_SPAWNTEMP },
	{ "distance", STOFS(distance), F_INT, FFL_SPAWNTEMP },
	{ "height", STOFS(height), F_INT, FFL_SPAWNTEMP },
	{ "noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP },
	{ "pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP },
	{ "item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP },
	{ "gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP },
	{ "sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP },
	{ "skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP },
	{ "skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP },
	{ "minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP },
	{ "minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP },
	{ "maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP },
	{ "nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP },
	{ NULL, 0, F_IGNORE, FFL_SPAWNTEMP }
};

// -------- just for savegames ----------
// all pointer fields should be listed here, or savegames
// won't work properly (they will crash and burn).
// this wasn't just tacked on to the fields array, because
// these don't need names, we wouldn't want map fields using
// some of these, and if one were accidentally present twice
// it would double swizzle (fuck) the pointer.

field_t		savefields[] =
{
	{ "", FOFS(classname), F_LSTRING },
	{ "", FOFS(target), F_LSTRING },
	{ "", FOFS(targetname), F_LSTRING },
	{ "", FOFS(killtarget), F_LSTRING },
	{ "", FOFS(team), F_LSTRING },
	{ "", FOFS(pathtarget), F_LSTRING },
//	{ "", FOFS(deathtarget), F_LSTRING },
                // { "", FOFS(combattarget), F_LSTRING },
	{ "", FOFS(model), F_LSTRING },
	{ "", FOFS(map), F_LSTRING },
	{ "", FOFS(message), F_LSTRING },

	{ "", FOFS(client), F_CLIENT },
	{ "", FOFS(item), F_ITEM },

	{ "", FOFS(goalentity), F_EDICT },
	{ "", FOFS(movetarget), F_EDICT },
	{ "", FOFS(enemy), F_EDICT },
	{ "", FOFS(oldenemy), F_EDICT },
	{ "", FOFS(activator), F_EDICT },
	{ "", FOFS(groundentity), F_EDICT },
	{ "", FOFS(teamchain), F_EDICT },
	{ "", FOFS(teammaster), F_EDICT },
	{ "", FOFS(owner), F_EDICT },
	// {"", FOFS(mynoise), F_EDICT},
	// {"", FOFS(mynoise2), F_EDICT},
	{ "", FOFS(target_ent), F_EDICT },
	{ "", FOFS(chain), F_EDICT },

	{ NULL, 0, F_INT }
};

field_t		levelfields[] =
{
	{ "", LLOFS(changemap), F_LSTRING },

	{ "", LLOFS(sight_client), F_EDICT },
	{ "", LLOFS(sight_entity), F_EDICT },
	{ "", LLOFS(sound_entity), F_EDICT },
	{ "", LLOFS(sound2_entity), F_EDICT },

	{ NULL, 0, F_INT }
};

field_t		clientfields[] =
{
	{ "", CLOFS(pers.weapon), F_ITEM },
	{ "", CLOFS(pers.lastweapon), F_ITEM },
	{ "", CLOFS(newweapon), F_ITEM },

	{ NULL, 0, F_INT }
};


/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/



void InitGame(void)
{
	gi.dprintf("==== InitGame ====\n");


	//K03 Begin
	srand((unsigned)time(0));

	gamedir = gi.cvar("gamedir", "vortex", CVAR_SERVERINFO);
	save_path = gi.cvar("save_path", va("%s//characters", gamedir->string), CVAR_LATCH);
	//K03 End


	// az begin
	gi.cvar_forceset("g_features", va("%d", GMF_VARIABLE_FPS));

	savemethod = gi.cvar("savemethod", "3", 0);
	generalabmode = gi.cvar("generalabmode", "1", CVAR_LATCH);
	game_path = gi.cvar("game", ".", CVAR_LATCH);

#ifdef Q2PRO_COMPATIBILITY
	gi.cvar_forceset("sv_allow_map", "2");
#endif

	defer_global_init();
	vrx_init_lua();
    CreateDirIfNotExists(va("%s/settings", gamedir->string));

	vrx_init_char_io();
	vrx_init_stash_io();

#ifdef CMD_USEHASH
	InitHash();
#endif

    vrx_init_ability_list();
	// az end

	gun_x = gi.cvar("gun_x", "0", 0);
	gun_y = gi.cvar("gun_y", "0", 0);
	gun_z = gi.cvar("gun_z", "0", 0);

	sv_fps = gi.cvar("sv_fps", "10", CVAR_LATCH);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar("sv_gravity", "800", 0);

	// noset vars
	dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

	// latched vars
	// sv_cheats = gi.cvar ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	gi.cvar("gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_LATCH);

	maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
	coop = gi.cvar("coop", "0", CVAR_LATCH);
	skill = gi.cvar("skill", "1", CVAR_SERVERINFO);
	maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);
	//chain edit flag
	//vwep support
	vwep = gi.cvar("vwep", "1", CVAR_LATCH);
	//ZOID
	//This game.dll only supports deathmatch
	/*if (!deathmatch->value)
	{
	gi.dprintf("Forcing deathmatch.\n");
	gi.cvar_set("deathmatch", "1");
	}
	//force coop off
	if (coop->value)
	gi.cvar_set("coop", "0");
	*/
	//ZOID

	// change anytime vars
	dmflags = gi.cvar("dmflags", "0", 0);
	fraglimit = gi.cvar("fraglimit", "100", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "21", CVAR_SERVERINFO);
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

	filterban = gi.cvar("filterban", "1", 0);

	run_pitch = gi.cvar("run_pitch", "0.002", 0);
	run_roll = gi.cvar("run_roll", "0.005", 0);
	bob_up = gi.cvar("bob_up", "0.005", 0);
	bob_pitch = gi.cvar("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar("bob_roll", "0.002", 0);

	//K03 Begin

	killboxspawn = gi.cvar("killboxatspawn", "1", 0);
	particles = gi.cvar("particles", "0", 0);

	// az 3.0
	hw = gi.cvar("hw", "0", CVAR_LATCH);

	vrx_password = gi.cvar("vrx_password", "", CVAR_ARCHIVE | CVAR_USERINFO);
	dm_monsters = gi.cvar("dm_monsters", "8", CVAR_SERVERINFO);
	hostname = gi.cvar("hostname", "q2vortex.com", CVAR_SERVERINFO);

	// vrxchile v1.3 ally half server
	allies = gi.cvar("allies", "1", CVAR_SERVERINFO | CVAR_LATCH);
	reconnect_ip = gi.cvar("reconnect_ip", "0", CVAR_ARCHIVE);

	// flood control
	flood_msgs = gi.cvar("flood_msgs", "4", 0);
	flood_persecond = gi.cvar("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);
	//maplist value
	min_level = gi.cvar("min_level", "0", 0);
	max_level = gi.cvar("max_level", "1000", 0);
	check_dupeip = gi.cvar("check_dupeip", "0", 0);
	check_dupename = gi.cvar("check_dupename", "1", 0);
	newbie_protection = gi.cvar("newbie_protection", "1.8", CVAR_ARCHIVE);
	debuginfo = gi.cvar("debuginfo", "0", CVAR_ARCHIVE);
	pvm = gi.cvar("pvm", "0", CVAR_LATCH);
	ffa = gi.cvar("ffa", "1", CVAR_LATCH);
	ptr = gi.cvar("ptr", "0", CVAR_LATCH);
	domination = gi.cvar("domination", "0", CVAR_LATCH);
	ctf = gi.cvar("ctf", "0", CVAR_LATCH);
	invasion = gi.cvar("invasion", "0", CVAR_LATCH);
	nolag = gi.cvar("nolag", "1", 0);

	// az v2.5 vrxchile trading mode
	trading = gi.cvar("trading", "0", 0);
    tradingmode_enabled = gi.cvar("vrx_trading_enabled", "1", CVAR_LATCH);
	tbi = gi.cvar("tbi", "0", CVAR_LATCH); // enable tbi mode
	// az end

	// az: pvm_respawntime, pvm_monstermult and ffa_monstermult currently unused
	pvm_respawntime = gi.cvar("pvm_respawntime", "15", 0);
	pvm_monstermult = gi.cvar("pvm_monstermult", "1.0", 0);
	ffa_respawntime = gi.cvar("ffa_respawntime", "15", 0);
	ffa_monstermult = gi.cvar("ffa_monstermult", "1.0", 0);

	// az note: only used in teamplay.c, which is unused....
	team1_skin = gi.cvar("team1_skin", "female/ctf_r", 0);
	team2_skin = gi.cvar("team2_skin", "male/ctf_b", 0);

	// class skins
	enforce_class_skins = gi.cvar("enforce_class_skins", "1", CVAR_LATCH);

	class1_model = gi.cvar("class1_model", "terran", CVAR_LATCH); // soldier
	class2_model = gi.cvar("class2_model", "zumlin", CVAR_LATCH); // polt
	class3_model = gi.cvar("class3_model", "caleb", CVAR_LATCH); // vampire
	class4_model = gi.cvar("class4_model", "linfang", CVAR_LATCH); // mage
	class5_model = gi.cvar("class5_model", "terminator", CVAR_LATCH); // engy
	class6_model = gi.cvar("class6_model", "pknight", CVAR_LATCH); // knight
	class7_model = gi.cvar("class7_model", "alita", CVAR_LATCH); // cleric

	if (generalabmode->value)
		class8_model = gi.cvar("class8_model", "ratamahatta", CVAR_LATCH); // weapon master
	else
		class8_model = gi.cvar("class8_model", "marine", CVAR_LATCH); // apprentice

	class9_model = gi.cvar("class9_model", "slith", CVAR_LATCH); // necromancer
	class10_model = gi.cvar("class10_model", "bauul", CVAR_LATCH); // shaman
	class11_model = gi.cvar("class11_model", "xenoid", CVAR_LATCH); // alien
	class12_model = gi.cvar("class12_model", "ddz", CVAR_LATCH); // kamikaze



	class1_skin = gi.cvar("class1_skin", "blue", CVAR_LATCH); // soldier
	class2_skin = gi.cvar("class2_skin", "zumlin", CVAR_LATCH); // polt
	class3_skin = gi.cvar("class3_skin", "caleb_undead", CVAR_LATCH); // vampire
	class4_skin = gi.cvar("class4_skin", "ctf_r", CVAR_LATCH); // mage
	class5_skin = gi.cvar("class5_skin", "blood", CVAR_LATCH); // engy
	class6_skin = gi.cvar("class6_skin", "knight", CVAR_LATCH); // knight
	class7_skin = gi.cvar("class7_skin", "alita", CVAR_LATCH); // cleric
	if (generalabmode->value)
		class8_skin = gi.cvar("class8_skin", "ratamahatta", CVAR_LATCH); // weapon master
	else
		class8_skin = gi.cvar("class8_skin", "brownie", CVAR_LATCH); // apprentice
	class9_skin = gi.cvar("class9_skin", "chompers", CVAR_LATCH); // necromancer
	class10_skin = gi.cvar("class10_skin", "bauul", CVAR_LATCH); // shaman
	class11_skin = gi.cvar("class11_skin", "xenotype1", CVAR_LATCH); // alien
	class12_skin = gi.cvar("class12_skin", "ctf_g", CVAR_LATCH); // kamikaze

	// minimum map ammo
	world_min_shells = gi.cvar("world_min_shells", "1", 0);
	world_min_bullets = gi.cvar("world_min_bullets", "1", 0);
	world_min_rockets = gi.cvar("world_min_rockets", "1", 0);
	world_min_grenades = gi.cvar("world_min_grenades", "1", 0);
	world_min_cells = gi.cvar("world_min_cells", "1", 0);
	world_min_slugs = gi.cvar("world_min_slugs", "1", 0);

	// enable special rules for flag carrier in CTF mode
	ctf_enable_balanced_fc = gi.cvar("ctf_enable_balanced_fc", "1", CVAR_LATCH);
	adminctrl = gi.cvar("admin_forcevote", "0", CVAR_LATCH);

	voting = gi.cvar("voting", "1", CVAR_SERVERINFO);
	pregame_time = gi.cvar("pregame_time", "60.0", 0);

    start_level = gi.cvar("start_level", "1", CVAR_LATCH);

    invasion_enabled = gi.cvar("vrx_invasion_enabled", "1", CVAR_LATCH);
	vrx_pointmult = gi.cvar("vrx_pointmult", "1.0", CVAR_SERVERINFO/* | CVAR_LATCH*/);
	vrx_pvppointmult = gi.cvar("vrx_pvppointmult", "1.0", 0);
	vrx_pvmpointmult = gi.cvar("vrx_pvmpointmult", "1.0", 0);

	vrx_creditmult = gi.cvar("vrx_creditmult", "1.5", 0/* | CVAR_LATCH*/);
	vrx_pvpcreditmult = gi.cvar("vrx_pvpcreditmult", "2.0", 0);
	vrx_pvmcreditmult = gi.cvar("vrx_pvmcreditmult", "1.0", 0);
    adminpass = gi.cvar("vrx_admin_pass", "", CVAR_ARCHIVE);
	//K03 End

	// items
	InitItems();

	Com_sprintf(game.helpmessage1, sizeof(game.helpmessage1), "");

	Com_sprintf(game.helpmessage2, sizeof(game.helpmessage2), "");

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts = V_Malloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->value;
	game.clients = V_Malloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients + 1;

	//3.0 Load the custom map lists
	if (vrx_load_map_list(MAPMODE_PVP) && vrx_load_map_list(MAPMODE_PVM) && vrx_load_map_list(MAPMODE_INV)
		&& vrx_load_map_list(MAPMODE_DOM) && vrx_load_map_list(MAPMODE_CTF) && vrx_load_map_list(MAPMODE_FFA)
		&& vrx_load_map_list(MAPMODE_TRA) && vrx_load_map_list(MAPMODE_INH) && vrx_load_map_list(MAPMODE_VHW)
		&& vrx_load_map_list(MAPMODE_TBI))
		gi.dprintf("INFO: Vortex Custom Map Lists loaded successfully\n");
	else
		gi.dprintf("WARNING: Error loading custom map lists\n");

	//3.0 Load the armory
	LoadArmory();

	InitializeTeamNumbers(); // for allies

	// az begin
	AI_Init();
	// az end
}

//=========================================================

void WriteField1(FILE *f, field_t *field, byte *base)
{
	void		*p;
	int			len;
	int			index;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;

	case F_LSTRING:
	case F_GSTRING:
		if (*(char **)p)
			len = strlen(*(char **)p) + 1;
		else
			len = 0;
		*(int *)p = len;
		break;
	case F_EDICT:
		if (*(edict_t **)p == NULL)
			index = -1;
		else
			index = *(edict_t **)p - g_edicts;
		*(int *)p = index;
		break;
	case F_CLIENT:
		if (*(gclient_t **)p == NULL)
			index = -1;
		else
			index = *(gclient_t **)p - game.clients;
		*(int *)p = index;
		break;
	case F_ITEM:
		if (*(edict_t **)p == NULL)
			index = -1;
		else
			index = *(gitem_t **)p - itemlist;
		*(int *)p = index;
		break;

	default:
		gi.error("WriteEdict: unknown field type");
	}
}

void WriteField2(FILE *f, field_t *field, byte *base)
{
	int			len;
	void		*p;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_LSTRING:
	case F_GSTRING:
		if (*(char **)p)
		{
			len = strlen(*(char **)p) + 1;
			fwrite(*(char **)p, len, 1, f);
		}
		break;
	default:
		break;
	}
}

static void ReadField(FILE *f, field_t *field, byte *base)
{
	void		*p;
	int			len;
	int			index;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;

	case F_LSTRING:
		len = *(int *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = V_Malloc(len, TAG_LEVEL);
			fread(*(char **)p, len, 1, f);
		}
		break;
	case F_GSTRING:
		len = *(int *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = V_Malloc(len, TAG_GAME);
			fread(*(char **)p, len, 1, f);
		}
		break;
	case F_EDICT:
		index = *(int *)p;
		if (index == -1)
			*(edict_t **)p = NULL;
		else
			*(edict_t **)p = &g_edicts[index];
		break;
	case F_CLIENT:
		index = *(int *)p;
		if (index == -1)
			*(gclient_t **)p = NULL;
		else
			*(gclient_t **)p = &game.clients[index];
		break;
	case F_ITEM:
		index = *(int *)p;
		if (index == -1)
			*(gitem_t **)p = NULL;
		else
			*(gitem_t **)p = &itemlist[index];
		break;

	default:
		gi.error("ReadEdict: unknown field type");
	}
}

//=========================================================

/*
==============
WriteClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteClient(FILE *f, gclient_t *client)
{
	field_t		*field;
	gclient_t	temp;

	// all of the ints, floats, and vectors stay as they are
	temp = *client;

	// change the pointers to lengths or indexes
	for (field = clientfields; field->name; field++)
	{
		WriteField1(f, field, (byte *)&temp);
	}

	// write the block
	fwrite(&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field = clientfields; field->name; field++)
	{
		WriteField2(f, field, (byte *)client);
	}
}

/*
==============
ReadClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void ReadClient(FILE *f, gclient_t *client)
{
	field_t		*field;

	fread(client, sizeof(*client), 1, f);

	for (field = clientfields; field->name; field++)
	{
		ReadField(f, field, (byte *)client);
	}
}

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
void WriteGame(char *filename, qboolean autosave)
{
	FILE	*f;
	int		i;
	char	str[16];

	if (!autosave)
		SaveClientData();

	f = fopen(filename, "wb");
	if (!f)
		gi.error("Couldn't open %s", filename);

	memset(str, 0, sizeof(str));
	strcpy(str, __DATE__);
	fwrite(str, sizeof(str), 1, f);

	game.autosaved = autosave;
	fwrite(&game, sizeof(game), 1, f);
	game.autosaved = false;

	for (i = 0; i<game.maxclients; i++)
		WriteClient(f, &game.clients[i]);

	fclose(f);
}

void ReadGame(char *filename)
{
	FILE	*f;
	int		i;
	char	str[16];

	gi.FreeTags(TAG_GAME);

	f = fopen(filename, "rb");
	if (!f)
		gi.error("Couldn't open %s", filename);

	fread(str, sizeof(str), 1, f);
	if (strcmp(str, __DATE__))
	{
		fclose(f);
		gi.error("Savegame from an older version.\n");
	}

	g_edicts = V_Malloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;

	fread(&game, sizeof(game), 1, f);
	game.clients = V_Malloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	for (i = 0; i<game.maxclients; i++)
		ReadClient(f, &game.clients[i]);

	fclose(f);
}

//==========================================================


/*
==============
WriteEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteEdict(FILE *f, edict_t *ent)
{
	field_t		*field;
	edict_t		temp;

	// all of the ints, floats, and vectors stay as they are
	temp = *ent;

	// change the pointers to lengths or indexes
	for (field = savefields; field->name; field++)
	{
		WriteField1(f, field, (byte *)&temp);
	}

	// write the block
	fwrite(&temp, sizeof(temp), 1, f);

	// now write any allocated data following the edict
	for (field = savefields; field->name; field++)
	{
		WriteField2(f, field, (byte *)ent);
	}

}

/* Local for WriteLevelLocals because size of level_locals_t exceeds stack space available. */
static level_locals_t		WLL_temp;

/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void WriteLevelLocals(FILE *f)
{
	field_t		*field;

	// all of the ints, floats, and vectors stay as they are
	WLL_temp = level;

	// change the pointers to lengths or indexes
	for (field = levelfields; field->name; field++)
	{
		WriteField1(f, field, (byte *)&WLL_temp);
	}

	// write the block
	fwrite(&WLL_temp, sizeof(WLL_temp), 1, f);

	// now write any allocated data following the edict
	for (field = levelfields; field->name; field++)
	{
		WriteField2(f, field, (byte *)&level);
	}
}


/*
==============
ReadEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void ReadEdict(FILE *f, edict_t *ent)
{
	field_t		*field;

	fread(ent, sizeof(*ent), 1, f);

	for (field = savefields; field->name; field++)
	{
		ReadField(f, field, (byte *)ent);
	}
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void ReadLevelLocals(FILE *f)
{
	field_t		*field;

	fread(&level, sizeof(level), 1, f);

	for (field = levelfields; field->name; field++)
	{
		ReadField(f, field, (byte *)&level);
	}
}

/*
=================
WriteLevel

=================
*/
void WriteLevel(char *filename)
{
	int		i;
	edict_t	*ent;
	FILE	*f;
	void	(*base)(void);

	f = fopen(filename, "wb");
	if (!f) {
		gi.error("Couldn't open %s", filename);
		exit(1); // never gets here
	}

	// write out edict size for checking
	i = sizeof(edict_t);
	fwrite(&i, sizeof(i), 1, f);

	// write out a function pointer for checking
	base = InitGame;
	fwrite(&base, sizeof(base), 1, f);

	// write out level_locals_t
	WriteLevelLocals(f);

	// write out all the entities
	for (i = 0; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		fwrite(&i, sizeof(i), 1, f);
		WriteEdict(f, ent);
	}
	i = -1;
	fwrite(&i, sizeof(i), 1, f);

	fclose(f);
}


/*
=================
ReadLevel

SpawnEntities will already have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel(char *filename)
{
	int		entnum;
	FILE	*f;
	int		i;
	void	*base;
	edict_t	*ent;

	f = fopen(filename, "rb");
	if (!f) {
		gi.error("Couldn't open %s", filename);
		return;
	}

	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags(TAG_LEVEL);

	// wipe all the entities
	memset(g_edicts, 0, game.maxentities*sizeof(g_edicts[0]));
	globals.num_edicts = maxclients->value + 1;

	// check edict size
	fread(&i, sizeof(i), 1, f);
	if (i != sizeof(edict_t))
	{
		fclose(f);
		gi.error("ReadLevel: mismatched edict size");
	}

	// check function pointer base address
	fread(&base, sizeof(base), 1, f);
	if (base != (void *)InitGame)
	{
		fclose(f);
		gi.error("ReadLevel: function pointers have moved");
	}

	// load the level locals
	ReadLevelLocals(f);

	// load all the entities
	while (1)
	{
		if (fread(&entnum, sizeof(entnum), 1, f) != 1)
		{
			fclose(f);
			gi.error("ReadLevel: failed to read entnum");
		}
		if (entnum == -1)
			break;
		if (entnum >= globals.num_edicts)
			globals.num_edicts = entnum + 1;

		ent = &g_edicts[entnum];
		ReadEdict(f, ent);

		// let the server rebuild world links for this ent
		memset(&ent->area, 0, sizeof(ent->area));
		gi.linkentity(ent);
	}

	fclose(f);

	// mark all clients as unconnected
	for (i = 0; i<maxclients->value; i++)
	{
		ent = &g_edicts[i + 1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
	}

	// do any load time things at this point
	for (i = 0; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level triggers
		if (ent->classname)
			if (strcmp(ent->classname, "target_crosslevel_target") == 0)
				ent->nextthink = level.time + ent->delay;
	}
}
