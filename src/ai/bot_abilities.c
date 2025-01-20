// GHz: this file should contain code that allows the bot to use Vortex character abilities
#include "g_local.h"
#include "ai_local.h"

//==========================================
// AI_InitAIAbilities
// populate AIAbilities with data needed to assist bots with combat ability selection
//==========================================
void AI_InitAIAbilities(void)
{
	//clear all
	memset(&AIAbilities, 0, sizeof(ai_ability_t) * MAX_ABILITIES);

	//MIRV
	AIAbilities[MIRV].is_weapon = true;
	AIAbilities[MIRV].aimType = AI_AIMSTYLE_BALLISTIC;
	AIAbilities[MIRV].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[MIRV].RangeWeight[AIWEAP_SNIPER_RANGE] = 0;
	AIAbilities[MIRV].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIAbilities[MIRV].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[MIRV].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2; // highly likely we're going to hurt ourselves
	AIAbilities[MIRV].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//NAPALM
	AIAbilities[NAPALM].is_weapon = true;
	AIAbilities[NAPALM].aimType = AI_AIMSTYLE_BALLISTIC;
	AIAbilities[NAPALM].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[NAPALM].RangeWeight[AIWEAP_SNIPER_RANGE] = 0;
	AIAbilities[NAPALM].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIAbilities[NAPALM].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[NAPALM].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2; // highly likely we're going to hurt ourselves
	AIAbilities[NAPALM].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//EMP
	AIAbilities[EMP].is_weapon = true;
	AIAbilities[EMP].aimType = AI_AIMSTYLE_BALLISTIC;
	AIAbilities[EMP].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[EMP].RangeWeight[AIWEAP_SNIPER_RANGE] = 0;
	AIAbilities[EMP].RangeWeight[AIWEAP_LONG_RANGE] = 0.4;
	AIAbilities[EMP].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[EMP].RangeWeight[AIWEAP_SHORT_RANGE] = 0.1; // highly likely we're going to hurt ourselves
	AIAbilities[EMP].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//MAGICBOLT
	AIAbilities[MAGICBOLT].is_weapon = true;
	AIAbilities[MAGICBOLT].aimType = AI_AIMSTYLE_PREDICTION;
	AIAbilities[MAGICBOLT].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[MAGICBOLT].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.3;
	AIAbilities[MAGICBOLT].RangeWeight[AIWEAP_LONG_RANGE] = 0.4; // meteor/lightning storm is better
	AIAbilities[MAGICBOLT].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[MAGICBOLT].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4; // nova is better
	AIAbilities[MAGICBOLT].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;

	//FIREBALL
	AIAbilities[FIREBALL].is_weapon = true;
	AIAbilities[FIREBALL].aimType = AI_AIMSTYLE_BALLISTIC;
	AIAbilities[FIREBALL].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[FIREBALL].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.1;
	AIAbilities[FIREBALL].RangeWeight[AIWEAP_LONG_RANGE] = 0.3;
	AIAbilities[FIREBALL].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[FIREBALL].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2; // highly likely we'll take some damage
	AIAbilities[FIREBALL].RangeWeight[AIWEAP_MELEE_RANGE] = 0.1;

	//PLASMA_BOLT
	AIAbilities[PLASMA_BOLT].is_weapon = true;
	AIAbilities[PLASMA_BOLT].aimType = AI_AIMSTYLE_PREDICTION_EXPLOSIVE;
	AIAbilities[PLASMA_BOLT].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[PLASMA_BOLT].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.3;
	AIAbilities[PLASMA_BOLT].RangeWeight[AIWEAP_LONG_RANGE] = 0.4; // meteor/lightning storm is better
	AIAbilities[PLASMA_BOLT].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[PLASMA_BOLT].RangeWeight[AIWEAP_SHORT_RANGE] = 0.2; // highly likely we'll take some damage
	AIAbilities[PLASMA_BOLT].RangeWeight[AIWEAP_MELEE_RANGE] = 0; // don't bother!

	//NOVA
	AIAbilities[NOVA].is_weapon = true;
	AIAbilities[NOVA].aimType = AI_AIMSTYLE_NONE;
	AIAbilities[NOVA].idealRange = AI_RANGE_SHORT;
	AIAbilities[NOVA].RangeWeight[AIWEAP_SNIPER_RANGE] = 0;
	AIAbilities[NOVA].RangeWeight[AIWEAP_LONG_RANGE] = 0;
	AIAbilities[NOVA].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0; // nova can't hit anything beyond short range
	AIAbilities[NOVA].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIAbilities[NOVA].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5;

	//CRIPPLE (aka static field!)
	AIAbilities[CRIPPLE].is_weapon = true;
	AIAbilities[CRIPPLE].aimType = AI_AIMSTYLE_NONE; // radius attack, no aiming needed
	AIAbilities[CRIPPLE].idealRange = AI_RANGE_LONG;
	AIAbilities[CRIPPLE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0; // exceeds cripple max range
	AIAbilities[CRIPPLE].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIAbilities[CRIPPLE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[CRIPPLE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIAbilities[CRIPPLE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5;

	//LIGHTNING
	AIAbilities[LIGHTNING].is_weapon = true;
	AIAbilities[LIGHTNING].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIAbilities[LIGHTNING].idealRange = AI_RANGE_MEDIUM;
	AIAbilities[LIGHTNING].RangeWeight[AIWEAP_SNIPER_RANGE] = 0;
	AIAbilities[LIGHTNING].RangeWeight[AIWEAP_LONG_RANGE] = 0.4; // meteor/lightning storm is better
	AIAbilities[LIGHTNING].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.5;
	AIAbilities[LIGHTNING].RangeWeight[AIWEAP_SHORT_RANGE] = 0.4; // nova is better
	AIAbilities[LIGHTNING].RangeWeight[AIWEAP_MELEE_RANGE] = 0.4;

	//METEOR
	AIAbilities[METEOR].is_weapon = true;
	AIAbilities[METEOR].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIAbilities[METEOR].idealRange = AI_RANGE_SNIPER;
	AIAbilities[METEOR].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.5;
	AIAbilities[METEOR].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIAbilities[METEOR].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4; // fireball/lightning/magicbolt is better
	AIAbilities[METEOR].RangeWeight[AIWEAP_SHORT_RANGE] = 0;
	AIAbilities[METEOR].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//LIGHTNING_STORM
	AIAbilities[LIGHTNING_STORM].is_weapon = true;
	AIAbilities[LIGHTNING_STORM].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIAbilities[LIGHTNING_STORM].idealRange = AI_RANGE_SNIPER;
	AIAbilities[LIGHTNING_STORM].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.5;
	AIAbilities[LIGHTNING_STORM].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIAbilities[LIGHTNING_STORM].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4; // fireball/lightning/magicbolt is better
	AIAbilities[LIGHTNING_STORM].RangeWeight[AIWEAP_SHORT_RANGE] = 0;
	AIAbilities[LIGHTNING_STORM].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//BOMB_SPELL
	AIAbilities[BOMB_SPELL].is_weapon = true;
	AIAbilities[BOMB_SPELL].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIAbilities[BOMB_SPELL].idealRange = AI_RANGE_SNIPER;
	AIAbilities[BOMB_SPELL].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.5;
	AIAbilities[BOMB_SPELL].RangeWeight[AIWEAP_LONG_RANGE] = 0.5;
	AIAbilities[BOMB_SPELL].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.4; // fireball/lightning/magicbolt is better
	AIAbilities[BOMB_SPELL].RangeWeight[AIWEAP_SHORT_RANGE] = 0;
	AIAbilities[BOMB_SPELL].RangeWeight[AIWEAP_MELEE_RANGE] = 0;

	//AMP_DAMAGE
	AIAbilities[AMP_DAMAGE].is_weapon = true;
	AIAbilities[AMP_DAMAGE].aimType = AI_AIMSTYLE_INSTANTHIT;
	AIAbilities[AMP_DAMAGE].idealRange = AI_RANGE_SNIPER;
	AIAbilities[AMP_DAMAGE].RangeWeight[AIWEAP_SNIPER_RANGE] = 0.6;
	AIAbilities[AMP_DAMAGE].RangeWeight[AIWEAP_LONG_RANGE] = 0.6;
	AIAbilities[AMP_DAMAGE].RangeWeight[AIWEAP_MEDIUM_RANGE] = 0.6; // prefer to curse a target before using other attacks, except at med-close range
	AIAbilities[AMP_DAMAGE].RangeWeight[AIWEAP_SHORT_RANGE] = 0.5;
	AIAbilities[AMP_DAMAGE].RangeWeight[AIWEAP_MELEE_RANGE] = 0.5;
}

qboolean CanTball(edict_t* ent, qboolean print);
// try to use a tball if we have one, return true if we were successful in doing so
qboolean BOT_DMclass_UseTball(edict_t* self, qboolean forget_enemy)
{
	if (CanTball(self, false) && self->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)])
	{
		// use tball to teleport away!
		Tball_Aura(self, self->s.origin);
		// reduce inventory count
		self->client->pers.inventory[ITEM_INDEX(Fdi_TBALL)]--;
		// don't do it too often
		self->client->tball_delay = level.time + GetRandom(4, 10);
		// if we were angry at someone, forget it so that we can focus on rearming/healing
		if (forget_enemy)
			self->enemy = NULL;
		// change state so that AI_Think finds a new goal and path since we teleported away from our previous one
		//self->ai.state = BOT_STATE_WANDER;
		AI_ResetNavigation(self);
		self->ai.bloqued_timeout = level.time + 15.0;
		return true;
	}
	return false;
}

float AI_GetAbilityRangeWeightByDistance(int ability_index, float distance)
{
	if (distance <= AI_RANGE_MELEE)
		return AIAbilities[ability_index].RangeWeight[AIWEAP_MELEE_RANGE];
	else if (distance <= AI_RANGE_SHORT)
		return AIAbilities[ability_index].RangeWeight[AIWEAP_SHORT_RANGE];
	else if (distance <= AI_RANGE_MEDIUM)
		return AIAbilities[ability_index].RangeWeight[AIWEAP_MEDIUM_RANGE];
	else if (distance <= AI_RANGE_LONG)
		return AIAbilities[ability_index].RangeWeight[AIWEAP_LONG_RANGE];
	else
		return AIAbilities[ability_index].RangeWeight[AIWEAP_SNIPER_RANGE];
}

float AI_GetAbilityProjectileVelocity(edict_t* ent, int ability_index)
{
	int slvl = ent->myskills.abilities[ability_index].current_level;
	switch (ability_index)
	{
	case MIRV: return 600;
	case NAPALM: return 400;
	case EMP: return 600;
	case MAGICBOLT: return BOLT_SPEED;
	case FIREBALL: return (FIREBALL_INITIAL_SPEED + FIREBALL_ADDON_SPEED * slvl);
	case PLASMA_BOLT: return (PLASMABOLT_INITIAL_SPEED + PLASMABOLT_ADDON_SPEED * slvl);
	}
	return 0;
}

float AI_GetAbilityCost(edict_t* ent, int ability_index)
{
	switch (ability_index)
	{
	case MIRV: return (MIRV_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER));
	case NAPALM: return (NAPALM_COST - vrx_get_talent_level(ent, TALENT_BOMBARDIER));
	case EMP: return (25 - vrx_get_talent_level(ent, TALENT_BOMBARDIER));
	case MAGICBOLT: return BOLT_COST;
	case FIREBALL: return FIREBALL_COST;
	case NOVA: return NOVA_COST;
	case LIGHTNING: return LIGHTNING_COST;
	case METEOR: return METEOR_COST;
	case LIGHTNING_STORM: return LIGHTNING_COST;
	case BOMB_SPELL: return COST_FOR_BOMB;
	case PLASMA_BOLT: return PLASMABOLT_COST;
	case CRIPPLE: return CRIPPLE_COST;
	case AMP_DAMAGE: return AMP_DAMAGE_COST;
	}
	return 0;
}

qboolean CanCurseTarget(edict_t* caster, edict_t* target, int type, qboolean isCurse, qboolean vis);

//==========================================
// BOT_DMclass_ChooseAbility
// Choose attack ability based on range & weights
//==========================================
int BOT_DMclass_ChooseAbility(edict_t* self)
{
	float	dist;
	int		i;
	float	best_weight = 0.0;
	int		best_ability = -1;
	int		weapon_range = 0;

	// no enemy
	if (!self->enemy)
		return -1;

	// no power cubes
	if (self->client->pers.inventory[power_cube_index] < 1)
		return -1;

	dist = entdist(self, self->enemy);

	if (dist < AI_RANGE_MELEE)
		weapon_range = AIWEAP_MELEE_RANGE;
	else if (dist < AI_RANGE_SHORT)
		weapon_range = AIWEAP_SHORT_RANGE;
	else if (dist < AI_RANGE_MEDIUM)
		weapon_range = AIWEAP_MEDIUM_RANGE;
	else if (dist < AI_RANGE_LONG)
		weapon_range = AIWEAP_LONG_RANGE;
	else
		weapon_range = AIWEAP_SNIPER_RANGE;

	for (i = 0; i < MAX_ABILITIES; i++)
	{
		// ability can't be fired or no data
		if (!AIAbilities[i].is_weapon)
			continue;
		// ability is disabled or not upgraded
		if (self->myskills.abilities[i].disable || self->myskills.abilities[i].current_level < 1)
			continue;
		// not enough power cubes to use this ability
		if (self->client->pers.inventory[power_cube_index] < AI_GetAbilityCost(self, i))
			continue;
		// don't try to curse uncurseable targets or use the same curse
		if ((i == AMP_DAMAGE || i == WEAKEN || i == LIFE_TAP || i == CURSE) 
			&& (!CanCurseTarget(self, self->enemy, i, true, false) || que_typeexists(self->enemy->curses, i)))
			continue;

		//compare range weights
		float weight = AIAbilities[i].RangeWeight[weapon_range];
		if (weight > best_weight) {
			best_weight = weight;
			best_ability = i;
		}
		// random selection for abilities that have the same weight
		else if (weight == best_weight && random() > 0.2) 
		{
			best_weight = weight;
			best_ability = i;
		}
	}

	return best_ability;
}

void Cmd_TossMirv(edict_t* ent);
void Cmd_Napalm_f(edict_t* ent);
void Cmd_TossEMP(edict_t* ent);
void Cmd_Magicbolt_f(edict_t* ent, float skill_mult, float cost_mult);
void Cmd_Fireball_f(edict_t* ent, float skill_mult, float cost_mult);
void Cmd_Nova_f(edict_t* ent, int frostLevel, float skill_mult, float cost_mult);
void Cmd_ChainLightning_f(edict_t* ent, float skill_mult, float cost_mult);
void Cmd_Meteor_f(edict_t* ent, float skill_mult, float cost_mult);
void Cmd_LightningStorm_f(edict_t* ent, float skill_mult, float cost_mult);
void Cmd_Plasmabolt_f(edict_t* ent);
void Cmd_Cripple_f(edict_t* ent);
void Cmd_AmpDamage(edict_t* ent);

// aims and fires at enemy using the specified ability
void BOT_DMclass_FireAbility(edict_t* self, int ability_index)
{
	vec3_t forward, start;
	qboolean is_rocket = false;
	qboolean is_ballistic = false;

	// can't use abilities right now
	if (self->client->ability_delay > level.time)
		return;

	// range check
	float dist = entdist(self, self->enemy);

	// don't bother firing if we're unlikely to hit anything
	if (AI_GetAbilityRangeWeightByDistance(ability_index, dist) < 0.1)
		return;

	// does this ability require aiming?
	if (AIAbilities[ability_index].aimType != AI_AIMSTYLE_NONE)
	{
		// don't fire abilities at the same time we're firing a weapon
		//if (self->client->weaponstate == WEAPON_FIRING)
		//	return;

		// get firing parameters
		int aimType = AIAbilities[ability_index].aimType;
		float speed = AI_GetAbilityProjectileVelocity(self, ability_index);
		if (aimType == AI_AIMSTYLE_BALLISTIC)
			is_ballistic = true;
		else if (aimType == AI_AIMSTYLE_PREDICTION_EXPLOSIVE)
			is_rocket = true;

		// calculate angle to enemy
		MonsterAim(self, -1, speed, is_rocket, 0, forward, start);
		VectorNormalize(forward);
		vectoangles(forward, forward);
		if (forward[PITCH] < -180)
			forward[PITCH] += 360;

		// is this a ballistic projectile?
		if (is_ballistic)
		{
			forward[PITCH] = BOT_DMclass_ThrowingPitch1(self, speed);
			//gi.dprintf("%s: speed: %.0f pitch: %.1f\n", __func__, speed, forward[PITCH]);
			// target can't be hit at current range
			if (forward[PITCH] < -90)
				forward[PITCH] = -45;
		}

		// copy ideal firing angles to bot
		VectorCopy(forward, self->s.angles);
		VectorCopy(forward, self->client->v_angle);
	}

	// use the ability
	switch (ability_index)
	{
	case MIRV: Cmd_TossMirv(self); break;
	case NAPALM: Cmd_Napalm_f(self); break;
	case EMP: Cmd_TossEMP(self); break;
	case MAGICBOLT: Cmd_Magicbolt_f(self, 1.0, 1.0); break;
	case FIREBALL: Cmd_Fireball_f(self, 1.0, 1.0); break;
	case NOVA: Cmd_Nova_f(self, 0, 1.0, 1.0); break;
	case LIGHTNING: Cmd_ChainLightning_f(self, 1.0, 1.0); break;
	case METEOR: Cmd_Meteor_f(self, 1.0, 1.0); break;
	case LIGHTNING_STORM: Cmd_LightningStorm_f(self, 1.0, 1.0); break;
	case PLASMA_BOLT: Cmd_Plasmabolt_f(self); break;
	case CRIPPLE: Cmd_Cripple_f(self); break;
	case AMP_DAMAGE: Cmd_AmpDamage(self); break;
	}
}

void Cmd_BlinkStrike_f(edict_t* self);
void BOT_DMclass_UseBlinkStrike(edict_t* self)
{
	// calculate cost
	int cost = BLINKSTRIKE_INITIAL_COST + BLINKSTRIKE_ADDON_COST * self->myskills.abilities[BLINKSTRIKE].current_level;
	if (BLINKSTRIKE_MIN_COST && cost < BLINKSTRIKE_MIN_COST)
		cost = BLINKSTRIKE_MIN_COST;

	// can't use blinkstrike
	if (!V_CanUseAbilities(self, BLINKSTRIKE, cost, false))
		return;
	// range check
	float dist = entdist(self, self->enemy);
	if (dist > AI_RANGE_SNIPER || dist < AI_RANGE_SHORT)
		return;
	// don't bother against flying opponents
	if (self->enemy->flags & FL_FLY)
		return;
	// bot should be attacking and using CombatMovement
	if (self->ai.state != BOT_STATE_ATTACK)
		return;
	Cmd_BlinkStrike_f(self);
}

void BOT_DMclass_UseBoost(edict_t* self)
{
	vec3_t forward, start;
	qboolean is_rocket = true;

	// can't boost while being hurt
	if (self->lasthurt + DAMAGE_ESCAPE_DELAY > level.time)
		return;
	// can't use boost
	if (!V_CanUseAbilities(self, BOOST_SPELL, COST_FOR_BOOST, false))
		return;
	// bot should be attacking and using CombatMovement
	if (self->ai.state != BOT_STATE_ATTACK)
		return;
	// range check
	float dist = entdist(self, self->enemy);
	if (dist > AI_RANGE_SNIPER || dist < AI_RANGE_SHORT)
		return;

	// normally we aim at the floor to prevent overshooting target.. unless it flies
	if (self->enemy->flags & FL_FLY)
		is_rocket = false;

	// get aiming vector
	MonsterAim(self, -1, 1000, is_rocket, 0, forward, start);
	VectorNormalize(forward);
	// convert to angles
	vectoangles(forward, forward);
	if (forward[PITCH] < -180)
		forward[PITCH] += 360;
	// copy ideal firing angles to bot
	VectorCopy(forward, self->s.angles);
	VectorCopy(forward, self->client->v_angle);
	// use the ability
	Cmd_BoostPlayer(self);
}

void Cmd_Raise_Skeleton_f(edict_t* ent);
void skeleton_set_bbox(vec3_t mins, vec3_t maxs);

void BOT_DMclass_UseSkeleton(edict_t* self)
{
	// can't use skeleton
	if (!V_CanUseAbilities(self, SKELETON, SKELETON_COST+10, false))
		return;
	// can't spawn any more
	if (self->num_skeletons >= SKELETON_MAX)
		return;
	// get view origin
	vec3_t forward, right, start, offset, mins, maxs;
	AngleVectors(self->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 8, self->viewheight - 8);
	P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);
	// get skeleton bounding box mins and maxs
	skeleton_set_bbox(mins, maxs);
	// abort if the skeleton can't fit at start
	if (!G_GetSpawnLocation(self, 64, mins, maxs, start, NULL, PROJECT_HITBOX_FAR, false))
		return;

	Cmd_Raise_Skeleton_f(self);
}