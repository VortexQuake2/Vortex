#include "g_local.h"
#include "../../characters/class_limits.h"
#include "../../gamemodes/ctf.h"

#define STATION_COST			50

int MaxAmmoType (edict_t *ent, int ammo_index)
{
	if (!ammo_index)
		return 0;
	else if (ammo_index == shell_index)//ITEM_INDEX(FindItemByClassname("ammo_shells")))
		return ent->client->pers.max_shells;
	else if (ammo_index == bullet_index)//ITEM_INDEX(FindItemByClassname("ammo_bullets")))
		return ent->client->pers.max_bullets;
	else if (ammo_index == grenade_index)//ITEM_INDEX(FindItemByClassname("ammo_grenades")))
		return ent->client->pers.max_grenades;
	else if (ammo_index == rocket_index)//ITEM_INDEX(FindItemByClassname("ammo_rockets")))
		return ent->client->pers.max_rockets;
	else if (ammo_index == cell_index)//ITEM_INDEX(FindItemByClassname("ammo_cells")))
		return ent->client->pers.max_cells;
	else if (ammo_index == slug_index)//ITEM_INDEX(FindItemByClassname("ammo_slugs")))
		return ent->client->pers.max_slugs;
	else if (ammo_index == magslug_index)
		return ent->client->pers.max_magslug;
	else if (ammo_index == trap_index)
		return ent->client->pers.max_trap;
	else if (ammo_index == tesla_index)
		return ent->client->pers.max_tesla;
	else if (ammo_index == disruptor_index)
		return ent->client->pers.max_disruptor;
	else return 0;
}

int G_GetRespawnWeaponIndex (edict_t *ent)
{
	gitem_t *item = NULL;

	switch (ent->myskills.respawn_weapon)
	{
	case 2:		return ITEM_INDEX(Fdi_SHOTGUN);
	case 3:		return ITEM_INDEX(Fdi_SUPERSHOTGUN);
	case 4:		return ITEM_INDEX(Fdi_MACHINEGUN);
	case 5:		return ITEM_INDEX(Fdi_CHAINGUN);
	case 6:		return ITEM_INDEX(Fdi_GRENADELAUNCHER);
	case 7:		return ITEM_INDEX(Fdi_ROCKETLAUNCHER);
	case 8:		return ITEM_INDEX(Fdi_HYPERBLASTER);
	case 9:		return ITEM_INDEX(Fdi_RAILGUN);
	case 10:	return ITEM_INDEX(Fdi_BFG);
	case 11:	return grenade_index;
	case 12:	return ITEM_INDEX(Fdi_20MM);
	case 14:
		item = FindItem("Ionripper");
		return item ? ITEM_INDEX(item) : 0;
	case 15:
		item = FindItem("Phalanx");
		return item ? ITEM_INDEX(item) : 0;
	case 16:
		item = FindItem("Trap");
		return item ? ITEM_INDEX(item) : 0;
	case 17:
		item = FindItem("ETF Rifle");
		return item ? ITEM_INDEX(item) : 0;
	case 18:
		item = FindItem("Plasma Beam");
		return item ? ITEM_INDEX(item) : 0;
	case 19:
		item = FindItem("Prox Launcher");
		return item ? ITEM_INDEX(item) : 0;
	case 20:
		item = FindItem("Chainfist");
		return item ? ITEM_INDEX(item) : 0;
	case 21:
		item = FindItem("Tesla");
		return item ? ITEM_INDEX(item) : 0;
	default:	return 0;
	}
}

int G_GetAmmoIndexByWeaponIndex (int weapon_index)
{
	gitem_t *item = NULL;

	//gi.dprintf("weapon_index=%d\n", weapon_index);

	if (!weapon_index)
		return 0;
	else if (weapon_index == ITEM_INDEX(Fdi_SHOTGUN))
		return shell_index;
	else if (weapon_index == ITEM_INDEX(Fdi_SUPERSHOTGUN))
		return shell_index;
	else if (weapon_index == ITEM_INDEX(Fdi_MACHINEGUN))
		return bullet_index;
	else if (weapon_index == ITEM_INDEX(Fdi_CHAINGUN))
		return bullet_index;
	else if (weapon_index == ITEM_INDEX(Fdi_GRENADELAUNCHER))
		return grenade_index;
	else if (weapon_index == ITEM_INDEX(Fdi_ROCKETLAUNCHER))
		return rocket_index;
	else if (weapon_index == ITEM_INDEX(Fdi_HYPERBLASTER))
		return cell_index;
	else if (weapon_index == ITEM_INDEX(Fdi_RAILGUN))
		return slug_index;
	else if (weapon_index == ITEM_INDEX(Fdi_BFG))
		return cell_index;
	else if (weapon_index == ITEM_INDEX(Fdi_20MM))
		return shell_index;
	item = FindItem("Ionripper");
	if (item && weapon_index == ITEM_INDEX(item))
		return cell_index;
	item = FindItem("Phalanx");
	if (item && weapon_index == ITEM_INDEX(item))
		return magslug_index;
	item = FindItem("Trap");
	if (item && weapon_index == ITEM_INDEX(item))
		return trap_index;
	item = FindItem("ETF Rifle");
	if (item && weapon_index == ITEM_INDEX(item))
	{
		gitem_t *flechettes = FindItem("Flechettes");
		return flechettes ? ITEM_INDEX(flechettes) : bullet_index;
	}
	item = FindItem("Plasma Beam");
	if (item && weapon_index == ITEM_INDEX(item))
		return cell_index;
	item = FindItem("Prox Launcher");
	if (item && weapon_index == ITEM_INDEX(item))
		return grenade_index;
	item = FindItem("Chainfist");
	if (item && weapon_index == ITEM_INDEX(item))
		return 0;
	item = FindItem("Tesla");
	if (item && weapon_index == ITEM_INDEX(item))
		return tesla_index;
	item = FindItem("Disruptor");
	if (item && weapon_index == ITEM_INDEX(item))
		return disruptor_index;
	if (weapon_index == grenade_index)
		return grenade_index;
	return 0;
}

float AmmoLevel (edict_t *ent, int ammo_index)
{
	int	max;

	// unknown weapon, return full ammo status
	if (!ammo_index)
		return 1.0;

	// check for max ammo of current weapon
	max = MaxAmmoType(ent, ammo_index);

	// can't determine max ammo, so return full ammo status
	if (!max)
		return 1.0;

	// return ratio of current ammo to max
	return ((float)ent->client->pers.inventory[ammo_index]/max);
}


int* packItems ;
bool depot_used[MAX_CLIENTS];

void vrx_depot_init()
{
	packItems = (int*)gi.TagMalloc((int)maxclients->value * MAX_ITEMS * sizeof(int32_t), TAG_LEVEL);
	memset(depot_used, 0, sizeof(depot_used));
}

int* vrx_depot_alloc()
{
	for (int i = 0; i < maxclients->value; i++)
	{
		if (!depot_used[i])
		{
			depot_used[i] = true;
			auto ret = packItems + i * MAX_ITEMS;
			memset(ret, 0, sizeof(int32_t) * MAX_ITEMS);
			return ret;
		}
	}
	return NULL;
}

void vrx_depot_free(int* pi) {
	auto idx = pi - packItems;
	if (idx >= 0 && idx < maxclients->value * MAX_ITEMS)
		depot_used[idx / MAX_ITEMS] = false;
}

#define DEPOT_BUILD_TIME	1.0
#define DEPOT_DELAY			1.0

qboolean depot_isvalid (edict_t *depot)
{
	return (depot && depot->inuse && depot->mtype == M_SUPPLYSTATION && depot->deadflag != DEAD_DEAD);
}

void depot_remove (edict_t *self, edict_t *owner, qboolean effect)
{
	// make sure this is a valid depot that were are trying to remove
	if (!depot_isvalid(self))
		return;

	// if owner is specified, make sure this is our depot
	if (owner && (!self->creator || !self->creator->inuse || self->creator != owner))
	{
		if (owner->client)
			owner->client->supplystation = NULL; // always clear the pointer
		return;
	}

	// clear our creator's pointer
	if (self->creator && self->creator->inuse)
	{
		// remove from HUD
		if (self->creator->client) {
			self->creator->client->supplystation = NULL;
			layout_remove_tracked_entity(&self->creator->client->layout, self);
		}
	}
	AI_EnemyRemoved(self);
	vrx_depot_free(self->packitems);

	if (effect)
		self->think = BecomeExplosion1;
	else
		self->think = G_FreeEdict;

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->svflags |= SVF_NOCLIENT;
	self->solid = SOLID_NOT;
	self->nextthink = level.time + FRAMETIME;
}

void depot_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (attacker->client)
		safe_cprintf(self->creator, PRINT_HIGH, "Your supply station was destroyed by %s.\n", attacker->client->pers.netname);
	else
		safe_cprintf(self->creator, PRINT_HIGH, "Your supply station was destroyed.\n");
	depot_remove(self, NULL, true);
}

void depot_effects (edict_t *self)
{
	self->s.effects = self->s.renderfx = 0;

	self->s.effects |= EF_COLOR_SHELL;
	if (level.framenum > self->count)
		self->s.renderfx |= RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_YELLOW;
	else if (self->creator->teamnum == RED_TEAM || ffa->value)
		self->s.renderfx |= RF_SHELL_RED;
	else
		self->s.renderfx |= RF_SHELL_BLUE;
}

void depot_add_item (edict_t *self, int item_index, int amount, int max)
{
	self->packitems[item_index] += amount;
	if (self->packitems[item_index] > max)
		self->packitems[item_index] = max;
	//gi.dprintf("%d: added %d, total: %d/%d\n", item_index, amount, self->packitems[item_index], max);
}

int depot_getcount (edict_t *self, int item_index)
{
	return self->packitems[item_index];
}

int depot_getmax (edict_t *self, int item_index)
{
	int skill_level;

	skill_level = self->monsterinfo.level
                  *
                  (1.0 + (0.2 * vrx_get_talent_level(self->creator, TALENT_STORAGE_UPGRADE)));//Talent: Storage Upgrade

	// body armor
	if (item_index == body_armor_index)
		return (100 * skill_level);

	// bullets and cells (200-1200 max)
	if (item_index == bullet_index || item_index == cell_index)
		return (100 * skill_level);

	// shells (100-600 max)
	if (item_index == shell_index)
		return (50 * skill_level);

	// rockets, grenades, and slugs (50-300 max)
	if (item_index == rocket_index || item_index == grenade_index || item_index == slug_index)
		return 25 * skill_level;

	return 0;
}

float depot_get_fraction_full(edict_t* self)
{
	int	current_total = 0, max_total = 0;

	current_total += depot_getcount(self, body_armor_index);
	max_total += depot_getmax(self, body_armor_index);
	current_total += depot_getcount(self, bullet_index);
	max_total += depot_getmax(self, bullet_index);
	current_total += depot_getcount(self, cell_index);
	max_total += depot_getmax(self, cell_index);
	current_total += depot_getcount(self, shell_index);
	max_total += depot_getmax(self, shell_index);
	current_total += depot_getcount(self, grenade_index);
	max_total += depot_getmax(self, grenade_index);
	current_total += depot_getcount(self, rocket_index);
	max_total += depot_getmax(self, rocket_index);
	current_total += depot_getcount(self, slug_index);
	max_total += depot_getmax(self, slug_index);

	//gi.dprintf("%d %d %.1f\n", current_total, max_total, (float)current_total / max_total);
	return (float)current_total / max_total;
}

void depot_add_inventory (edict_t *self, int frames)
{
	int	current_total = 0, max_total = 0;
	int amount, max_amount, skill_level = self->monsterinfo.level;

	if (level.framenum > self->count)
	{
		// add body armor
		amount = skill_level * 10;
		max_amount = depot_getmax(self, body_armor_index);
		depot_add_item(self, body_armor_index, amount, max_amount);
		// keep track of grand totals
		//max_total += max_amount;
		//current_total += depot_getcount(self, body_armor_index);

		// add bullets and cells (200-1200 max)
		amount = skill_level * 10;
		max_amount = depot_getmax(self, bullet_index);
		depot_add_item(self, bullet_index, amount, max_amount);
		depot_add_item(self, cell_index, amount, max_amount);
		// keep track of grand totals
		//max_total += 2 * max_amount;
		//current_total += depot_getcount(self, bullet_index);
		//current_total += depot_getcount(self, cell_index);

		// add shells (100-600 max)
		amount = skill_level * 5;
		max_amount = depot_getmax(self, shell_index);
		depot_add_item(self, shell_index, amount, max_amount);
		// keep track of grand totals
		//max_total += max_amount;
		//current_total += depot_getcount(self, shell_index);

		// add rockets, grenades, and slugs (50-300 max)
		amount = skill_level * 2.5;
		max_amount = depot_getmax(self, rocket_index);
		depot_add_item(self, grenade_index, amount, max_amount);
		depot_add_item(self, rocket_index, amount, max_amount);
		depot_add_item(self, slug_index, amount, max_amount);
		// keep track of grand totals
		//max_total += 3 * max_amount;
		//current_total += depot_getcount(self, grenade_index);
		//current_total += depot_getcount(self, rocket_index);
		//current_total += depot_getcount(self, slug_index);

		self->count = level.framenum + frames;
		//self->wait = 100 * ((float)current_total / max_total);//for HUD display
		//gi.dprintf("%d/%d = %.1f %d\n", current_total, max_total, self->wait, (int)self->wait);
	}
	// store percentage full for hud display
	self->wait = 100 * depot_get_fraction_full(self);
}

void depot_think (edict_t *self)
{
	if (!G_EntIsAlive(self->creator))
	{
		depot_remove(self, NULL, false);
		return;
	}

	depot_effects(self);
	depot_add_inventory(self, qf2sf(100));

	self->nextthink = level.time + FRAMETIME;
}

int depot_give_item (edict_t *self, edict_t *other, int item_index)
{
	int max, add, *p_inv, *d_inv;

	// max value that player can hold
	if (item_index == body_armor_index)
		max = MAX_ARMOR(other); 
	else
		max = MaxAmmoType(other, item_index);

	p_inv = &other->client->pers.inventory[item_index]; // player current inventory count
	d_inv = &self->packitems[item_index]; // depot current inventory count
	add = max - *p_inv; // amount player needs

	// the player can't carry any more ammo
	if (add < 0)
		return 0;

	// can't add more than the depot has
	if (add > *d_inv) 
		add = *d_inv;

	*p_inv += add; // add to player inventory
	*d_inv -= add; // reduce depot inventory

	//gi.dprintf("player: %d/%d depot: %d add: %d\n", *p_inv, max, *d_inv, add);

	return add;
}

void depot_give_inventory (edict_t *self, edict_t *other)
{
	int result = 0;
	int qty_armor, qty_bullets, qty_cells, qty_shells, qty_grenades, qty_rockets, qty_slugs;

	if ((self->sentrydelay > level.time) || !G_EntIsAlive(self) || !G_EntIsAlive(other) || !other->client || OnSameTeam(self, other) < 2)
		return;

	result += qty_armor = depot_give_item(self, other, body_armor_index);
	result += qty_bullets = depot_give_item(self, other, bullet_index);
	result += qty_cells = depot_give_item(self, other, cell_index);
	result += qty_shells = depot_give_item(self, other, shell_index);
	result += qty_grenades = depot_give_item(self, other, grenade_index);
	result += qty_rockets = depot_give_item(self, other, rocket_index);
	result += qty_slugs = depot_give_item(self, other, slug_index);

	safe_cprintf(other, PRINT_HIGH, "You received %d armor, %d bullets, %d cells, %d shells, %d grenades, %d rockets, %d slugs.\n",
		qty_armor, qty_bullets, qty_cells, qty_shells, qty_grenades, qty_rockets, qty_slugs);

	safe_cprintf(other, PRINT_HIGH, "Depot has %d armor, %d bullets, %d cells, %d shells, %d grenades, %d rockets, %d slugs remaining.\n", 
		self->packitems[body_armor_index], self->packitems[bullet_index], self->packitems[cell_index], 
		self->packitems[shell_index], self->packitems[grenade_index], self->packitems[rocket_index], 
		self->packitems[slug_index]);

	// delay before depot can be used again
	self->sentrydelay = level.time + 2.0;

	if (result > 0) {
		gi.sound(self, CHAN_ITEM, gi.soundindex("misc/w_pkup.wav"), 1, ATTN_STATIC, 0);
	
		other->supply_exp_owner = self->creator;
		if ( other->supply_exp_time < level.time )
			other->supply_exp_time = level.time;
		if ( other->supply_exp_time < level.time + 60.0 ) {
			other->supply_exp_time += 30.0;
		}
	}
}

void depot_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	V_Touch(ent, other, plane, surf);
	depot_give_inventory(ent, other);
}

edict_t *BuildDepot (edict_t *ent, float skill_mult, float delay_mult)
{
	edict_t *station;

	station = G_Spawn();
	station->creator = ent;
	station->think = depot_think;
	station->nextthink = level.time + DEPOT_BUILD_TIME * delay_mult;
	station->s.modelindex = gi.modelindex ("models/objects/depot/tris.md2");
	station->s.effects |= EF_PLASMA;
	station->s.renderfx |= RF_IR_VISIBLE;
	station->solid = SOLID_BBOX;
	station->movetype = MOVETYPE_TOSS;
	station->clipmask = MASK_MONSTERSOLID;
	station->mass = 500;
	station->mtype = M_SUPPLYSTATION;//FIXME: change this
	station->classname = "depot";
	station->takedamage = DAMAGE_YES;
	station->health = station->max_health =  500;//FIXME: change this
	station->monsterinfo.level = ent->myskills.abilities[SUPPLY_STATION].current_level * skill_mult;
	station->touch = depot_touch;
	station->die = depot_die;
	station->packitems = vrx_depot_alloc();
	if (!station->packitems)
	{
		G_FreeEdict(station);
		return NULL;
	}
	VectorSet(station->mins, -18, -18, -22);
	VectorSet(station->maxs, 18, 18, 0);
	gi.linkentity(station);//FIXME: do we need this since we haven't set final position?

	ent->client->supplystation = station;

	return station;
}

void Cmd_CreateSupplyStation_f (edict_t *ent)
{
	edict_t *depot;
	vec3_t	start;

	int talentLevel, cost=STATION_COST;
	float skill_mult=1.0, cost_mult=1.0, delay_mult=1.0;//Talent: Rapid Assembly & Precision Tuning

	if (debuginfo->value)
		gi.dprintf("%s just called Cmd_CreateSupplyStation_f\n", ent->client->pers.netname);

	if (ent->client->supplystation)
	{
		depot_remove(ent->client->supplystation, ent, true);
		return;
	}

	if(ent->myskills.abilities[SUPPLY_STATION].disable)
		return;

	//Talent: Rapid Assembly
    talentLevel = vrx_get_talent_level(ent, TALENT_RAPID_ASSEMBLY);
	if (talentLevel > 0)
		delay_mult -= 0.1 * talentLevel;
	//Talent: Precision Tuning
    else if ((talentLevel = vrx_get_talent_level(ent, TALENT_PRECISION_TUNING)) > 0)
	{
		cost_mult += PRECISION_TUNING_COST_FACTOR * talentLevel;
		delay_mult += PRECISION_TUNING_DELAY_FACTOR * talentLevel;
		skill_mult += PRECISION_TUNING_SKILL_FACTOR * talentLevel;
	}
	cost *= cost_mult;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[SUPPLY_STATION].current_level, cost))
		return;
	//BuildSupplyStation(ent, cost, skill_mult, delay_mult);

	depot = BuildDepot(ent, skill_mult, delay_mult);
	if (!G_GetSpawnLocation(ent, 100, depot->mins, depot->maxs, start, NULL, PROJECT_HITBOX_FAR, false))
	{
		safe_cprintf(ent, PRINT_HIGH, "Not enough room to spawn supply station.\n");
		ent->client->supplystation = NULL;
		G_FreeEdict(depot);
		return;
	}

	VectorCopy(start, depot->s.origin);
	VectorCopy(ent->s.angles, depot->s.angles);
	depot->s.angles[PITCH] = 0;
	depot->s.angles[ROLL] = 0;
	gi.linkentity(depot);

	ent->client->ability_delay = level.time + DEPOT_DELAY * delay_mult;
	ent->client->pers.inventory[power_cube_index] -= cost;
	ent->holdtime = level.time + DEPOT_BUILD_TIME * delay_mult;
	layout_add_tracked_entity(&ent->client->layout, depot); // add to HUD
	AI_EnemyAdded(depot);
	gi.sound(depot, CHAN_ITEM, gi.soundindex("weapons/repair.wav"), 1, ATTN_NORM, 0);
}
