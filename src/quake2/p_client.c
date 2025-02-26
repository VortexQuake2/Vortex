#include "g_local.h"
#include "../gamemodes/ctf.h"
#include "monsterframes/m_player.h"
#include "../combat/abilities/jump.h"
#include "../combat/abilities/v_think.h"
#include "../gamemodes/boss.h"
#include "characters/class_limits.h"
#include "characters/io/v_characterio.h"

//Function prototypes required for this .c file:
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void SP_misc_teleporter_dest (edict_t *ent);
void EatCorpses (edict_t *ent);
void RunCacodemonFrames (edict_t *ent, usercmd_t *ucmd);

//
// Gross, ugly, disgustuing hack section
//

// this function is an ugly as hell hack to fix some map flaws
//
// the coop spawn spots on some maps are SNAFU.  There are coop spots
// with the wrong targetname as well as spots with no name at all
//
// we use carnal knowledge of the maps to fix the coop spot targetnames to match
// that of the nearest named single player spot

static void SP_FixCoopSpots (edict_t *self)
{
	edict_t	*spot;
	vec3_t	d;

	spot = NULL;

	while(1)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_start");
		if (!spot)
			return;
		if (!spot->targetname)
			continue;
		VectorSubtract(self->s.origin, spot->s.origin, d);
		if (VectorLength(d) < 384)
		{
			if ((!self->targetname) || Q_stricmp(self->targetname, spot->targetname) != 0)
			{
//				gi.dprintf("FixCoopSpots changed %s at %s targetname from %s to %s\n", self->classname, vtos(self->s.origin), self->targetname, spot->targetname);
				self->targetname = spot->targetname;
			}
			return;
		}
	}
}

// now if that one wasn't ugly enough for you then try this one on for size
// some maps don't have any coop spots at all, so we need to create them
// where they should have been

static void SP_CreateCoopSpots (edict_t *self)
{
	edict_t	*spot;

	if(Q_stricmp(level.mapname, "security") == 0)
	{
		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 - 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 128;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		return;
	}
}


/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	if (!coop->value)
		return;
	if(Q_stricmp(level.mapname, "security") == 0)
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_CreateCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/

qboolean CTF_CorrectSpawnPosition(edict_t* self);

void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (CTF_CorrectSpawnPosition(self))
		SP_misc_teleporter_dest (self);
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/

void SP_info_player_coop(edict_t *self)
{
	if (!coop->value)
	{
		G_FreeEdict (self);
		return;
	}

	if((Q_stricmp(level.mapname, "jail2") == 0)   ||
	   (Q_stricmp(level.mapname, "jail4") == 0)   ||
	   (Q_stricmp(level.mapname, "mine1") == 0)   ||
	   (Q_stricmp(level.mapname, "mine2") == 0)   ||
	   (Q_stricmp(level.mapname, "mine3") == 0)   ||
	   (Q_stricmp(level.mapname, "mine4") == 0)   ||
	   (Q_stricmp(level.mapname, "lab") == 0)     ||
	   (Q_stricmp(level.mapname, "boss1") == 0)   ||
	   (Q_stricmp(level.mapname, "fact3") == 0)   ||
	   (Q_stricmp(level.mapname, "biggun") == 0)  ||
	   (Q_stricmp(level.mapname, "space") == 0)   ||
	   (Q_stricmp(level.mapname, "command") == 0) ||
	   (Q_stricmp(level.mapname, "power2") == 0) ||
	   (Q_stricmp(level.mapname, "strike") == 0))
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_FixCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}


/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	BOT_DMclass_Pain(self, other, kick, damage);//GHz
	// player pain is handled at the end of the frame in P_DamageFeedback
}


qboolean IsFemale (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}

int GetGender(edict_t *ent) {
	char *info;

	if ( !ent->client )
		return GENDER_NEUTRAL;
	
	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if ( !strncmp( info, "fluid", 6 ) ) {
		return GetRandom( GENDER_NEUTRAL, GENDER_MAX -1 );
	} else if ( info[0] == 'f' || info[0] == 'F' || info[0] == 's' || info[0] == 'S') {
		return GENDER_FEMALE;
	} else if ( info[0] == 'm' || info[0] == 'M' || info[0] == 'h' || info[0] == 'H' ) {
		return GENDER_MALE;
	} else if ( info[0] == 'o' || info[0] == 'O' || info[0] == 'i' || info[0] == 'I' ) { 
		return GENDER_OBJECT;
	} else if ( info[0] == 'c' || info[0] == 'C' ) { 
		return GENDER_CUSTOM;
	} else {
		return GENDER_NEUTRAL;
	}
}

char *GetPossesiveAdjective(edict_t *ent) {
	int gender = GetGender(ent);
	char *info;

	switch( gender ) {
		case GENDER_MALE:
			return "his";
		case GENDER_FEMALE:
			return "her";
		case GENDER_OBJECT:
			return "its";
		case GENDER_CUSTOM:
			info = Info_ValueForKey(ent->client->pers.userinfo, "gender_pa" );
			if ( strncmp( info, "", 2 ) )
				return info;
		case GENDER_NEUTRAL:
		default:
			return "their";
	}
}

char *GetReflexivePronoun(edict_t *ent) {
	int gender = GetGender(ent);
	char *info;

	switch( gender ) {
		case GENDER_MALE:
			return "himself";
		case GENDER_FEMALE:
			return "herself";
		case GENDER_OBJECT:
			return "itself";
		case GENDER_CUSTOM:
			info = Info_ValueForKey(ent->client->pers.userinfo, "gender_rp" );
			if ( strncmp( info, "", 2 ) )
				return info;
		case GENDER_NEUTRAL:
		default:
			return "theirself";
	}
}

qboolean MonsterObits (edict_t *player, edict_t *monster)
{
	edict_t *monster_owner;

	//Spirits will fail G_EntExists(), so check for them first
	if(monster->mtype != M_YANGSPIRIT 
		&& monster->mtype != M_BALANCESPIRIT
		&& monster->mtype != M_YINSPIRIT)
	{
		//Check the normal monsters (now that we know they are not spirits)
		if (!G_EntExists(player) ||  !G_EntExists(monster))
			return false;
	}

	// monsters are not clients
	if (monster->client)
		return false;
	
	// monster must have a type
	if (!monster->mtype)
		return false;
	
	// monster must have a level
	if (monster->monsterinfo.level < 1)
		return false;

	if (monster->activator == player || monster->owner == player)
	{
		gi.bprintf(PRINT_MEDIUM, "%s suicides.\n", player->client->pers.netname);
		return true;
	}

	// is this monster owned by another player?
	if (!IsABoss(monster) && ((monster_owner = G_GetClient(monster)) != NULL))
	{
		gi.bprintf(PRINT_MEDIUM, "%s was killed by %s's %s\n", 
			player->client->pers.netname, monster_owner->client->pers.netname, 
			GetMonsterKindString(monster->mtype));
	}
	else
	{
		// this is a world-spawned monster
		gi.bprintf(PRINT_MEDIUM, "%s was killed by a level %d %s\n", player->client->pers.netname, 
			monster->monsterinfo.level, V_GetMonsterName(monster));
	}

	return true;
}


void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int			mod;
	char 		cmessage[64];
	char		*message;
	char		*message2;
	qboolean	ff;

	//JABot [start]
	//	if (self->ai.is_bot){
	//		AI_BotObituary (self, inflictor, attacker);
	//		return;
	//	} //[end]

	//K03 Begin
	if (attacker && (attacker->creator) &&(!attacker->client) && (meansOfDeath == MOD_SENTRY || meansOfDeath == MOD_SENTRY_ROCKET || meansOfDeath == MOD_SENTRY_BEAM))
		attacker = attacker->creator;
	// fire totem
	if ((attacker->mtype == TOTEM_FIRE) && attacker->owner && attacker->owner->inuse)
		attacker = attacker->owner;
	// Is this a monster doing the killing??
	//if (Monster_Obits(self, attacker))
	if (PM_MonsterHasPilot(attacker))
		attacker = attacker->activator;

	if (MonsterObits(self, attacker))
		return;

	//K03 End

	if (coop->value && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	if (deathmatch->value || coop->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides :)";
			break;
		case MOD_FALLING:
			message = "made a leap of faith";
			break;
		case MOD_CRUSH:
			message = "suffers from claustrophobia";
			break;
		case MOD_WATER:
			message = "forgot to breath";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "joins the lava gods";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			break;
		// RAFAEL
		case MOD_GEKK:
		case MOD_BRAINTENTACLE:
			snprintf( cmessage, 64, "got %s brains sucked out", GetPossesiveAdjective( self ) );
			message = cmessage;
			break;
		}
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_SHRAPNEL:
				message = "received a fatal piercing";
				break;
			case MOD_HELD_GRENADE:
				message = "tried to put the pin back in";
				break;
			case MOD_HG_SPLASH:
			case MOD_G_SPLASH:
				snprintf( cmessage, 64, "hugs %s grenade", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_R_SPLASH:
				snprintf( cmessage, 64, "hates %s", GetReflexivePronoun( self ) );
				message = cmessage;
				break;
			case MOD_BFG_BLAST:
				message = "should have used a smaller gun";
				break;
			case MOD_CORPSEEXPLODE:
				snprintf( cmessage, 64, "hugs %s exploding corpse", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_BOMBS:
				snprintf( cmessage, 64, "plays catch with %s bombs", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_DECOY:
				snprintf( cmessage, 64, "hates %s decoy", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			// RAFAEL 03-MAY-98
			case MOD_TRAP:
			 	snprintf( cmessage, 64, "was sucked into %s own trap", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_SUPPLYSTATION:
				snprintf( cmessage, 64, "took a seat on %s exploding supply station", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_CACODEMON_FIREBALL:
				snprintf( cmessage, 64, "juggles %s exploding skulls", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_EXPLODINGARMOR:
				snprintf( cmessage, 64, "shows off %s exploding armor", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_PROXY:
				snprintf( cmessage, 64, "stood too close to %s proxy grenade", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_METEOR:
				snprintf( cmessage, 64, "pitched a tent underneath %s falling meteor", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_NAPALM:
				snprintf( cmessage, 64, "makes a thorough examination of %s napalm", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_EMP:
				snprintf( cmessage, 64, "detonates %s ammo", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_SPIKEGRENADE:
				snprintf( cmessage, 64, "eats %s spike grenade", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_FIREBALL:
				snprintf( cmessage, 64, "rides %s fireball", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_ICEBOLT:
				message = "becomes a popsicle";
				break;
			case MOD_PLASMABOLT:
				message = "turns to dust";
				break;
			case MOD_LIGHTNING_STORM:
				message = "receives the death penalty";
				break;
			case MOD_MIRV:
				snprintf( cmessage, 64, "eats %s mirv grenade", GetPossesiveAdjective( self ) );
				message = cmessage;
				break;
			case MOD_SELFDESTRUCT:
				message = "goes off with a blast";
				break;
			default:
				message = "becomes bored with life";
				break;
			}
		}
		if (message)
		{
			gi.bprintf (PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			if (deathmatch->value)
				self->client->resp.score--;
		//	self->enemy = NULL;
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client && invasion->value < 2)
		{
			switch (mod)
			{
			case MOD_BLASTER:
				message = "was humiliated by";
				break;
			case MOD_SHOTGUN:
				message = "'s face was impacted by";
				message2 = "'s shotgun";
				break;
			case MOD_SSHOTGUN:
				message = "was blown to pieces by";
				message2 = "'s super shotgun";
				break;
			case MOD_MACHINEGUN:
				message = "was shredded by";
				message2 = "'s machinegun";
				break;
			case MOD_CHAINGUN:
				message = "'s torso was removed by";
				message2 = "'s chaingun";
				break;
			case MOD_GRENADE:
				message = "was gibbed by";
				message2 = "'s grenade";
				break;
			case MOD_G_SPLASH:
				message = "was splattered by";
				message2 = "'s shrapnel";
				break;
			case MOD_ROCKET:
				message = "rides";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "was reamed by";
				message2 = "'s rocket";
				break;
			case MOD_HYPERBLASTER:
				message = "was liquidated by";
				message2 = "'s hyperblaster";
				break;
			case MOD_RAILGUN:
				message = "was railed by";
				break;
			case MOD_SNIPER:
				message = "receives a sucking chest wound from";
				break;
			case MOD_BFG_LASER:
				message = "turned inside out from";
				message2 = "'s BFG";
				break;
			case MOD_BFG_BLAST:
				message = "was disintegrated by";
				message2 = "'s BFG blast";
				break;
			case MOD_BFG_EFFECT:
				message = "couldn't hide from";
				message2 = "'s BFG";
				break;
			case MOD_HANDGRENADE:
				message = "tries to hatch";
				message2 = "'s handgrenade";
				break;
			case MOD_HG_SPLASH:
				message = "was split in half by";
				message2 = "'s handgrenade";
				break;
			case MOD_HELD_GRENADE:
				message = "feels";
				message2 = "'s pain";
				break;
			case MOD_SELFDESTRUCT:
				message = "got bombed by";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
			
				break;
//ZOID
			case MOD_GRAPPLE:
				message = "was caught by";
				message2 = "'s grapple";
				break;
//ZOID
// RAFAEL 14-APR-98
			case MOD_RIPPER:
				message = "ripped to shreds by";
				message2 = "'s ripper gun";
				break;
			case MOD_PHALANX:
				message = "was evaporated by";
				break;
			case MOD_TRAP:
				message = "caught in trap by";
				break;
// END 14-APR-98	
			case MOD_BURN:
				message = "was burned alive by";
				break;
			case MOD_SWORD:
				message = "was eviscerated by";
				message2 ="'s sword";
				break;
			case MOD_LASER_DEFENSE:
				message = "saw";
				message2 = "'s light";
				break;
			case MOD_SENTRY:
				message = "'s spine was extracted by";
				message2 = "'s Sentry Gun";
				break;
			case MOD_SENTRY_ROCKET:
				message = "hates";
				message2 = "'s Sentry Gun";
				break;
			case MOD_SENTRY_BEAM:
				message = "is disintegrated by";
				message2 = "'s Sentry Gun";
				break;
			case MOD_CORPSEEXPLODE:
				message = "stood to close to";
				message2 = "'s corpse";
				break;
			case MOD_BOMBS:
				message = "happily collects all of";
				message2 = "'s exploding bombs";
				break;
			case MOD_DECOY:
				message = "gets too friendly with";
				message2 = "'s decoy";
				break;
			case MOD_CANNON:
				message = "'s ribs were shattered by";
				message2 = "'s 20mm cannon";
				break;
			case MOD_PARASITE:
				message = "tastes finger lickin' good to";
				break;
			case MOD_SUPPLYSTATION:
				message = "is blown to bits by";
				message2 = "'s supply station";
				break;
			case MOD_CACODEMON_FIREBALL:
				message = "succumbs to";
				message2 = "'s fiery skull";
				break;
			case MOD_PLAGUE:
				message = "dies from";
				message2 = "'s social disease";
				break;
			case MOD_SKULL:
				message = "is executed by";
				message2 = "'s hell spawn";
				break;
			case MOD_CORPSEEATER:
				message = "becomes";
				message2 = "'s human dessert";
				break;
			case MOD_TENTACLE:
				message = "becomes";
				message2 = "'s tasty treat";
				break;
			case MOD_BEAM:
				message = "gets cooked by";
				message2 = "'s laser beam";
				break;
			case MOD_MAGICBOLT:
				message = "catches";
				message2 = "'s magic bolt";
				break;
			case MOD_CALTROPS:
				message = "is impaled by";
				message2 = "'s caltrops";
				break;
			case MOD_CRIPPLE:
				message = "'s heart skips a beat thanks to";
				message2 = "'s cripple spell";
				break;
			case MOD_NOVA:
				message = "is blown away by";
				message2 = "'s nova spell";
				break;
			case MOD_EXPLODINGARMOR:
				message = "adorns";
				message2 = "'s exploding armor";
				break;
			case MOD_MUTANT:
				message = "was torn to pieces by";
				break;
			case MOD_SPIKE:
				message = "was poked to death by";
				message2 = "'s spike";
				break;
			case MOD_SPIKEGRENADE:
				message = "got cozy with";
				message2 = "'s spike grenade";
				break;
			case MOD_MINDABSORB:
				message = "'s brain was fried by";
				message2 = "'s mind absorb spell";
				break;
			case MOD_LIFE_DRAIN:
				message = "'s soul was taken by";
				break;
			case MOD_PROXY:
				message = "got a closer look of";
				message2 = "'s exploding proxy grenade";
				break;
			case MOD_METEOR:
				message = "was crushed under";
				message2 = "'s meteorite";
				break;
			case MOD_NAPALM:
				message = "enjoys";
				message2 = "'s toasty napalm";
				break;
			case MOD_TANK_PUNCH:
				message = "was pulverized by";
				break;
			case MOD_TANK_BULLET:
				message = "was perforated by";
				break;
			case MOD_FREEZE:
				message = "was frozen solid by";
				break;
			case MOD_LIGHTNING:
				message = "was electrocuted by";
				break;
			case MOD_AUTOCANNON:
				message = "receives a lead enema from";
				message2 = "'s autocannon";
				break;
			case MOD_HAMMER:
				message = "was smashed by";
				message2 = "'s blessed hammer";
				break;
			case MOD_BLACK_HOLE:
				message = "was crushed by the gravitational forces of";
				message2 = "'s black hole";
				break;
			case MOD_BERSERK_PUNCH:
				message = "was knocked out by";
				break;
			case MOD_BERSERK_CRUSH:
				message = "was pulverized by";
				break;
			case MOD_BERSERK_SLASH:
				message = "was ripped apart by";
				break;
			case MOD_BLEED:
				message = "donates blood to";
				break;
			case MOD_EMP:
				message = "'s ammo detonates thanks to";
				message2 = "'s EMP grenade";
				break;
			case MOD_FIREBALL:
				message = "caught";
				message2 = "'s fireball";
				break;
			case MOD_ICEBOLT:
				message = "became";
				message2 = "'s ice sculpture";
				break;
			case MOD_PLASMABOLT:
				message = "was disintegrated by";
				message2 = "'s plasma bolt";
				break;
			case MOD_LIGHTNING_STORM:
				message = "was electrocuted by";
				message2 = "'s lightning storm";
				break;
			case MOD_MIRV:
				message = "was blown away by";
				message2 = "'s mirv grenade";
				break;
			case MOD_OBSTACLE:
				message = "was impaled by";
				message2 = "'s obstacle";
				break;
			case MOD_GAS:
				message = "visits";
				message2 = "'s gas chamber";
				break;
			case MOD_ACID:
				message = "took a bath in";
				message2 = "'s acid";
				break;
			case MOD_UNHOLYGROUND:
				message = "was sacrificed by";
				message2 = "'s unholy ground";
				break;
			case MOD_EXPLODING_BARREL:
				message = "closely inspects";
				message2 = "'s exploding barrel";
				break;
			case MOD_SHRAPNEL:
				message = "adorns";
				message2 = "'s burning shrapnel";
				break;
				//K03 End
			}
			if (message)
			{
				gi.bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				if (deathmatch->value)
				{
					if (ff)
						attacker->client->resp.score--;
					else
						attacker->client->resp.score++;
				}
				return;
			}
		}
	}

	gi.bprintf (PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);
	if (deathmatch->value)
		self->client->resp.score--;
}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	// RAFAEL
	qboolean	quadfire;
	float		dist;
	vec3_t		v;
	edict_t		*enemy = NULL;
	float		spread;

	if(self->enemy && self->enemy != self)
	{
		if(self->enemy->classname[0] == 'p')
		{
			
			VectorSubtract(self->s.origin,self->enemy->s.origin,v);
			dist = VectorLength(v);
			if(dist < 200) enemy = self->enemy;
		}
	}

	if (!deathmatch->value)
		return;

	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + qf2sf(10)));

	// RAFAEL
	if (!((int)(dmflags->value) & DF_QUADFIRE_DROP))
		quadfire = false;
	else
		quadfire = (self->client->quadfire_framenum > (level.framenum + qf2sf(10)));
	

	if (item && quad)
		spread = 22.5;
	else if (item && quadfire)
		spread = 12.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
		self->client->pers.inventory[ITEM_INDEX(item)] = 0;
		self->client->newweapon = FindItem ("Blaster");
		self->client->weaponstate = WEAPON_ACTIVATING;
		self->client->ps.gunframe = 0;
		ChangeWeapon (self);
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
	// RAFAEL
	if (quadfire)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quadfire"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quadfire_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
}

/*
==================
player_die
==================
*/


void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;
	qboolean dogrenade = false;
//GHz START
	if (debuginfo->value > 1)
		gi.dprintf("player_die()\n");
	
	vrx_player_death(self, attacker, inflictor);
//GHz END
	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;
	self->s.modelindex2 = 0; // remove linked weapon model
	self->s.modelindex3 = 0;
	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	self->s.sound = 0;
	self->client->weapon_sound = 0;
//GHz START
	if (self->mtype != M_MYPARASITE) // parasite's bbox is already short enough
		self->maxs[2] = -8;
//GHz END
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag) // called only once at player death
	{
		int talentLevel;
//GHz START
		if (ctf->value)
			CTF_PlayerRespawnTime(self);
		else
//GHz END
			self->client->respawn_time = level.time + 1.0;

		LookAtKiller (self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);
		//TossClientWeapon (self);
		if (deathmatch->value && !self->ai.is_bot) // GHz: don't send unicast packets to bots!
			Cmd_Help_f (self);		// show scores
		// clear inventory
		memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

		if ( self->client->pers.weapon == Fdi_GRENADES ) {
			if ( self->client->weaponstate == WEAPON_FIRING ) {
				dogrenade = true;
			}
		}

		if (!self->exploded)
		{
            talentLevel = vrx_get_talent_level(self, TALENT_MARTYR);

				if (talentLevel > 0) // Martyr
				{
					// do the damage
					self->exploded = true; // make sure we don't do it more than once
					T_RadiusDamage(self, self, SELFDESTRUCT_BONUS*0.7 * talentLevel, self, SELFDESTRUCT_RADIUS*0.6 * talentLevel, MOD_SELFDESTRUCT);

					// GO BOOM!
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_EXPLOSION1);
					gi.WritePosition (self->s.origin);
					gi.multicast (self->s.origin, MULTICAST_PVS);
				}
		}
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~FL_POWER_ARMOR;
	// RAFAEL
	self->client->quadfire_framenum = 0;
//GHz START
	if (self->health < self->gib_health)
	{	// gib
        V_GibSound(self, GetRandom(1, 6));
//GHz END
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			ThrowClientHead(self, damage);
		}
		else
			self->svflags |= SVF_NOCLIENT;//GHz: don't waste client bandwidth!
//ZOID
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = 0;
//ZOID.
		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1-1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death101-1;
				self->client->anim_end = FRAME_death106;
				break;
			case 1:
				self->s.frame = FRAME_death201-1;
				self->client->anim_end = FRAME_death206;
				break;
			case 2:
				self->s.frame = FRAME_death301-1;
				self->client->anim_end = FRAME_death308;
				break;
			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (randomMT()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

	self->deadflag = DEAD_DEAD;


	if ( dogrenade ) {
		weapon_grenade_fire(self, false);
	}
	gi.linkentity (self);
}

//=======================================================================

void vrx_add_basic_weapons(gclient_t *client, gitem_t *item, int spectator);

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client)
{
	//K03 Begin
	gitem_t		*item;
	
	int spectator=client->pers.spectator;

	if (debuginfo->value > 1)
		gi.dprintf("InitClientPersistant()\n");
	
	//K03 End

	memset (&client->pers, 0, sizeof(client->pers));


	item = FindItem("Blaster");
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[ITEM_INDEX(item)] = 1;

    //K03 Begin
    vrx_add_basic_weapons(client, item, spectator);

	//K03 End

	client->pers.health			= 100;
	client->pers.max_health		= 100;

	client->pers.max_bullets	= 200;
	client->pers.max_shells		= 100;
	client->pers.max_rockets	= 50;
	client->pers.max_grenades	= 50;
	client->pers.max_cells		= 200;
	client->pers.max_slugs		= 50;

	// RAFAEL
	client->pers.max_magslug	= 50;
	client->pers.max_trap		= 5;

	//K03 Begin
	client->pers.max_powercubes = 200;
	client->pers.max_tballs = 20;
	//K03 End

	client->pers.connected = true;
	ClearScanner(client);
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;

	// per-wave exp tracking
	client->resp.wave_solo_targets = 0;
	client->resp.wave_solo_dmgmod = 0;
	client->resp.wave_solo_exp = 0;
	client->resp.wave_solo_credits = 0;
	client->resp.wave_shared_exp = 0;
	client->resp.wave_shared_credits = 0;
	client->resp.wave_assist_exp = 0;
	client->resp.wave_assist_credits = 0;
}

/*
==================
SaveClientData

Some information that should be persistant, like health, 
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.powerArmorActive = (ent->flags & FL_POWER_ARMOR);
		if (coop->value)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	if (ent->client->pers.powerArmorActive)
		ent->flags |= FL_POWER_ARMOR;
	if (coop->value)
		ent->client->resp.score = ent->client->pers.score;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float	PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint (edict_t *ent)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	//gi.dprintf("SelectRandomDeathmatchSpawnPoint()\n");

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		if (ctf->value && ent && spot->teamnum && spot->teamnum != ent->teamnum)
			continue;

		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = count ? randomMT() % count : 0;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint (edict_t *ent)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;

	//gi.dprintf("SelectFarthestDeathmatchSpawnPoint()\n");

	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		// in CTF, don't use spawns without matching teamnum, unless they are unclaimed
		//FIXME: ent can be NULL, but for some reason we still crash here :(
		if (ctf->value && ent && ent->inuse && ent->teamnum && spot->teamnum && spot->teamnum != ent->teamnum)
			continue;

		bestplayerdistance = PlayersRangeFromSpot (spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	if (ctf->value)
		return NULL; // find somewhere else to spawn!

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

edict_t *SelectDeathmatchSpawnPoint (edict_t *ent)
{
	if ( (int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint (ent);
	else
		return SelectRandomDeathmatchSpawnPoint (ent);
}


edict_t *SelectCoopSpawnPoint (edict_t *ent)
{
	int		index;
	edict_t	*spot = NULL;
	char	*target;

	index = ent->client - game.clients;

	// player 0 starts in normal player spawn point
	if (!index)
		return NULL;

	spot = NULL;

	// assume there are four coop spots at each spawnpoint
	while (1)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_coop");
		if (!spot)
			return NULL;	// we didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";
		if ( Q_stricmp(game.spawnpoint, target) == 0 )
		{	// this is a coop spawn point for one of the clients here
			index--;
			if (!index)
				return spot;		// this is it
		}
	}


	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
qboolean SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t	*spot = NULL;
	qboolean failed=false;

//GHz START
	if (ctf->value)
	{
		if ((spot = CTF_SelectSpawnPoint(ent)) == NULL)
		{
			edict_t *base = CTF_GetFlagBaseEnt(ent->teamnum);

			//gi.dprintf("CTF_SelectSpawnPoint() couldn't find a player spawn\n");

			if (base && !TeleportNearArea(ent, base->s.origin, 512, false))
				failed = true;
			else if (!base && !vrx_find_random_spawn_point(ent, false))
				failed = true;
			else
			{
				//gi.dprintf("successfully teleported\n");
				VectorCopy(ent->s.angles, angles);
				VectorCopy(ent->s.origin, origin);
				VectorCopy(vec3_origin, ent->s.origin); // to avoid killbox() fragging us
				gi.linkentity(ent);
				origin[2] += 9;
				return true;
			}

			if (failed)
				gi.dprintf("CTF failed to find a valid spawn point for player\n");
		}
	}
			
	// if we're in invasion mode, then only use invasion spawns
	else if (INVASION_OTHERSPAWNS_REMOVED)
	{
		// if a spot is not found, they'll have to wait until a spawn is free
		if ((spot = vrx_inv_select_player_spawn_point(ent)) != NULL)
		{
			vrx_inv_remove_spawn_que(ent); // remove from waiting list
			ent->spawn = NULL; // player is no longer assigned this spawn

            VectorCopy (spot->s.angles, angles);
			VectorCopy (spot->s.origin, origin);
			origin[2] += 9;

			return true;
		}
		return false;
	}
//GHz END
	// az begin
	else if (tbi->value)
	{
		spot = TBI_FindSpawn(ent);

		if (!spot)
		{
			if (vrx_find_random_spawn_point(ent, true))
			{
				VectorCopy(ent->s.angles, angles);
				VectorCopy(ent->s.origin, origin);
				VectorCopy(vec3_origin, ent->s.origin); // to avoid killbox() fragging us
				gi.linkentity(ent);
				origin[2] += 9;
				return true;
			}
        }
	}
	// az end
	else if (deathmatch->value)
		spot = SelectDeathmatchSpawnPoint (ent);
	else if (coop->value)
		spot = SelectCoopSpawnPoint (ent);

	// find a single player start spot
	if (!spot)
	{
		while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL)
		{
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}

		if (!spot)
		{
			if (!game.spawnpoint[0])
			{	// there wasn't a spawnpoint without a target, so use any
				spot = G_Find (spot, FOFS(classname), "info_player_start");
			}
			if (!spot)
				gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);
	return true;
}

//======================================================================


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}


void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;

	if (self->health < self->gib_health) //GHz changed from -40
	{
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			self->s.origin[2] -= 48;
			ThrowClientHead(self, damage);
		}
		else
			self->svflags |= SVF_NOCLIENT;//GHz: don't waste client bandwidth!
		self->s.frame = 0;
		self->takedamage = DAMAGE_NO; //3.0 gibbed corpses should not be solid! ;)
		self->solid = SOLID_NOT;
	}
}

void body_think (edict_t *self)
{
	/*
	if (level.time > self->delay)
	{
		gi.dprintf("hide the body!\n");
		self->svflags |= SVF_NOCLIENT;
		gi.unlinkentity(self);
		return;
	}*/

	self->s.effects = 0;
	self->s.renderfx = 0;

	// plague flies
	if (que_typeexists(self->curses, CURSE_PLAGUE))
		self->s.effects |= EF_FLIES;

	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}

	if (self->groundentity)
	{
		self->velocity[0] *= 0.6;
		self->velocity[1] *= 0.6;
	}

	self->nextthink = level.time+FRAMETIME;
}

void CopyToBodyQue (edict_t *ent)
{
	que_t		*slot = NULL;
	edict_t		*body;

	//gi.dprintf("CopyToBodyQue\n");
	//gi.dprintf("effects %d damage %d movetype %d frame %d\n", ent->s.effects, ent->takedamage, ent->movetype, ent->s.frame);
	//if (ent->solid == SOLID_NOT)
	//	gi.dprintf("copytobodyque called on nonsolid\n");

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// is the player cursed with plague?
	if ((slot = que_findtype(ent->curses, slot, CURSE_PLAGUE)) != NULL)
	{
		// try to add it to the body's curse list
		if (!que_addent(body->curses, slot->ent, 999.0))
		{
			// failed, remove the curse entity
			G_FreeEdict(slot->ent);
		}
		else
			slot->ent->enemy = body; // make the plague entity target the body
		
		// remove plague from the player's curses
		slot->ent = NULL;
		slot->time = 0;
	}
	// FIXME: send an effect on the removed body

	gi.unlinkentity (ent);
	gi.unlinkentity (body);

	body->s = ent->s;
//GHz START
	body->s.effects = 0;
	body->s.renderfx = 0;
//GHz END
	body->s.number = body - g_edicts;
	body->svflags = ent->svflags;
	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;
	body->die = body_die;
	//body->touch = body_touch;
	body->takedamage = ent->takedamage;//DAMAGE_YES;
//GHz START
	body->think = body_think;
	body->nextthink = level.time + FRAMETIME;
	//body->delay = level.time + GetRandom(5, 10);// hide the body after some time
	body->deadflag = DEAD_DEAD; // added so its easier to identify a body
	body->gib_health = ent->gib_health; // necessary for corpse explosion and yin spirit
	body->max_health = ent->max_health;//4.55
//GHz END

	gi.linkentity (body);
}

void respawn (edict_t *self)
{
	if (debuginfo->value > 1)
		gi.dprintf("respawn()\n");
	if (deathmatch->value || coop->value)
	{
		// don't let them respawn unless they've been assigned a spawn
		if (INVASION_OTHERSPAWNS_REMOVED)
		{
			if (!self->spawn)
			{
				vrx_inv_add_spawn_que(self);
				return;
			}
		}

		//JABot[start]
		if (self->ai.is_bot) {
			BOT_Respawn(self);
			return;
		}
		//JABot[end]

		vrx_print_newbie_tip(self);

		// spectator's don't leave bodies
		if (self->movetype != MOVETYPE_NOCLIP)
			CopyToBodyQue (self);
		self->mtype = 0;
		self->svflags &= ~SVF_NOCLIENT;
		
		PutClientInServer (self);

		// add a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		//3.0 You get invincibility when you spawn, but you can't shoot
		self->client->respawn_time = self->client->ability_delay = level.time + (RESPAWN_INVIN_TIME / 10);
		self->client->invincible_framenum = level.framenum + qf2sf(RESPAWN_INVIN_TIME);
		return;
	}

	// restart the entire server
	gi.AddCommandString ("menu_loadgame\n");
}

/* 
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn (edict_t *ent)
{
	int i, numspec;

	if (debuginfo->value > 1)
		gi.dprintf("specatator_respawn()\n");
	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator) {
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "spectator");
		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value)) {
			safe_cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			safe_cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset their spectator var
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	} else {
		// he was a spectator and wants to join the game
		// he must have the right password
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "password");
		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value)) {
			safe_cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent->client->resp.score = ent->client->pers.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator)  {
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator) 
		gi.bprintf (PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	else
		gi.bprintf (PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}

//==============================================================

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
// ### Hentai ### BEGIN
void ShowGun(edict_t *ent);
// ###	Hentai ### END

//az begin
void KillBoxMonsters(edict_t *ent);
// az end

void PutClientInServer (edict_t *ent)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;

	if (debuginfo->value > 1)
		gi.dprintf("PutClientInServer()\n");

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if (!SelectSpawnPoint (ent, spawn_origin, spawn_angles))
		return;

	index = ent-g_edicts-1;
	client = ent->client;

	// deathmatch wipes most client data every spawn
	if (deathmatch->value || coop->value)
	{
		char		userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		ClientUserinfoChanged (ent, userinfo);
	}
	else
	{
		memset (&resp, 0, sizeof(resp));
	}
	
	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	if (client->pers.health <= 0)
		InitClientPersistant(client);
	client->resp = resp;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->owner = NULL; //GHz
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->flags &= ~FL_CHATPROTECT;//GHz
	ent->svflags &= ~SVF_DEADMONSTER;
	ent->svflags &= ~SVF_MONSTER;
	ent->lastkill = 0;//GHz
	ent->nfer = 0;
	ent->exploded = false;
	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);

	//K03 Begin
	if (ent->myskills.class_num > 0)
	{
		int talentLevel;

		vrx_update_all_character_maximums(ent);
		vrx_add_respawn_weapon(ent, ent->myskills.respawn_weapon);
		vrx_add_respawn_items(ent);

		//Talent: Sidearms
		talentLevel = vrx_get_talent_level(ent, TALENT_SIDEARMS);
		if (talentLevel > 0)
		{
			int i;

			//Give the player one additional respawn weapon for every point in the talent.
			//This does not give them ammo.
			for (i = 0; i < talentLevel + 1; ++i)
				vrx_give_additional_respawn_weapons(ent, i + 1);
		}

		// az: restore blaster ammo on death
		ent->monsterinfo.lefty = 25 + 12.5 * ent->myskills.abilities[MAX_AMMO].level;

		//4.57 give a partial ability charge
		if (ent->myskills.abilities[SHIELD].current_level > 0)
			ent->myskills.abilities[SHIELD].charge = 50;
		if (ent->myskills.abilities[BERSERK].current_level > 0)
			ent->myskills.abilities[BERSERK].charge = 50;
		if (ent->myskills.abilities[BEAM].current_level > 0)
			ent->myskills.abilities[BEAM].charge = 50;

		// armor regen grants starting armor
		if (ent->myskills.abilities[ARMOR_REGEN].current_level > 0) {
			float factor = ent->myskills.abilities[ARMOR_REGEN].current_level * 0.05f;
			ent->client->pers.inventory[body_armor_index] += MAX_ARMOR(ent) * factor;
		}

	}
	//K03 End

	vrx_reset_player_state(ent);

	// clear playerstate values
	memset (&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;
//ZOID
	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
//ZOID

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	if (client->pers.weapon)//K03
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);

	// clear entity state values
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255;		// will use the skin specified model
//	ent->s.modelindex2 = 255;		// custom gun model
	ShowGun(ent);					// ### Hentai ### special gun model
	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	//JABot[start]
	if( ent->ai.is_bot == true )
		return;
	//JABot[end]

	ent->client->lowlight = 0; //GHz: clear lowlight flag

	// spawn a spectator
	if (client->pers.spectator) {
		client->chase_target = NULL;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		//K03 return;
	} else
		client->resp.spectator = false;

	if (level.time < pregame_time->value) {
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}

	//K03 Begin
	GetScorePosition();//Gets each persons rank and total players in game
	if (StartClient(ent))
		return;

//	if (ent->myskills.class_num == CLASS_BRAIN)
//		MorphToBrain(ent);

	ent->client->invincible_framenum = level.framenum + (int)(2 / FRAMETIME);//Add 2.0 seconds to invincibility when you spawn.
	KickPlayerBack(ent);//Kicks all campers away!
	//K03 End
	
	if (!killboxspawn->value)
	{
		if (!invasion->value || !pvm->value) // pvp telefrags. Kickback in pvm.
			KillBox(ent);
		else
			KillBoxMonsters(ent);
	}else
		KillBox(ent);
	gi.linkentity (ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch (edict_t *ent)
{
	if (debuginfo->value > 1)
		gi.dprintf("ClientBeginDeathmatch()\n");

	G_InitEdict (ent);

	InitClientResp (ent->client);

	//K03 Begin
	ent->client->pers.spectator = true;
	ent->client->resp.spectator = true;
	ent->client->cloaking = false;
	ent->max_pipes = 0;
	ent->num_hammers = 0;//4.4 Talent: Boomerang

	V_AutoStuff(ent);

	if (SPREE_WAR == true && ent && ent == SPREE_DUDE)
	{
		SPREE_WAR = false;
		SPREE_DUDE = NULL;
	}
	//K03 End

	// locate ent at a spawn point
	PutClientInServer (ent);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	//JABot[start]
	AI_EnemyAdded(ent);
	//[end]

	gi.bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

//	gi.centerprintf(ent,ClientMessage);

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{

	if (debuginfo->value > 1)
		gi.dprintf("ClientBegin()\n");

	ent->client = game.clients + (ent - g_edicts - 1);

	if (!ent->ai.is_bot)
	{
		//[QBS]
		// set msg mode fully on so zbot would receive text & crash :)
		gi.WriteByte (svc_stufftext);
		gi.WriteString ("msg 4\n");
		gi.unicast(ent, true);

		//[QBS] RATBOT STOPPER 
		gi.WriteByte (svc_stufftext);
		gi.WriteString (".==|please.disconnect.all.bots|==.\n");
		gi.unicast(ent, true);
		//[QBS]

		gi.WriteByte (svc_stufftext);
		gi.WriteString ("msg 0\n");
		gi.unicast(ent, true);
		//[QBS]end
	}

	ClientBeginDeathmatch (ent);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/

void classmenu_handler (edict_t *ent, int option); // az
void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	char	*s;
	int		playernum;
	char	ip[16];
	
	if (debuginfo->value > 1) {
        gi.dprintf("ClientUserinfoChanged(): %s\n", userinfo);
    }


	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy (userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	//r1: FIXME: clean up client pers/resp system, don't allow the player to change their own ip via userinfo!!
	
	// don't allow invalid ip
	s = Info_ValueForKey (userinfo, "ip");
	if (strlen(s) < 1)
	{
		s = Info_ValueForKey (ent->client->pers.userinfo, "ip");
		Info_SetValueForKey(userinfo, "ip", s);
	}
	
	// update current ip, with less userinfo clobbering
	s = Info_ValueForKey (userinfo, "ip");

	if (s[0])
	{
		Q_strncpy (ip, s, sizeof(ip)-1);

		s = strchr (ip, ':');
		if (s)
			s[0] = '\0';

		strncpy(ent->client->pers.current_ip, ip, sizeof(ent->client->pers.current_ip)-1);
	}

	// name changes not allowed
	s = Info_ValueForKey (userinfo, "name");
	// log event if new name is different than old name
	if (strcmp(s, ent->client->pers.netname))
	{
        vrx_write_to_logfile(ent, va("Changed name to %s.\n", s));
		if (strlen(ent->client->pers.netname) > 3)
			gi.bprintf(PRINT_HIGH, "%s changed name to %s\n", ent->client->pers.netname, s);
	}

	if (strlen( ent->client->pers.netname) < 1 || G_IsSpectator(ent)) {
        strncpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname) - 1);
        if (ent->client->menustorage.optionselected == classmenu_handler)
            menu_close(ent, true); // az. just in case
    }

	Info_SetValueForKey(userinfo, "name", ent->client->pers.netname);

	if (!ClientCanConnect(ent, userinfo))
	{
		s = Info_ValueForKey (userinfo, "rejmsg");
		gi.bprintf(PRINT_HIGH, "%s was kicked. Reason: %s\n", ent->client->pers.netname, s);
		stuffcmd(ent, "disconnect\n");
		return;
	}

	// set skin
	s = Info_ValueForKey (userinfo, "skin");
	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	if (!G_IsSpectator(ent) && !vrx_assign_character_skin(ent, s))
		gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = 2;
		if (ent->myskills.class_num > 0)
		{
            if ((!vrx_is_morphing_polt(ent) || !ent->mtype) && ent->myskills.class_num != CLASS_KNIGHT)
                ent->client->pers.hand = atoi(s);
        }
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	static int lastID = 1;
	char	ip[16];
	char	*value;
	
	if (debuginfo->value > 1) {
        gi.dprintf("ClientConnect(): %s\n", userinfo);
    }

	value = Info_ValueForKey (userinfo, "name");

	if (strlen(value) < 3)
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Your nickname must be at least 5 characters long.");
		return false;
	}

	// Reset names!
	memset(ent->client->pers.netname, 0, sizeof(ent->client->pers.netname)-1);


	strncpy (ent->client->pers.netname, value, sizeof(ent->client->pers.netname)-1);

	// update current ip
	value = Info_ValueForKey (userinfo, "ip");
	// update current ip, with less userinfo clobbering
	if (value[0])
	{
		Q_strncpy (ip, value, sizeof(ip)-1);

		value = strchr (ip, ':');
		if (value)
			value[0] = '\0';

		strncpy(ent->client->pers.current_ip, ip, sizeof(ent->client->pers.current_ip)-1);
	}

	//Info_SetValueForKey(userinfo, "ip", value);

	if (!ClientCanConnect(ent, userinfo))
		return false;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a spectator
	value = Info_ValueForKey (userinfo, "spectator");
	if (deathmatch->value && *value && strcmp(value, "0")) {
		int i, numspec;

		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < maxclients->value; i++)
			if (g_edicts[i+1].inuse && g_edicts[i+1].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	} else {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		InitClientResp (ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant (ent->client);

		//K03 Begin
		if (SPREE_WAR == true && ent && ent == SPREE_DUDE)
		{
			SPREE_WAR = false;
			SPREE_DUDE = NULL;
		}
		//K03 End
	}

	ClientUserinfoChanged (ent, userinfo);
//GHz START
	value = Info_ValueForKey (userinfo, "ip");
	if (game.maxclients > 1)
	{
		gi.dprintf ("%s@%s connected at %s on %s\n", ent->client->pers.netname, value, CURRENT_TIME, CURRENT_DATE);
        vrx_write_to_logfile(ent, "Connected to server.\n");
	}
//GHz END

	ent->svflags = 0;// make sure we start with known default
	ent->client->pers.connected = true;
	ent->ai.is_bot = false;

	ent->gds_connection_id = lastID;
	lastID++;

	if (lastID == INT_MAX - 1)
	{
		gi.dprintf("We seem to have passed a big player ID. Resetting!");
		lastID = 1;
	}

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void KillMyVote (edict_t *ent);
void soldier_die(edict_t *ent);
void turret_remove(edict_t *ent);
void SaveCharacterQuit (edict_t *ent);

void ClientDisconnect (edict_t *ent)
{
	int		i;
	edict_t	*scan = NULL;
	edict_t *player;
    vec3_t zvec={0,0,0};
	int		playernum;

	if (debuginfo->value > 1)
		gi.dprintf("ClientDisconnect()\n");

	if (!ent->client)
		return;

	KillMyVote (ent);

    vrx_clean_damage_list(ent, true);
	vrx_reset_player_state(ent);

	// remove this player from allies' list
	if (allies->value && !ctf->value && !domination->value && !ptr->value)
	{
		NotifyAllies(ent, 1, va("%s disconnected\n", ent->client->pers.netname));
		RemoveAlly(ent, NULL);	
	}

	// remove from list of players waiting to spawn
	if (INVASION_OTHERSPAWNS_REMOVED)
		vrx_inv_remove_spawn_que(ent);

	if (ent->teamnum && !G_IsSpectator(ent))
	{
		if (!tbi->value)
		{
			gi.bprintf(PRINT_HIGH, "Waiting for player(s) to re-join...\n");
			AddJoinedQueue(ent);
		}else
			OrganizeTeams(false);
	}

	average_player_level = AveragePlayerLevel();//GHz
	pvm_average_level = PvMAveragePlayerLevel();

	// make sure there are no mini-bosses on spree war status
	if (deathmatch->value)
	{
		for (i = 1; i <= maxclients->value; i++) {
			player = &g_edicts[i];
			if (!player->inuse)
				continue;
			if (player->solid == SOLID_NOT)
				continue;
			if (vrx_is_newbie_basher(player) && SPREE_WAR && (SPREE_DUDE == player))
			{
				SPREE_WAR = false;
				SPREE_DUDE = NULL;
			}
		}
	}

	//K03 Begin -- Save users info

	if (SPREE_WAR == true && (ent == SPREE_DUDE))
	{
		SPREE_WAR = false;
		SPREE_DUDE = NULL;
	}

    vrx_save_character(ent, true);

	//GHz: Reset their skills_t to prevent any possibility of duping
	memset(&ent->myskills,0,sizeof(skills_t));
	//K03 End

	gi.sound(ent, CHAN_VOICE, gi.soundindex("misc/eject.wav"), 1, ATTN_NORM, 0);
	gi.bprintf (PRINT_HIGH, "%s disconnected at %s on %s\n", ent->client->pers.netname, CURRENT_TIME, CURRENT_DATE);

	if (G_EntExists(ent))
        vrx_write_to_logfile(ent, "Logged out.\n");
	else
        vrx_write_to_logfile(ent, "Disconnected from server.\n");

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");

	//JABot[start]
	AI_EnemyRemoved (ent);
	//[end]
}

edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
//GHz START
	// allows non-standard bbox sizes to be used by pmove()
	if (pm_passent->mtype)
	{
		VectorCopy(pm_passent->mins, mins);
		VectorCopy(pm_passent->maxs, maxs);
	}
//GHz END
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}


/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/


void RunParasiteFrames (edict_t *ent, usercmd_t *ucmd);
void RunBrainFrames (edict_t *ent, usercmd_t *ucmd);
void V_AutoAim (edict_t *player);
void UpdateMirroredEntities (edict_t *ent);
void LeapAttack (edict_t *ent);

float V_ModifyMovement(edict_t *ent, usercmd_t *ucmd, que_t *curse);

void think_trade(edict_t *ent);
void BlinkStrike_think(edict_t* ent);
void V_PickUpEntity(edict_t* ent);

void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j;
	pmove_t	pm;
	float velocity_mod = 1;

    que_t *curse = NULL;
	int		viewheight;

	defer_run(&ent->client->defers);

// GHz START
	// if we're a morphed player, then save the current viewheight
	// the player will be locked into this viewheight while morphed
    // if (ent->mtype)
    viewheight = ent->viewheight;
//GHz END

	//K03 Begin
	if (ent->client->resp.spectator != true)
	{
        velocity_mod = V_ModifyMovement(ent, ucmd, curse);

		//End of slow move
		if(que_typeexists(ent->curses, CURSE_FROZEN))
		{
			ent->client->ps.pmove.pm_type = PM_DEAD;
			if (ent->client->buttons & BUTTON_ATTACK)
				ent->client->buttons &= ~BUTTON_ATTACK;
			return;
		}
	}

	//K03 End
	//get JumpMax

//--------------------------------------
	level.current_entity = ent;
	client = ent->client;

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if (level.time > level.intermissiontime + 5.0 
			&& (ucmd->buttons & BUTTON_ANY) )
			level.exitintermission = true;
		return;
	}

	

	pm_passent = ent;

	if (ent->client->chase_target) {
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	} else {

		if (ent->lockon == 1 && ent->enemy)
		{
			client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
			client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
			client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
		}

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		
		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;
		//K03 Begin
		if (client->hook_state == HOOK_ON)
			client->ps.pmove.gravity = 0;
		else {
			//3.0 matrix jump
			if (ent->v_flags & SFLG_MATRIXJUMP)
				client->ps.pmove.gravity = sv_gravity->value / MJUMP_GRAVITY_MULT;
			else if ((ent->mtype == MORPH_FLYER) && (ent->deadflag != DEAD_DEAD)) // flyers are not affected by gravity
				client->ps.pmove.gravity = 0;
			else
				client->ps.pmove.gravity = sv_gravity->value;
			//end doomie

			// az: uncomment to make gravity multiplier work
			// client->ps.pmove.gravity *= ent->gravity;
		}
		//K03 End

		// reset jump flag
		if (!ucmd->upmove)
			ent->client->jump = false;

		// double jump
		if (CanDoubleJump(ent, ucmd))
		{
			if (ent->velocity[2] < 256)
				ent->velocity[2] = 256;
			else
				ent->velocity[2] += 256;
			ent->v_flags |= SFLG_DOUBLEJUMP;
			V_PlayerJump(ent);
		}
		
		pm.s = client->ps.pmove;

		// az: apply slowdowns
		if (velocity_mod < 1) {
		    ucmd->forwardmove *= velocity_mod;
		    ucmd->sidemove *= velocity_mod;
		}

		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = ent->s.origin[i]*8;
            pm.s.velocity[i] = ent->velocity[i] * 8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapinitial = true;

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		

		pm.pointcontents = gi.pointcontents;

		// perform a pmove
		gi.Pmove (&pm);

		// az: for SPEED, run a Pmove twice
		// we don't wan't to compound on the existing velocity
		// thus we undo the change in velocity right after...
		if (velocity_mod > 1) {
		    float oldZpos, oldZspeed;
		    oldZpos = pm.s.origin[2];
		    oldZspeed = pm.s.velocity[2];

			pm.s.velocity[0] += ent->velocity[0] * 8 * (velocity_mod - 1);
            pm.s.velocity[1] += ent->velocity[1] * 8 * (velocity_mod - 1);
			gi.Pmove(&pm);
			pm.s.velocity[0] -= ent->velocity[0] * 8 * (velocity_mod - 1);
            pm.s.velocity[1] -= ent->velocity[1] * 8 * (velocity_mod - 1);

            pm.s.origin[2] = oldZpos;
            pm.s.velocity[2] = oldZspeed;
		}

// GHz START
		// if this is a morphed player, restore saved viewheight
		// this locks them into that viewheight
		if (ent->mtype)
			pm.viewheight = viewheight;
//GHz END



		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i=0 ; i<3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			ent->velocity[i] = pm.s.velocity[i]*0.125;
		}

		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		if (!(ent->lockon == 1 && ent->enemy)){
			client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
			client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
			client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
		}

		//K03 Begin
		//4.07 can't superspeed while being hurt
        // pm = V_Think_ApplySuperSpeed(ent, ucmd, client, i, &pm, viewheight);

		if (/*ent->groundentity && !pm.groundentity &&*/ (pm.cmd.upmove >= 10) /*&& (pm.waterlevel == 0)*/)
		{
			if (((ent->mtype == MORPH_BRAIN || ent->mtype == MORPH_MUTANT) && pm.waterlevel > 0) || 
				 (ent->groundentity && !pm.groundentity))
			{
				V_PlayerJump(ent);
				ent->client->jump = true;
			}
		}

		//GHz: bots should call touchdown function if they have one after landing
		if (ent->ai.is_bot && !ent->groundentity && pm.groundentity && ent->monsterinfo.touchdown)
		{
			ent->monsterinfo.touchdown(ent);
		}
		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;

		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{	
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		// touch other objects
		for (i=0 ; i<pm.numtouch ; i++)
		{
			other = pm.touchents[i];
			for (j=0 ; j<i ; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch (other, ent, NULL, NULL);
		}

					//3.0 begin doomie
	
			/*************************************************************************
			The following is a flag to replace pm.waterlevel
				there is no point in calling gi.Pmove() more than how often iD
				calls for it, if we can avoid doing so. -doomie
			**************************************************************************/

			switch (pm.waterlevel)
			{
			case 0:	//not in water
				{
					//Make sure all water flags are off
					if (ent->v_flags & (SFLG_TOUCHING_WATER))
					{
						if (ent->v_flags & SFLG_UNDERWATER)
							ent->v_flags ^= SFLG_UNDERWATER;
						if (ent->v_flags & SFLG_PARTIAL_INWATER)
							ent->v_flags ^= SFLG_PARTIAL_INWATER;
					}
				}
				break;
			case 1:	//standing or swimming
				{
					//Turn off underwater flag
					if (ent->v_flags & SFLG_UNDERWATER)
						ent->v_flags ^= SFLG_UNDERWATER;
					//Turn on partial underwater flag
					if (!(ent->v_flags & SFLG_PARTIAL_INWATER))
						ent->v_flags ^= SFLG_PARTIAL_INWATER;
				}
				break;
			default: //underwater
				{
					//Turn on underwater flag
					if (!(ent->v_flags & SFLG_UNDERWATER))
						ent->v_flags ^= SFLG_UNDERWATER;
					//Turn off partial underwater flag
					if (ent->v_flags & SFLG_PARTIAL_INWATER)
						ent->v_flags ^= SFLG_PARTIAL_INWATER;
				}
				break;
			}

			if ((ent->v_flags & SFLG_MATRIXJUMP) && (ent->velocity[2] < 10))
				ent->v_flags ^= SFLG_MATRIXJUMP;

	//end doomie (3.0)

	}
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (client->resp.spectator) {

			client->latched_buttons = 0;

			//K03 Begin
			client->chasecam_mode = (client->chasecam_mode == 1) ? 0 : 1;
			safe_cprintf(ent, PRINT_HIGH, "CHASE CAM MODE: %s\n", (client->chasecam_mode == 1) ? "EYE CAM" : "FOLLOW CAM");
			if (!client->chase_target)
				GetChaseTarget(ent);

		} else if (!client->weapon_thunk) {
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

	//JABot[start]
	AITools_DropNodes(ent);
	//JABot[end]

	// double jump reset flag
	if (ent->groundentity)
	{
		ent->v_flags &= ~SFLG_DOUBLEJUMP;

		// the player (parasite) has completed their jump, clear the flag
		ent->monsterinfo.jumpup = 0;

		//Talent: Leap Attack - player has touched down
		LeapAttack(ent);
	}

	if (client->resp.spectator) {
		if (ucmd->upmove >= 10) {
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		} else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	UpdateChaseCam(ent);

	if (ent->lockon == 1)
		V_AutoAim(ent);

    think_trade(ent);

    boss_update(ent, ucmd);
	RunParasiteFrames(ent, ucmd);
	RunCacodemonFrames(ent, ucmd);
	//RunTankFrames(ent, ucmd);
	RunBrainFrames(ent, ucmd);
	RunFlyerFrames(ent, ucmd);
	RunMutantFrames(ent, ucmd);
	RunMedicFrames(ent, ucmd);
	RunBerserkFrames(ent, ucmd);
	EatCorpses(ent);
	UpdateMirroredEntities(ent);
	vrx_morph_think(ent);
	BlinkStrike_think(ent);
	V_PickUpEntity(ent);
}

// az: this code has been a little redundant in places
void vrx_remove_chat_protect(edict_t* ent) {
	ent->flags &= ~FL_CHATPROTECT;
	if (!PM_PlayerHasMonster(ent))
	{
		if (!pvm->value)
		{
			ent->solid = SOLID_BBOX;
			ent->svflags &= ~SVF_NOCLIENT;//4.5
		}
	}

	// in pvp modes, teleport them.
	if (!pvm->value)
		Teleport_them(ent);
}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;

	if (level.intermissiontime)
		return;

	//GHz START
	if (G_EntExists(ent) && !(level.framenum % (int)(1 / FRAMETIME)))
	{
		//3.0 reduce their mute list
		int i;
        for(i = 0; i < MAX_CLIENTS; ++i)
		{
			if (!ent->myskills.mutelist[i].player || ent->myskills.mutelist[i].time < 1)
				continue;
			ent->myskills.mutelist[i].time -= 1;
			if (ent->myskills.mutelist[i].time < 1)
				ent->myskills.mutelist[i].player = NULL;

		}
		//3.0 end
		ent->myskills.playingtime++;		
	}

	// idle frame counter
    think_idle_frame_counter(ent);

	// chat-protect ends when you move or fire
	if ((ent->client->idle_frames < qf2sf(CHAT_PROTECT_FRAMES)) && (ent->flags & FL_CHATPROTECT))
	{
		vrx_remove_chat_protect(ent);
	}

    think_player_inactivity(ent);

	// initialize chat-protect
    /*else*/
    think_chat_protect_activate(ent);
	//GHz END

	client = ent->client;

	if (ent->lastkill < level.time && ent->nfer) // we're out of nfer time!
	{
		G_PrintGreenText(va("%s got a %dfer.", ent->client->pers.netname, ent->nfer));
		ent->nfer = 0;
	}
		

	if (ent->client->resp.spectator != true)
        vrx_client_think(ent);

	//Check player to see if they were loading or saving
	//if(gds->value)	GDS_CheckPlayer(ent);

	if (deathmatch->value &&
		client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5) {
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk
//ZOID
		&& ent->movetype != MOVETYPE_NOCLIP
//ZOID
		)
		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if ( level.time > client->respawn_time)
		{
			if (client->latched_buttons || ctf->value || // 3.7 force respawn in CTF
				(deathmatch->value && ((int)dmflags->value & DF_FORCE_RESPAWN)))
			{
				//gi.dprintf("respawn() called from player\n");
				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->value)
		if (!visible (ent, PlayerTrail_LastSpot() ) )
			PlayerTrail_Add (ent->s.old_origin);

	client->latched_buttons = 0;
}

