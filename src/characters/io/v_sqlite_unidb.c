#include "g_local.h"

#include <sys/stat.h>

#include "v_characterio.h"
#include "../../libraries/sqlite3.h"

#include "../class_limits.h"

#ifdef _WIN32
#pragma warning ( disable : 4090 ; disable : 4996 )
#endif

// *********************************
// Definitions
// *********************************
#define TOTAL_TABLES 15
#define TOTAL_INSERTONCE 5
#define TOTAL_RESETTABLES 7

void cdb_start_connection();

void cdb_end_connection();

const char* VSFU_CREATEDBQUERY[TOTAL_TABLES] =
{
	"CREATE TABLE [abilities] ( [char_idx] INTEGER,[index] INTEGER, [level] INTEGER, [max_level] INTEGER, [hard_max] INTEGER, [modifier] INTEGER,   [disable] INTEGER,   [general_skill] INTEGER)",
	"CREATE TABLE [ctf_stats] ( [char_idx] INTEGER,  [flag_pickups] INTEGER,   [flag_captures] INTEGER,   [flag_returns] INTEGER,   [flag_kills] INTEGER,   [offense_kills] INTEGER,   [defense_kills] INTEGER,   [assists] INTEGER)",
	"CREATE TABLE [game_stats] ([char_idx] INTEGER,  [shots] INTEGER,   [shots_hit] INTEGER,   [frags] INTEGER,   [fragged] INTEGER,   [num_sprees] INTEGER,   [max_streak] INTEGER,   [spree_wars] INTEGER,   [broken_sprees] INTEGER,   [broken_spreewars] INTEGER,   [suicides] INT,   [teleports] INTEGER,   [num_2fers] INTEGER)",
	"CREATE TABLE [point_data] ([char_idx] INTEGER,  [exp] INTEGER,   [exptnl] INTEGER,   [level] INTEGER,   [classnum] INTEGER,   [skillpoints] INTEGER,   [credits] INTEGER,   [weap_points] INTEGER,   [resp_weapon] INTEGER,   [tpoints] INTEGER)",
	"CREATE TABLE [runes_meta] ([char_idx] INTEGER,[index] INTEGER, [itemtype] INTEGER, [itemlevel] INTEGER, [quantity] INTEGER, [untradeable] INTEGER, [id] CHAR(16), [name] CHAR(24), [nummods] INTEGER, [setcode] INTEGER, [classnum] INTEGER)",
	"CREATE TABLE [runes_mods] ([char_idx] INTEGER,  [rune_index] INTEGER, [type] INTEGER, [mod_index] INTEGER, [value] INTEGER, [set] INTEGER)",
	"CREATE TABLE [talents] (   [char_idx] INTEGER,[id] INTEGER, [upgrade_level] INTEGER, [max_level] INTEGER)",
	"CREATE TABLE [userdata] (  [char_idx] INTEGER,[title] CHAR(24), [playername] CHAR(64), [password] CHAR(24), [email] CHAR(64), [owner] CHAR(24), [member_since] CHAR(30), [last_played] CHAR(30), [playtime_total] INTEGER,[playingtime] INTEGER)",
	"CREATE TABLE [weapon_meta] ([char_idx] INTEGER,[index] INTEGER, [disable] INTEGER)",
	"CREATE TABLE [weapon_mods] ([char_idx] INTEGER,[weapon_index] INTEGER, [modindex] INTEGER, [level] INTEGER, [soft_max] INTEGER, [hard_max] INTEGER)",
	"CREATE TABLE [character_data] ([char_idx] INTEGER,  [respawns] INTEGER,   [health] INTEGER,   [maxhealth] INTEGER,   [armour] INTEGER,   [maxarmour] INTEGER,   [nerfme] INTEGER,   [adminlevel] INTEGER,   [bosslevel] INTEGER, [prestigelevel] INTEGER, [prestigepoints] INTEGER)",
	"create table [prestige](char_idx int not null, pindex int not null, param int not null, level int not null, primary key (char_idx, pindex, param))"
	"create table stash( char_idx int not null, lock_char_id int null, primary key (char_idx));",
	"create table stash_runes_meta( char_idx int not null, stash_index int not null, itemtype int null, itemlevel int null, quantity int null, untradeable int null, id char(16) null, name char(24) null, nummods int null, setcode int null, classnum int null, primary key (char_idx, stash_index));",
	"create table stash_runes_mods( char_idx int not null, stash_index int not null, rune_mod_index int not null, type int null, mod_index int null, value int null, [set] int null, primary key (char_idx, stash_index, rune_mod_index));"
};

// SAVING

const char* CreateCharacterData = "INSERT INTO character_data VALUES (%d,0,0,0,0,0,0,0,0,0,0)";
const char* CreateCtfStats = "INSERT INTO ctf_stats VALUES (%d,0,0,0,0,0,0,0)";
const char* CreateGameStats = "INSERT INTO game_stats VALUES (%d,0,0,0,0,0,0,0,0,0,0,0,0)";
const char* CreatePointData = "INSERT INTO point_data VALUES (%d,0,0,0,0,0,0,0,0,0)";
const char* CreateUserData = "INSERT INTO userdata VALUES (%d,\"\",\"\",\"\",\"\",\"\",\"\",\"\",0,0)";
const char* CreateStashData = "INSERT INTO stash(char_idx) VALUES (%d)";

const char* VSFU_RESETTABLES[TOTAL_RESETTABLES] =
{
	"DELETE FROM abilities WHERE char_idx=%d;",
	"DELETE FROM talents WHERE char_idx=%d;",
	"DELETE FROM prestige WHERE char_idx=%d;",
	"DELETE FROM runes_meta WHERE char_idx=%d;",
	"DELETE FROM runes_mods WHERE char_idx=%d;",
	"DELETE FROM weapon_meta WHERE char_idx=%d;",
	"DELETE FROM weapon_mods WHERE char_idx=%d;"
};

// ab/talent

const char* VSFU_INSERTABILITY = "INSERT INTO abilities VALUES (%d,%d,%d,%d,%d,%d,%d,%d);";

const char* VSFU_INSERTTALENT = "INSERT INTO talents VALUES (%d,%d,%d,%d);";

// weapons

const char* VSFU_INSERTWMETA = "INSERT INTO weapon_meta VALUES (%d,%d,%d);";

const char* VSFU_INSERTWMOD = "INSERT INTO weapon_mods VALUES (%d,%d,%d,%d,%d,%d);";

// runes

const char* VSFU_INSERTRMETA = "INSERT INTO runes_meta VALUES (%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,%d,%d);";

const char* VSFU_INSERTRMOD = "INSERT INTO runes_mods VALUES (%d,%d,%d,%d,%d,%d);";

const char* VSFU_INSERTRMODEX = "INSERT INTO runes_mods VALUES (%d,%d,0,%d,%d,%d,%d);";


const char* VSFU_UPDATECDATA = "UPDATE character_data SET respawns=%d, health=%d, maxhealth=%d, armour=%d, "
							   "maxarmour=%d, nerfme=%d, adminlevel=%d, bosslevel=%d, prestigelevel=%d, prestigepoints=%d"
		  " WHERE char_idx=%d;";

const char* VSFU_UPDATESTATS = "UPDATE game_stats SET shots=%d, shots_hit=%d, frags=%d, fragged=%d, num_sprees=%d, max_streak=%d, spree_wars=%d, broken_sprees=%d, broken_spreewars=%d, suicides=%d, teleports=%d, num_2fers=%d WHERE char_idx=%d;";

const char* VSFU_UPDATEPDATA = "UPDATE point_data SET exp=%d, exptnl=%d, level=%d, classnum=%d, skillpoints=%d, credits=%d, weap_points=%d, resp_weapon=%d, tpoints=%d WHERE char_idx=%d;";

const char* VSFU_UPDATECTFSTATS = "UPDATE ctf_stats SET flag_pickups=%d, flag_captures=%d, flag_returns=%d, flag_kills=%d, offense_kills=%d, defense_kills=%d, assists=%d WHERE char_idx=%d;";

#define CHECK_ERR_RETURN() if(r!=SQLITE_OK){ if(r != SQLITE_ROW && r != SQLITE_OK && r != SQLITE_DONE){gi.dprintf("sqlite error %d: %s\n", r, sqlite3_errmsg(db));return false;}}
#define CHECK_ERR() if(r!=SQLITE_OK){ if(r != SQLITE_ROW && r != SQLITE_OK && r != SQLITE_DONE){gi.dprintf("sqlite error %d: %s\n", r, sqlite3_errmsg(db));}}

#define QUERY(x) { char* format=x;\
	sqlite3_stmt* statement; \
	int r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL); \
	CHECK_ERR_RETURN()\
	r = sqlite3_step(statement); \
	CHECK_ERR_RETURN()\
	sqlite3_finalize(statement); }

#define QUERY_RESULT(x) { char* format=x;\
	r=sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);\
	CHECK_ERR()\
	r=sqlite3_step(statement);\
	CHECK_ERR()}

#define DEFAULT_PATH va("%s/settings/characters.db", game_path->string)
sqlite3* db = NULL;


void cdb_begin_tran(sqlite3* db)
{
	QUERY("BEGIN TRANSACTION;");
}

void cdb_commit_tran(sqlite3* db)
{
	QUERY("COMMIT;")
}

int cdb_get_id(char* playername)
{
	sqlite3_stmt* stmt;
	int r, id;

	char sql[] = "SELECT char_idx FROM userdata WHERE playername=?";
	sqlite3_prepare_v2(db, sql, sizeof sql, &stmt, NULL);
	sqlite3_bind_text(stmt, 1, playername, strlen(playername), SQLITE_STATIC);
	r = sqlite3_step(stmt);

	if (r == SQLITE_ROW) // character exists
	{
		id = sqlite3_column_int(stmt, 0);
	}
	else
		id = -1;

	sqlite3_finalize(stmt);
	return id;
}

// az begin
void cdb_save_runes(edict_t* player)
{
	int numRunes = CountRunes(player);
	int id = cdb_get_id(player->client->pers.netname);

	cdb_begin_tran(db);

	QUERY(va("DELETE FROM runes_meta WHERE char_idx=%d;", id));
	QUERY(va("DELETE FROM runes_mods WHERE char_idx=%d;", id));

	//begin runes
	for (int i = 0; i < numRunes; ++i)
	{
		int index = FindRuneIndex(i + 1, player);
		if (index != -1)
		{
			QUERY(va(VSFU_INSERTRMETA,
				id,
				index,
				player->myskills.items[index].itemtype,
				player->myskills.items[index].itemLevel,
				player->myskills.items[index].quantity,
				player->myskills.items[index].untradeable,
				player->myskills.items[index].id,
				player->myskills.items[index].name,
				player->myskills.items[index].numMods,
				player->myskills.items[index].setCode,
				player->myskills.items[index].classNum));


			for (int j = 0; j < MAX_VRXITEMMODS; ++j)
			{
				const char* query;
				// cant remove columns with sqlite.
				// ugly hack to work around that
				if (vrx_lua_get_int("useMysqlTablesOnSQLite", 0))
					query = VSFU_INSERTRMODEX;
				else
					query = VSFU_INSERTRMOD;

				QUERY(va(query,
					id,
					index,
					player->myskills.items[index].modifiers[j].type,
					player->myskills.items[index].modifiers[j].index,
					player->myskills.items[index].modifiers[j].value,
					player->myskills.items[index].modifiers[j].set));
			}
		}
	}
	//end runes

	cdb_commit_tran(db);
}

int cdb_make_id()
{
	sqlite3_stmt* statement;
	int r, ncount;

	QUERY_RESULT("SELECT COUNT(*) FROM userdata");

	ncount = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);
	return ncount;
}

//************************************************
//************************************************
qboolean cdb_save_player(edict_t* player)
{
	int i, id;
	int numAbilities = CountAbilities(player);
	int numWeapons = CountWeapons(player);
	int numRunes = CountRunes(player);


	id = cdb_get_id(player->client->pers.netname);

	cdb_begin_tran(db);

	if (id == -1)
	{
		// Create initial database.
		id = cdb_make_id();

		gi.dprintf("SQLite (single mode): creating initial data for player id %d.\n", id);

		QUERY(va(CreateCharacterData, id))
		QUERY(va(CreateCtfStats, id))
		QUERY(va(CreateGameStats, id))
		QUERY(va(CreatePointData, id))
		QUERY(va(CreateStashData, id))

		/* sorry about this :( -az*/
		if (!vrx_lua_get_int("useMysqlTablesOnSQLite", 0))
			QUERY(va(CreateUserData, id))
		else
			QUERY(va("INSERT INTO userdata VALUES (%d,\"\",\"\",\"\",\"\",\"\",\"\",\"\",0,0,0)", id))
	}

	{ // real saving
		sqlite3_stmt* statement;

		// reset tables (remove records for reinsertion)
		for (i = 0; i < TOTAL_RESETTABLES; i++)
		{
			QUERY(va(VSFU_RESETTABLES[i], id));
		}

		{
			sqlite3_stmt* stmt;
			char sql[] = "UPDATE userdata "
				"SET title=?, "
				"playername=?, "
				"password=?, "
				"email=?, "
				"owner=?, "
				"member_since=?, "
				"last_played=?, "
				"playtime_total=?, "
				"playingtime=? "
				"WHERE char_idx=?;";
			sqlite3_prepare_v2(db, sql, sizeof sql, &stmt, NULL);


			sqlite3_bind_text(stmt, 1, player->myskills.title, strlen(player->myskills.title), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 2, player->client->pers.netname, strlen(player->client->pers.netname), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 3, player->myskills.password, strlen(player->myskills.password), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 4, player->myskills.email, strlen(player->myskills.email), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 5, player->myskills.owner, strlen(player->myskills.owner), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 6, player->myskills.member_since, strlen(player->myskills.member_since), SQLITE_STATIC);
			sqlite3_bind_text(stmt, 7, player->myskills.last_played, strlen(player->myskills.last_played), SQLITE_STATIC);
			sqlite3_bind_int(stmt, 8, player->myskills.total_playtime);
			sqlite3_bind_int(stmt, 9, player->myskills.playingtime);
			sqlite3_bind_int(stmt, 10, id);

			int r = sqlite3_step(stmt);
			CHECK_ERR();
			sqlite3_finalize(stmt);
		}

		// talents
		for (i = 0; i < player->myskills.talents.count; ++i)
		{
			QUERY(va(VSFU_INSERTTALENT, id, player->myskills.talents.talent[i].id,
				player->myskills.talents.talent[i].upgradeLevel,
				player->myskills.talents.talent[i].maxLevel));
		}

		// abilities

		for (i = 0; i < numAbilities; ++i)
		{
			int index = FindAbilityIndex(i + 1, player);
			if (index != -1)
			{
				char* format = va(VSFU_INSERTABILITY, id, index,
					player->myskills.abilities[index].level,
					player->myskills.abilities[index].max_level,
					player->myskills.abilities[index].hard_max,
					player->myskills.abilities[index].modifier,
					(int)player->myskills.abilities[index].disable,
					(int)player->myskills.abilities[index].general_skill);

				int r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL); // insert ability
				r = sqlite3_step(statement);
				sqlite3_finalize(statement);
			}
		}
		// gi.dprintf("saved abilities");

		//*****************************
		//in-game stats
		//*****************************

		QUERY(va(VSFU_UPDATECDATA,
			player->myskills.weapon_respawns,
			player->myskills.current_health,
			MAX_HEALTH(player),
			player->client->pers.inventory[body_armor_index],
			MAX_ARMOR(player),
			player->myskills.nerfme,
			player->myskills.administrator, // flags
			player->myskills.boss,
			player->myskills.prestige.total,
			player->myskills.prestige.points,
			id));


		//*****************************
		//stats
		//*****************************

		QUERY(va(VSFU_UPDATESTATS,
			player->myskills.shots,
			player->myskills.shots_hit,
			player->myskills.frags,
			player->myskills.fragged,
			player->myskills.num_sprees,
			player->myskills.max_streak,
			player->myskills.spree_wars,
			player->myskills.break_sprees,
			player->myskills.break_spree_wars,
			player->myskills.suicides,
			player->myskills.teleports,
			player->myskills.num_2fers, id));
		
		//*****************************
		//standard stats
		//*****************************

		QUERY(va(VSFU_UPDATEPDATA,
			player->myskills.experience,
			player->myskills.next_level,
			player->myskills.level,
			player->myskills.class_num,
			player->myskills.speciality_points,
			player->myskills.credits,
			player->myskills.weapon_points,
			player->myskills.respawn_weapon,
			player->myskills.talents.talentPoints, id));

		//begin weapons
		for (i = 0; i < numWeapons; ++i)
		{
			int index = FindWeaponIndex(i + 1, player);
			if (index != -1)
			{
				int j;
				QUERY(va(VSFU_INSERTWMETA, id,
					index,
					player->myskills.weapons[index].disable));

				for (j = 0; j < MAX_WEAPONMODS; ++j)
				{
					QUERY(va(VSFU_INSERTWMOD, id,
						index,
						j,
						player->myskills.weapons[index].mods[j].level,
						player->myskills.weapons[index].mods[j].soft_max,
						player->myskills.weapons[index].mods[j].hard_max));
				}
			}
		}
		//end weapons

		//begin runes
		for (i = 0; i < numRunes; ++i)
		{
			int index = FindRuneIndex(i + 1, player);
			if (index != -1)
			{
				int j;

				QUERY(va(VSFU_INSERTRMETA, id,
					index,
					player->myskills.items[index].itemtype,
					player->myskills.items[index].itemLevel,
					player->myskills.items[index].quantity,
					player->myskills.items[index].untradeable,
					player->myskills.items[index].id,
					player->myskills.items[index].name,
					player->myskills.items[index].numMods,
					player->myskills.items[index].setCode,
					player->myskills.items[index].classNum));

				for (j = 0; j < MAX_VRXITEMMODS; ++j)
				{
					const char* query;
					// cant remove columns with sqlite.
					// ugly hack to work around that
					if (vrx_lua_get_int("useMysqlTablesOnSQLite", 0))
						query = VSFU_INSERTRMODEX;
					else
						query = VSFU_INSERTRMOD;

					QUERY(va(query,
						id,
						index,
						player->myskills.items[index].modifiers[j].type,
						player->myskills.items[index].modifiers[j].index,
						player->myskills.items[index].modifiers[j].value,
						player->myskills.items[index].modifiers[j].set));
				}
			}
		}
		//end runes

		QUERY(va(VSFU_UPDATECTFSTATS,
			player->myskills.flag_pickups,
			player->myskills.flag_captures,
			player->myskills.flag_returns,
			player->myskills.flag_kills,
			player->myskills.offense_kills,
			player->myskills.defense_kills,
			player->myskills.assists, id));

		// prestige
		QUERY(va("INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
			id, PRESTIGE_CREDITS, 0, player->myskills.prestige.creditLevel));

		QUERY(va("INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
			id, PRESTIGE_ABILITY_POINTS, 0, player->myskills.prestige.abilityPoints));

		QUERY(va("INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
			id, PRESTIGE_WEAPON_POINTS, 0, player->myskills.prestige.weaponPoints));

		// softmax bump points - param is the ab index, level is the bump
		for (i = 0; i < MAX_ABILITIES; i++)
		{
			if (player->myskills.prestige.softmaxBump[i] > 0) {
				QUERY(va("INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
					id, PRESTIGE_SOFTMAX_BUMP, i, player->myskills.prestige.softmaxBump[i]));
			}
		}

		// class skills - param is the ab index, level is always 0
		for (i = 0; i < MAX_ABILITIES; i++)
		{
			if (player->myskills.prestige.classSkill[i / 32] & (1 << (i % 32)))
			{
				QUERY(va("INSERT INTO prestige(char_idx, pindex, param, level) VALUES (%d, %d, %d, %d);",
					id, PRESTIGE_CLASS_SKILL, i, 0));
			}
		}

	} // end saving

	cdb_commit_tran(db);

	if (player->client->pers.inventory[power_cube_index] > player->client->pers.max_powercubes)
		player->client->pers.inventory[power_cube_index] = player->client->pers.max_powercubes;

	return true;
}

qboolean cdb_saveclose_player(edict_t* player)
{
	int id = cdb_get_id(player->client->pers.netname);
	cdb_save_player(player);

	QUERY(va("update stash set lock_char_id = NULL where lock_char_id=%d", id));
	return true;
}


qboolean cdb_load_player(edict_t* player)
{
	sqlite3_stmt* statement, * statement_mods;
	char* format;
	int numAbilities, numWeapons, numRunes;
	int i, r, id;

	id = cdb_get_id(player->client->pers.netname);

	if (id == -1)
		return false;

	QUERY_RESULT(va("SELECT * FROM userdata WHERE char_idx=%d", id));

	strcpy(player->myskills.title, sqlite3_column_text(statement, 1));
	strcpy(player->myskills.player_name, sqlite3_column_text(statement, 2));
	strcpy(player->myskills.password, sqlite3_column_text(statement, 3));
	strcpy(player->myskills.email, sqlite3_column_text(statement, 4));
	strcpy(player->myskills.owner, sqlite3_column_text(statement, 5));
	strcpy(player->myskills.member_since, sqlite3_column_text(statement, 6));
	strcpy(player->myskills.last_played, sqlite3_column_text(statement, 7));
	player->myskills.total_playtime = sqlite3_column_int(statement, 8);

	player->myskills.playingtime = sqlite3_column_int(statement, 9);

	sqlite3_finalize(statement);

	format = va("SELECT COUNT(*) FROM talents WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//begin talents
	player->myskills.talents.count = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM talents WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	for (i = 0; i < player->myskills.talents.count; ++i)
	{
		//don't crash.
		if (i > MAX_TALENTS)
			return false;

		player->myskills.talents.talent[i].id = sqlite3_column_int(statement, 1);
		player->myskills.talents.talent[i].upgradeLevel = sqlite3_column_int(statement, 2);
		player->myskills.talents.talent[i].maxLevel = sqlite3_column_int(statement, 3);


		if ((r = sqlite3_step(statement)) == SQLITE_DONE)
			break;
	}
	//end talents

	sqlite3_finalize(statement);

	format = va("SELECT COUNT(*) FROM abilities WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//begin abilities
	numAbilities = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM abilities WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	for (i = 0; i < numAbilities; ++i)
	{
		int index;
		index = sqlite3_column_int(statement, 1);

		if ((index >= 0) && (index < MAX_ABILITIES))
		{
			player->myskills.abilities[index].level = sqlite3_column_int(statement, 2);
			player->myskills.abilities[index].max_level = sqlite3_column_int(statement, 3);
			player->myskills.abilities[index].hard_max = sqlite3_column_int(statement, 4);
			player->myskills.abilities[index].modifier = sqlite3_column_int(statement, 5);
			player->myskills.abilities[index].disable = sqlite3_column_int(statement, 6);
			player->myskills.abilities[index].general_skill = (qboolean)sqlite3_column_int(statement, 7);

			if ((r = sqlite3_step(statement)) == SQLITE_DONE)
				break;
		}
		else
		{
			gi.dprintf("Error loading player: %s. Ability index not loaded correctly!\n", player->client->pers.netname);
			vrx_write_to_logfile(player, "ERROR during loading: Ability index not loaded correctly!");
			return false;
		}
	}
	//end abilities

	sqlite3_finalize(statement);

	format = va("SELECT COUNT(*) FROM weapon_meta WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//begin weapons
	numWeapons = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM weapon_meta WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	for (i = 0; i < numWeapons; ++i)
	{
		int index;
		index = sqlite3_column_int(statement, 1);

		if ((index >= 0) && (index < MAX_WEAPONS))
		{
			int j;
			player->myskills.weapons[index].disable = sqlite3_column_int(statement, 2);

			format = va("SELECT * FROM weapon_mods WHERE weapon_index=%d AND char_idx=%d", index, id);

			r = sqlite3_prepare_v2(db, format, strlen(format), &statement_mods, NULL);
			r = sqlite3_step(statement_mods);

			for (j = 0; j < MAX_WEAPONMODS; ++j)
			{

				player->myskills.weapons[index].mods[j].level = sqlite3_column_int(statement_mods, 3);
				player->myskills.weapons[index].mods[j].soft_max = sqlite3_column_int(statement_mods, 4);
				player->myskills.weapons[index].mods[j].hard_max = sqlite3_column_int(statement_mods, 5);

				if ((r = sqlite3_step(statement_mods)) == SQLITE_DONE)
					break;
			}

			sqlite3_finalize(statement_mods);
		}
		else
		{
			gi.dprintf("Error loading player: %s. Weapon index not loaded correctly!\n", player->myskills.player_name);
			vrx_write_to_logfile(player, "ERROR during loading: Weapon index not loaded correctly!");
			return false;
		}

		if ((r = sqlite3_step(statement)) == SQLITE_DONE)
			break;

	}

	sqlite3_finalize(statement);
	//end weapons

	//begin runes

	format = va("SELECT COUNT(*) FROM runes_meta WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	numRunes = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM runes_meta WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	for (i = 0; i < numRunes; ++i)
	{
		int index;
		index = sqlite3_column_int(statement, 1);
		if ((index >= 0) && (index < MAX_VRXITEMS))
		{
			int j;
			player->myskills.items[index].itemtype = sqlite3_column_int(statement, 2);
			player->myskills.items[index].itemLevel = sqlite3_column_int(statement, 3);
			player->myskills.items[index].quantity = sqlite3_column_int(statement, 4);
			player->myskills.items[index].untradeable = sqlite3_column_int(statement, 5);
			strcpy(player->myskills.items[index].id, sqlite3_column_text(statement, 6));
			strcpy(player->myskills.items[index].name, sqlite3_column_text(statement, 7));
			player->myskills.items[index].numMods = sqlite3_column_int(statement, 8);
			player->myskills.items[index].setCode = sqlite3_column_int(statement, 9);
			player->myskills.items[index].classNum = sqlite3_column_int(statement, 10);

			format = va("SELECT * FROM runes_mods WHERE rune_index=%d AND char_idx=%d", index, id);

			r = sqlite3_prepare_v2(db, format, strlen(format), &statement_mods, NULL);
			r = sqlite3_step(statement_mods);

			for (j = 0; j < MAX_VRXITEMMODS; ++j)
			{
				if (vrx_lua_get_int("useMysqlTablesOnSQLite", 0))
				{
					player->myskills.items[index].modifiers[j].type = sqlite3_column_int(statement_mods, 3);
					player->myskills.items[index].modifiers[j].index = sqlite3_column_int(statement_mods, 4);
					player->myskills.items[index].modifiers[j].value = sqlite3_column_int(statement_mods, 5);
					player->myskills.items[index].modifiers[j].set = sqlite3_column_int(statement_mods, 6);
				}
				else
				{
					player->myskills.items[index].modifiers[j].type = sqlite3_column_int(statement_mods, 2);
					player->myskills.items[index].modifiers[j].index = sqlite3_column_int(statement_mods, 3);
					player->myskills.items[index].modifiers[j].value = sqlite3_column_int(statement_mods, 4);
					player->myskills.items[index].modifiers[j].set = sqlite3_column_int(statement_mods, 5);
				}

				if ((r = sqlite3_step(statement_mods)) == SQLITE_DONE)
					break;
			}

			sqlite3_finalize(statement_mods);
		}

		if ((r = sqlite3_step(statement)) == SQLITE_DONE)
			break;
	}

	sqlite3_finalize(statement);
	//end runes


	//*****************************
	//standard stats
	//*****************************

	format = va("SELECT * FROM point_data WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//Exp
	player->myskills.experience = sqlite3_column_int(statement, 1);
	//next_level
	player->myskills.next_level = sqlite3_column_int(statement, 2);
	//Level
	player->myskills.level = sqlite3_column_int(statement, 3);
	//Class number
	player->myskills.class_num = sqlite3_column_int(statement, 4);
	//skill points
	player->myskills.speciality_points = sqlite3_column_int(statement, 5);
	//credits
	player->myskills.credits = sqlite3_column_int(statement, 6);
	//weapon points
	player->myskills.weapon_points = sqlite3_column_int(statement, 7);
	//respawn weapon
	player->myskills.respawn_weapon = sqlite3_column_int(statement, 8);
	//talent points
	player->myskills.talents.talentPoints = sqlite3_column_int(statement, 9);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM character_data WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);


	//*****************************
	//in-game stats
	//*****************************
	//respawns
	player->myskills.weapon_respawns = sqlite3_column_int(statement, 1);
	//health
	player->myskills.current_health = sqlite3_column_int(statement, 2);
	//max health
	player->myskills.max_health = sqlite3_column_int(statement, 3);
	//armour
	player->myskills.current_armor = sqlite3_column_int(statement, 4);
	//max armour
	player->myskills.max_armor = sqlite3_column_int(statement, 5);
	//nerfme			(cursing a player maybe?)
	player->myskills.nerfme = sqlite3_column_int(statement, 6);

	//*****************************
	//flags
	//*****************************
	//admin flag
	player->myskills.administrator = sqlite3_column_int(statement, 7);
	//boss flag
	player->myskills.boss = sqlite3_column_int(statement, 8);

	// prestige
	player->myskills.prestige.total = sqlite3_column_int(statement, 9);
	player->myskills.prestige.points = sqlite3_column_int(statement, 10);

	//*****************************
	//stats
	//*****************************

	sqlite3_finalize(statement);

	format = va("SELECT * FROM game_stats WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//shots fired
	player->myskills.shots = sqlite3_column_int(statement, 1);
	//shots hit
	player->myskills.shots_hit = sqlite3_column_int(statement, 2);
	//frags
	player->myskills.frags = sqlite3_column_int(statement, 3);
	//deaths
	player->myskills.fragged = sqlite3_column_int(statement, 4);
	//number of sprees
	player->myskills.num_sprees = sqlite3_column_int(statement, 5);
	//max spree
	player->myskills.max_streak = sqlite3_column_int(statement, 6);
	//number of wars
	player->myskills.spree_wars = sqlite3_column_int(statement, 7);
	//number of sprees broken
	player->myskills.break_sprees = sqlite3_column_int(statement, 8);
	//number of wars broken
	player->myskills.break_spree_wars = sqlite3_column_int(statement, 9);
	//suicides
	player->myskills.suicides = sqlite3_column_int(statement, 10);
	//teleports			(link this to "use tballself" maybe?)
	player->myskills.teleports = sqlite3_column_int(statement, 11);
	//number of 2fers
	player->myskills.num_2fers = sqlite3_column_int(statement, 12);

	sqlite3_finalize(statement);

	format = va("SELECT * FROM ctf_stats WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	r = sqlite3_step(statement);

	//CTF statistics
	player->myskills.flag_pickups = sqlite3_column_int(statement, 1);
	player->myskills.flag_captures = sqlite3_column_int(statement, 2);
	player->myskills.flag_returns = sqlite3_column_int(statement, 3);
	player->myskills.flag_kills = sqlite3_column_int(statement, 4);
	player->myskills.offense_kills = sqlite3_column_int(statement, 5);
	player->myskills.defense_kills = sqlite3_column_int(statement, 6);
	player->myskills.assists = sqlite3_column_int(statement, 7);
	//End CTF

	sqlite3_finalize(statement);

	// prestige
	format = va("SELECT * FROM prestige WHERE char_idx=%d", id);

	r = sqlite3_prepare_v2(db, format, strlen(format), &statement, NULL);
	while(sqlite3_step(statement) == SQLITE_ROW) {
		int pindex = sqlite3_column_int(statement, 1);
		int param = sqlite3_column_int(statement, 2);
		int level = sqlite3_column_int(statement, 3);

		switch (pindex)
		{
		case PRESTIGE_CREDITS:
			player->myskills.prestige.creditLevel = level;
			break;
		case PRESTIGE_ABILITY_POINTS:
			player->myskills.prestige.abilityPoints = level;
			break;
		case PRESTIGE_WEAPON_POINTS:
			player->myskills.prestige.weaponPoints = level;
			break;
		case PRESTIGE_SOFTMAX_BUMP:
			player->myskills.prestige.softmaxBump[param] = level;
			break;
		case PRESTIGE_CLASS_SKILL:
			player->myskills.prestige.classSkill[param / 32] |= (1 << (param % 32));
			break;
		default: break;
		}
	}



	//Apply runes
	vrx_runes_unapply(player);
	for (i = 0; i < 4; ++i)
		vrx_runes_apply(player, &player->myskills.items[i]);

	//Apply health
	if (player->myskills.current_health > MAX_HEALTH(player))
		player->myskills.current_health = MAX_HEALTH(player);

	//Apply armor
	if (player->myskills.current_armor > MAX_ARMOR(player))
		player->myskills.current_armor = MAX_ARMOR(player);
	player->myskills.inventory[body_armor_index] = player->myskills.current_armor;

	return true;
}

int cdb_get_owner_id (edict_t* ent) {
	char sql[] =
		"select char_idx from userdata "
		"where playername = (select owner from userdata where playername = :1) "
		"or (playername = :1 and email is not null and LENGTH(email) > 0)";

	sqlite3_stmt* stmt;
	int r = sqlite3_prepare_v2(db, sql, sizeof sql, &stmt, NULL);
	CHECK_ERR()
	r = sqlite3_bind_text(stmt, 1, ent->client->pers.netname, strlen(ent->client->pers.netname), SQLITE_STATIC);
	CHECK_ERR()
	r = sqlite3_step(stmt);
	CHECK_ERR()

	int id = -1;
	if (r == SQLITE_ROW)
		id = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);
	return id;
}

qboolean cdb_stash_store(edict_t* ent, int itemindex)
{
	int owner_id = cdb_get_owner_id(ent);
	if (owner_id == -1)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = ent;
		notif->gds_connection_id = ent->gds_connection_id;

		vrx_notify_stash_no_owner(notif);
		return false;
	}

	item_t item;
	V_ItemClear(&item);
	V_ItemSwap(&item, &ent->myskills.items[itemindex]);

	int index = 0;
	{
		sqlite3_stmt* statement;
		int r;
		QUERY_RESULT(va("select stash_index from stash_runes_meta "
			"where char_idx=%d "
			"order by stash_index", owner_id))

		while (r == SQLITE_ROW)
		{
			int index_result = sqlite3_column_int(statement, 0);

			// we found a free slot
			if (index < index_result)
				break;

			// continue looking
			index++;
			r = sqlite3_step(statement);
		}

		sqlite3_finalize(statement);
	}

	QUERY(va("INSERT INTO stash_runes_meta "
		"VALUES (%d,%d,%d,%d,%d,%d,\"%s\",\"%s\",%d,%d,%d)",
		owner_id,
		index,
		item.itemtype,
		item.itemLevel,
		item.quantity,
		item.untradeable,
		item.id,
		item.name,
		item.numMods,
		item.setCode,
		item.classNum))

		for (int j = 0; j < MAX_VRXITEMMODS; ++j)
		{
			// TYPE_NONE, so skip it
			if (item.modifiers[j].type == 0 ||
				item.modifiers[j].value == 0)
				continue;

			QUERY(va("INSERT INTO stash_runes_mods "
				"VALUES (%d,%d,%d,%d,%d,%d,%d)",
				owner_id, index, j,
				item.modifiers[j].type,
				item.modifiers[j].index,
				item.modifiers[j].value,
				item.modifiers[j].set))
		}

	return true;
}

qboolean cdb_stash_get_page(edict_t* ent, int page_index, int items_per_page)
{
	int owner_id = cdb_get_owner_id(ent);
	stash_page_event_t* evt = V_Malloc(sizeof(stash_page_event_t), TAG_GAME);
	evt->gds_connection_id = ent->gds_connection_id;
	evt->gds_owner_id = owner_id;
	evt->ent = ent;
	evt->pagenum = page_index;

	item_t* page = evt->page;
	memset(page, 0, sizeof(item_t) * items_per_page);

	const int start_index = page_index * items_per_page;
	const int end_index = start_index + items_per_page;

	{
		sqlite3_stmt* statement;
		int r;
		QUERY_RESULT(va("select stash_index, itemtype, itemlevel, quantity, untradeable, "
			"id, name, nummods, setcode, classnum "
			"from stash_runes_meta where char_idx=%d "
			"and stash_index >= %d and stash_index <= %d",
			owner_id, start_index, end_index));

		int safeguard_items = 0;
		while (r == SQLITE_ROW && safeguard_items < items_per_page)
		{
			const int page_row_index = sqlite3_column_int(statement, 0) - start_index;
			assert(page_row_index >= 0 && page_row_index < items_per_page);

			item_t* item = &page[page_row_index];

			item->itemtype = sqlite3_column_int(statement, 1);
			item->itemLevel = sqlite3_column_int(statement, 2);
			item->quantity = sqlite3_column_int(statement, 3);
			item->untradeable = sqlite3_column_int(statement, 4);
			strncpy(item->id, sqlite3_column_text(statement, 5), sizeof item->id);
			strncpy(item->name, sqlite3_column_text(statement, 6), sizeof item->name);
			item->numMods = sqlite3_column_int(statement, 7);
			item->setCode = sqlite3_column_int(statement, 8);
			item->classNum = sqlite3_column_int(statement, 9);

			safeguard_items++;
			r = sqlite3_step(statement);
		}

		sqlite3_finalize(statement);
	}

	// we fetch everything in one query.
	{
		sqlite3_stmt* statement;
		int r;

		QUERY_RESULT(va(
			"select stash_index, "
			"type, mod_index, value, [set] "
			"from stash_runes_mods where char_idx = %d "
			"and stash_index >= %d and stash_index <= %d "
			"order by stash_index", 
			owner_id, start_index, end_index
		))

		int last_row = 0;
		int mod_index = 0;
		while (r == SQLITE_ROW && mod_index < MAX_VRXITEMMODS)
		{
			const int page_row_index = sqlite3_column_int(statement, 0) - start_index;
			assert(page_row_index >= 0 && page_row_index < items_per_page);

			if (last_row != page_row_index) {
				mod_index = 0;
				last_row = page_row_index;
			}

			/*const int modn = atoi(item_modifier_row[1]);
			assert(modn >= 0 && modn < MAX_VRXITEMMODS);*/

			imodifier_t* mod = &page[page_row_index].modifiers[mod_index];

			mod->type = sqlite3_column_int(statement, 1);
			mod->index = sqlite3_column_int(statement, 2);
			mod->value = sqlite3_column_int(statement, 3);
			mod->set = sqlite3_column_int(statement, 4);

			r = sqlite3_step(statement);
			mod_index++;
		}

		sqlite3_finalize(statement);
	}

	vrx_notify_open_stash(evt);
	return true;
}

qboolean cdb_stash_open(edict_t* ent)
{
	int owner_id = cdb_get_owner_id(ent);
	if (owner_id == -1)
	{
		stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
		notif->ent = ent;
		notif->gds_connection_id = ent->gds_connection_id;

		vrx_notify_stash_no_owner(notif);
		return false;
	}

	{
		sqlite3_stmt* statement;
		int r;
		
		QUERY_RESULT(va("select lock_char_id from stash where char_idx=%d", owner_id))

		if (r == SQLITE_ROW)
		{
			if (sqlite3_column_type(statement, 0) != SQLITE_NULL)
			{
				stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
				notif->ent = ent;
				notif->gds_connection_id = ent->gds_connection_id;

				vrx_notify_stash_locked(notif);
				sqlite3_finalize(statement);
				return false;
			}
		}

		sqlite3_finalize(statement);
	}

	int id = cdb_get_id(ent->client->pers.netname);
	QUERY(va("update stash set lock_char_id=%d where char_idx=%d", id, owner_id));

	cdb_stash_get_page(ent, 0, sizeof ent->client->stash.page / sizeof(item_t));
	return true;
}

qboolean cdb_stash_close_id(int owner_id)
{
	QUERY(va("update stash set lock_char_id = NULL where char_idx=%d", owner_id));
	return true;
}

qboolean cdb_stash_close(edict_t* ent)
{
	int owner_id = cdb_get_owner_id(ent);
	cdb_stash_close_id(owner_id);
	return true;
}

void cdb_set_owner(edict_t* ent, char* owner_name, char* masterpw)
{
	int new_owner_id = cdb_get_id(owner_name);

	event_owner_error_t* evt = V_Malloc(sizeof(event_owner_error_t), TAG_GAME);
	strcpy(evt->owner_name, owner_name);
	evt->ent = ent;
	evt->connection_id = ent->gds_connection_id;

	if (new_owner_id == -1)
	{
		vrx_notify_owner_nonexistent(evt);
		return;
	}

	int id = cdb_get_id(ent->client->pers.netname);

	sqlite3_stmt* statement;
	int r;
	char sql[] = "update userdata "
		"set owner = ? "
		"from (select 1 as e from userdata o "
		"       where "
		"		o.email = ? and "
		"		o.email is not null and "
		"		o.char_idx=?) as owner "
		"where char_idx = ?";

	r = sqlite3_prepare_v2(db, sql, sizeof sql, &statement, NULL);
	CHECK_ERR();

	sqlite3_bind_text(statement, 1, owner_name, strlen(owner_name), SQLITE_STATIC);
	sqlite3_bind_text(statement, 2, masterpw, strlen(masterpw), SQLITE_STATIC);
	sqlite3_bind_int(statement, 3, new_owner_id);
	sqlite3_bind_int (statement, 4, id);

	sqlite3_step(statement);
	sqlite3_finalize(statement);

	int rows = sqlite3_changes(db);
	if (rows == 0)
		vrx_notify_owner_bad_password(evt);
	else
		vrx_notify_owner_success(evt);

	
}

qboolean cdb_stash_take(edict_t* ent, int stash_index)
{
	int owner_id = cdb_get_owner_id(ent);
	int id = cdb_get_id(ent->client->pers.netname);

	// check if we're the owners of the stash before taking
	{
		sqlite3_stmt* statement;
		int r;
		QUERY_RESULT(va("select char_idx from stash where lock_char_id=%d", id));

		qboolean got_result = r == SQLITE_ROW;
		qboolean someone_else_locked_it = false;

		if (got_result)
			someone_else_locked_it = sqlite3_column_int(statement, 0) != owner_id;

		if (!got_result || someone_else_locked_it)
		{
			stash_event_t* notif = V_Malloc(sizeof(stash_event_t), TAG_GAME);
			notif->ent = ent;
			notif->gds_connection_id = ent->gds_connection_id;

			vrx_notify_stash_locked(notif);
			sqlite3_finalize(statement);
			return false;
		}

		sqlite3_finalize(statement);
	}

	/* take it */
	item_t item;
	V_ItemClear(&item);

	{
		sqlite3_stmt* statement;
		int r;
		QUERY_RESULT(va("select stash_index, itemtype, itemlevel, quantity, untradeable, "
			"id, name, nummods, setcode, classnum "
			"from stash_runes_meta where char_idx=%d "
			"and stash_index = %d",
			owner_id, stash_index));

		if (r == SQLITE_ROW)
		{
			item.itemtype    = sqlite3_column_int(statement, 1);
			item.itemLevel   = sqlite3_column_int(statement, 2);
			item.quantity    = sqlite3_column_int(statement, 3);
			item.untradeable = sqlite3_column_int(statement, 4);
			strncpy(item.id, sqlite3_column_text(statement, 5), sizeof item.id);
			strncpy(item.name, sqlite3_column_text(statement, 6), sizeof item.name);
			item.numMods     = sqlite3_column_int(statement, 7);
			item.setCode     = sqlite3_column_int(statement, 8);
			item.classNum    = sqlite3_column_int(statement, 9);
		}

		sqlite3_finalize(statement);
	}

	{
		sqlite3_stmt* statement;
		int r;
		QUERY_RESULT(va("select type, mod_index, value, [set] "
			"from stash_runes_mods where char_idx = %d "
			"and stash_index = %d", owner_id, stash_index));

		int mod_index = 0;
		while (r == SQLITE_ROW && mod_index < MAX_VRXITEMMODS)
		{
			imodifier_t* mod = &item.modifiers[mod_index];

			mod->type  = sqlite3_column_int(statement, 0);
			mod->index = sqlite3_column_int(statement, 1);
			mod->value = sqlite3_column_int(statement, 2);
			mod->set   = sqlite3_column_int(statement, 3);

			r = sqlite3_step(statement);
			mod_index++;
		}

		sqlite3_finalize(statement);
	}

	QUERY(va("delete from stash_runes_meta where stash_index=%d and char_idx=%d", stash_index, owner_id));
	QUERY(va("delete from stash_runes_mods where stash_index=%d and char_idx=%d", stash_index, owner_id));

	stash_taken_event_t* evt = V_Malloc(sizeof(stash_taken_event_t), TAG_GAME);
	evt->ent = ent;
	evt->gds_connection_id = ent->gds_connection_id;
	V_ItemCopy(&item, &evt->taken);
	strcpy(evt->requester, ent->client->pers.netname);

	vrx_notify_stash_taken(evt);
	return true;
}




// Start Connection to SQLite

void cdb_start_connection()
{
	const char* dbname = vrx_lua_get_string("SQLitePath");
	struct stat mybuf;
	int i;
	char* format;
	sqlite3_stmt* statement;
	qboolean build_db = false;

	if (db)
		return;

	if (!dbname)
		dbname = DEFAULT_PATH;

	if (stat(dbname, &mybuf))
	{
		build_db = true;
	}

	sqlite3_open(dbname, &db);

	if (build_db)
	{
		for (i = 0; i < TOTAL_TABLES; i++)
		{
			QUERY(va(VSFU_CREATEDBQUERY[i]))
		}
	}

}

void cdb_end_connection()
{
	sqlite3_close(db);
	db = NULL;
}
