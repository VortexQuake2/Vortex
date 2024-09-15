#include "g_local.h"
#include "../../gamemodes/ctf.h"

//3.0 matrix jump
void cmd_mjump(edict_t *ent)
{

    if (ent->holdtime > level.time)
        return; // can't use abilities

    if (vrx_has_flag(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "Can't use this while carrying the flag!\n");
        return;
    }

    if ((!(ent->v_flags & SFLG_MATRIXJUMP)) && (ent->velocity[2] == 0) && (!(ent->v_flags & SFLG_UNDERWATER))) {
        item_t *slot;
        int i;
        qboolean found = false;

		//Find item in inventory
		for (i = 3; i < MAX_VRXITEMS; ++i)
		{
			if (ent->myskills.items[i].itemtype & ITEM_GRAVBOOTS)
			{
				slot = &ent->myskills.items[i];
				found = true;
				break;
			}
		}

		//no boots? no jump
		if (!found) return;

		ent->v_flags ^= SFLG_MATRIXJUMP;
		ent->velocity[2] += MJUMP_VELOCITY;

		//Consume a charge
		if (!(ent->myskills.items[i].itemtype & ITEM_UNIQUE))
			ent->myskills.items[i].quantity -= 1;

		//if out of charges, erase the item
		if (ent->myskills.items[i].quantity == 0)
		{
			int count = 0;
			V_ItemClear(&ent->myskills.items[i]);
			//Alert the player
            safe_cprintf(ent, PRINT_HIGH, "Your anti-gravity boots have broken!\n");
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/itembreak.wav"), 1, ATTN_NORM, 0);

			//Check for more boots (backup items)
			for (i = 3; i < MAX_VRXITEMS; ++i)
				if (ent->myskills.items[i].itemtype & ITEM_GRAVBOOTS)
					++count;
			safe_cprintf(ent, PRINT_HIGH, "Boots left: %d.\n", count);

		}
		else gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/gravjump.wav"), 1, ATTN_NORM, 0);
		PlayerNoise(ent, ent->s.origin, PNOISE_SELF);

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}
}
//end matrix jump

qboolean CheckAuraOwner (edict_t *self, int aura_cost);

/*
void Cmd_AmmoStealer_f(edict_t *ent)
{
	edict_t		*other = NULL;
	int			shells=0, bullets=0, rockets=0, slugs=0, cells=0;
	float		steal_base;	//Base multiplier for ammo steal
	int			skill_level;
	qboolean	foundtarget = false;
	vec3_t		start;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_AmmoStealer_f()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[AMMO_STEAL].disable)
		return;

	//3.0 new ammo steal algorithm, more efficient less buggy (no ammo lost during steal)
	if (!G_CanUseAbilities(ent, ent->myskills.abilities[AMMO_STEAL].current_level, COST_FOR_STEALER))
		return;

	G_GetSpawnLocation(ent, 512, vec3_origin, vec3_origin, start);

	//Search for targets
	while ((other = findradius(other, start, 128)) != NULL)
	{
		if (other == ent)
			continue;
		if (!other->inuse)
			continue;
		if (!other->takedamage)
			continue;
		if (other->solid == SOLID_NOT)
			continue;
		if (!other->client)
			continue;
		if (OnSameTeam(ent, other))
			continue;
		if (!visible(ent, other))
			continue;

		//A target has been found
		foundtarget = true;

		//Calculate the steal multiplier (random amount for each target)
		skill_level = ent->myskills.abilities[AMMO_STEAL].current_level;
		//min = 6.25%/lvl, max = 7.14%/lvl
		steal_base = GetRandom((int)(skill_level / 16.0 * 100.0), (int)(skill_level / 14.0 * 100.0)) / 100.0;
		if (steal_base > 0.9)	steal_base = 0.9; //max at 90%

		//Start stealing ammo
		shells	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]	* steal_base;
		bullets += other->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))]	* steal_base;
		rockets += other->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))]	* steal_base;
		slugs	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))]		* steal_base;
		cells	+= other->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))]		* steal_base;
		
		//This next part does the same thing as subtracting the amount stolen from the target
		other->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))]	*= (1 - steal_base);
		other->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))]	*= (1 - steal_base);

		//Play the spell sound!
		gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/telekinesis.wav"), 1, ATTN_NORM, 0);

		//Notify the two clients
		safe_cprintf(other, PRINT_HIGH, "%s just stole some of your ammo!\n", ent->client->pers.netname);
		safe_cprintf(ent, PRINT_HIGH, "You just stole %d%% of %s's ammo!\n", (int)(steal_base * 100), other->client->pers.netname);

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
	}
	//Add what we stole from the other people. :)
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] += shells;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] += bullets;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] += rockets;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))] += slugs;
	ent->client->pers.inventory[ITEM_INDEX(FindItem("Cells"))] += cells;
	Check_full(ent);//Make sure we are not over our limits

	if(foundtarget)
	{
		ent->client->ability_delay = level.time + DELAY_AMMOSTEAL;
		ent->client->pers.inventory[power_cube_index] -= COST_FOR_STEALER;
	}
}
*/

void Cmd_BoostPlayer(edict_t *ent)
{
	edict_t		*other = NULL;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	float		boost_delay = DELAY_BOOST;
	int			talentLevel;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_BoostPlayer()\n", ent->client->pers.netname);

    if (ent->myskills.abilities[BOOST_SPELL].disable)
        return;

    //4.07 can't boost while being hurt
    if (ent->lasthurt + DAMAGE_ESCAPE_DELAY > level.time)
        return;

    if (!V_CanUseAbilities(ent, BOOST_SPELL, COST_FOR_BOOST, true))
        return;

    if (vrx_has_flag(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "Can't use this ability while carrying the flag!\n");
        return;
    }

//    if (ent->client->snipertime >= level.time) {
//        safe_cprintf(ent, PRINT_HIGH, "You can't use boost while trying to snipe!\n");
//        return;
//    }

	//Talent: Mobility
    talentLevel = vrx_get_talent_level(ent, TALENT_MOBILITY);
	switch(talentLevel)
	{
	case 1:		boost_delay -= 0.1;	break;
	case 2:		boost_delay -= 0.3;		break;
	case 3:		boost_delay -= 0.5;		break;
	case 4:     boost_delay -= 0.7;     break;
	case 5:     boost_delay -= 1.0;     break;
	default:	//Do nothing
		break;
	}

	ent->client->pers.inventory[power_cube_index] -= COST_FOR_BOOST;
	ent->client->ability_delay = level.time + boost_delay;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  ent->viewheight-8);

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	
	//4.2 player-monster support
	if (PM_PlayerHasMonster(ent))
	{
		other = ent->owner;
		other->monsterinfo.air_frames = 1;// used to turn off velocity restriction
	}
	else
		other = ent;

	other->velocity[0] += forward[0] * 1000;
	other->velocity[1] += forward[1] * 1000;
	other->velocity[2] += forward[2] * 1000;

	if (other->groundentity)
		other->velocity[2] += 300;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	//Talent: Leap Attack
    if (vrx_get_talent_level(ent, TALENT_LEAP_ATTACK) > 0)
	{
		ent->client->boosted = true;
        gi.sound(ent, CHAN_VOICE, gi.soundindex(va("abilities/leapattack%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
	}
	else
		gi.sound (ent, CHAN_BODY, gi.soundindex("weapons/rockfly.wav"), 1, ATTN_NORM, 0);
}

void LeapAttack (edict_t *ent)
{
	float	duration;
	vec3_t	point, forward;
	edict_t *e=NULL;

	if (!ent->client->boosted)
		return;

    duration = 0.2 * vrx_get_talent_level(ent, TALENT_LEAP_ATTACK);

	while ((e = findradius (e, ent->s.origin, 128)) != NULL)
	{
		if (!G_ValidTarget(ent, e, true, true))
			continue;
		if (!infront(ent, e))
			continue;

		G_EntMidPoint(e, point);
		VectorSubtract(point, ent->s.origin, forward);
		T_Damage(e, e, ent, forward, point, vec3_origin, 0, 1000, 0, 0);

		gi.sound (e, CHAN_VOICE, gi.soundindex (va("misc/stun%d.wav", GetRandom(1, 3))), 1, ATTN_NORM, 0);
		
		// stun them
		if (e->client)
		{
			e->holdtime = level.time + duration;
			e->client->ability_delay = level.time + duration;
		}
		else
			e->nextthink = level.time + duration;
	}

	ent->client->boosted = false;
}

void BecomeExplosion2 (edict_t *self);
void Cmd_CorpseExplode(edict_t *ent)
{
	int		damage, min_dmg, max_dmg, slvl;
	float	fraction, radius;
	vec3_t	start, end, forward, right, offset;
	trace_t	tr;
	edict_t *e=NULL;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_CorpseExplode()\n", ent->client->pers.netname);

	if(ent->myskills.abilities[CORPSE_EXPLODE].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[CORPSE_EXPLODE].current_level, COST_FOR_CORPSEEXPLODE))
		return;

	slvl = ent->myskills.abilities[CORPSE_EXPLODE].current_level;
	radius = CORPSE_EXPLOSION_INITIAL_RADIUS + CORPSE_EXPLOSION_ADDON_RADIUS * slvl;

	// calculate starting position
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	VectorMA(start, CORPSE_EXPLOSION_MAX_RANGE, forward, end);
	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID);

	while ((e = findclosestradius (e, tr.endpos, CORPSE_EXPLOSION_SEARCH_RADIUS)) != NULL)
	{
		if (!G_EntExists(e))
			continue;
		if (e->health > 0)
			continue;
		if (e->max_health < 1)
			continue;

		// kill the corpse
		T_Damage(e, e, ent, vec3_origin, e->s.origin, vec3_origin, 10000, 0, DAMAGE_NO_PROTECTION, MOD_CORPSEEXPLODE);

		// inflict damage
		fraction = 0.1 * GetRandom(5, 10);
		damage = fraction * e->max_health;
		
		// calculate min/max damage range
		min_dmg = CORPSE_EXPLOSION_INITIAL_DAMAGE + CORPSE_EXPLOSION_ADDON_DAMAGE * slvl;
		max_dmg = 5 * min_dmg;

		if (damage < min_dmg)
			damage = min_dmg;
		else if (damage > max_dmg)
			damage = max_dmg;

		T_RadiusDamage (e, ent, damage, NULL, radius, MOD_CORPSEEXPLODE);

		//gi.dprintf("fraction %.1f, damage %d, max_health: %d\n", fraction, damage, e->max_health);

		//Spells like corpse explode shouldn't display 10000 damage, so show the corpse damage instead
		ent->client->ps.stats[STAT_ID_DAMAGE] = damage;

        gi.sound(e, CHAN_ITEM, gi.soundindex("abilities/corpseexplodecast.wav"), 1, ATTN_NORM, 0);
		ent->client->pers.inventory[power_cube_index] -= COST_FOR_CORPSEEXPLODE;
		ent->client->ability_delay = level.time + DELAY_CORPSEEXPLODE;
		safe_cprintf(ent, PRINT_HIGH, "Corpse exploded for %d damage!\n", damage);

		//decino: explosion effect
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_GRENADE_EXPLOSION);
		gi.WritePosition (e->s.origin);
		gi.multicast (e->s.origin, MULTICAST_PVS);

		//decino: shoot 6 gibs that deal damage
		//az: todo, more copypaste.
		/*for (i = 0; i < 10; i++)
		{
			e->s.angles[YAW] += 36;
			AngleCheck(&e->s.angles[YAW]);

			AngleVectors(e->s.angles, forward, NULL, up);
			fire_gib(ent, e->s.origin, forward, dmg, 0, 1000);
		}*/

		// calling entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;

		break;
	}
}


void TeleportPlayer (edict_t *player)
{
	int dist = 512;
	vec3_t forward, start, end;
	trace_t tr;

	//GHz: Calculate end point and trace
	VectorCopy(player->s.origin, start);
	start[2]++;
	AngleVectors (player->s.angles, forward, NULL, NULL);
	//GHz: Keep trying until we get a valid spot
	while (dist > 0){
		VectorMA(start, dist, forward, end);
		tr = gi.trace(start, player->mins, player->maxs, end, player, MASK_SOLID);
		if (!(tr.contents & MASK_SOLID) && tr.fraction == 1.0)
		{
			player->s.event = EV_PLAYER_TELEPORT;
			VectorCopy(tr.endpos, player->s.origin);

			//GHz: Hold in place briefly
			player->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
			player->client->ps.pmove.pm_time = 14;
			//player->client->respawn_time = level.time;

			dist = 0;//GHz: We found a spot, so we're done
			VectorClear(player->velocity);
			gi.linkentity(player);
		}
		else
			dist -= 10;//GHz: No luck, so try further back
	}

}

qboolean ValidTeleportSpot (edict_t *ent, vec3_t spot)
{
	vec3_t	start, forward, right;
	trace_t	tr;

	AngleVectors(ent->s.angles, forward, right, NULL);

	// check above
	VectorCopy(spot, start);
	start[2] += 128;
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check left
	VectorMA(spot, -128, right, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check right
	VectorMA(spot, 128, right, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check forward
	VectorMA(spot, 128, forward, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	// check behind
	VectorMA(spot, -128, forward, start);
	tr = gi.trace(spot, NULL, NULL, start, ent, MASK_SOLID);
	if (tr.surface && (tr.surface->flags & SURF_SKY))
		return false;

	return true;
}

void TeleportForward (edict_t *ent) {
    int dist = 512;
    vec3_t angles, offset, forward, right, start, end;
    trace_t tr;

    if (!G_EntIsAlive(ent))
        return;
    //4.07 can't teleport while being hurt
    if (ent->lasthurt + DAMAGE_ESCAPE_DELAY > level.time)
        return;
    if (vrx_has_flag(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "Can't use this while carrying the flag!\n");
        return;
    }

    if (!V_CanUseAbilities(ent, TELEPORT, TELEPORT_COST, true))
        return;

    // calling entity made a sound, used to alert monsters
    ent->lastsound = level.framenum;

	RemoveAllDrones(ent, false);
	RemoveHellspawn(ent);
	RemoveMiniSentries(ent);
//	vrx_remove_player_summonables(ent);

	// get forward vector
	if (!ent->client)
	{
		VectorCopy(ent->s.angles, angles);
		angles[PITCH] = 0;
		AngleVectors(angles, forward, NULL, NULL);
	}
	else
	{
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 0, 7,  ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	}
	// get starting position
	VectorCopy(ent->s.origin, start);
	start[2]++;

	// in CTF, you can't teleport thru solid objects
	/*
	if (ctf->value)
	{
		VectorMA(start, dist, forward, end);
		tr = gi.trace(start, ent->mins, ent->maxs, end, ent, MASK_SHOT);

		ent->s.event = EV_PLAYER_TELEPORT;
		VectorCopy(tr.endpos, ent->s.origin);
		VectorCopy(tr.endpos, ent->s.old_origin);
		VectorClear(ent->velocity);
		gi.linkentity(ent);
		ent->client->pers.inventory[power_cube_index]-=TELEPORT_COST;
		ent->client->ability_delay = level.time + 1;
		return;
	}
	*/

	// keep trying to teleport until there is no room left
	while (dist > 0)
	{
		edict_t *teleport_target;

		//4.2 support player-monsters
		if (PM_PlayerHasMonster(ent))
			teleport_target = ent->owner;
		else
			teleport_target = ent;

		VectorMA(start, dist, forward, end);

		// if there is a nearby sky brush, we shouldn't teleport there
		if (!ValidTeleportSpot(teleport_target, end))
		{
			// try further back
			dist -= 8;
			continue;
		}

		tr = gi.trace(end, teleport_target->mins, teleport_target->maxs, end, teleport_target, MASK_SHOT);

		// is this a valid position?
		if (!(tr.contents & MASK_SHOT) && !(gi.pointcontents(end) & MASK_SHOT) && !tr.allsolid)
		{
			teleport_target->s.event = EV_PLAYER_TELEPORT;
			VectorCopy(end, teleport_target->s.origin);
			VectorCopy(end, teleport_target->s.old_origin);
			VectorClear(teleport_target->velocity);
			gi.linkentity(teleport_target);
			ent->client->pers.inventory[power_cube_index]-=TELEPORT_COST;

			// 3.14 prevent teleport+nova spam
			ent->client->ability_delay = level.time + (ctf->value?1.5:0.7);
			return;
		}
		else
		{
			// try further back
			dist-=8;
		}
	}
}

void NovaExplosionEffect(vec3_t org);

void CrippleAttack (edict_t* ent, int cripple_level)
{
	float		damage, min_health, delta;
	edict_t* target = NULL;

	while ((target = findradius(target, ent->s.origin, CRIPPLE_RANGE)) != NULL)
	{
		if (!G_ValidTargetEnt(target, false) || OnSameTeam(ent, target))
			continue;
		// limit maximum damage inflicted to bosses and others to a percentage of max health
		if (target->monsterinfo.control_cost >= 3)
			min_health = 0.2 * target->max_health;
		else
			min_health = 0.1 * target->max_health;
		// calculate damage
		damage = target->health * (1 - (1 / (1 + 0.1 * cripple_level)));
		// delta is the difference between current health and minimum health
		delta = target->health - min_health;
		// are we above our minimum?
		if (delta > 0)
		{
			if (damage > delta)
				damage = delta;
			if (damage > CRIPPLE_MAX_DAMAGE)
				damage = CRIPPLE_MAX_DAMAGE;
			T_Damage(target, ent, ent, vec3_origin, target->s.origin, vec3_origin, damage, 0, DAMAGE_NO_ABILITIES, MOD_CRIPPLE);

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_TELEPORT_EFFECT);
			gi.WritePosition(target->s.origin);
			gi.multicast(target->s.origin, MULTICAST_PVS);
		}

	}

	// write a nice effect so everyone knows we've cast a spell
	/*
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);*/

	NovaExplosionEffect(ent->s.origin);
	gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/novaelec.wav"), 1, ATTN_NORM, 0);

	if (ent->client)
	{
		ent->client->ability_delay = level.time + CRIPPLE_DELAY;
		ent->client->pers.inventory[power_cube_index] -= CRIPPLE_COST;
	}
	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}
/*
void CrippleAttack (edict_t *ent)
{
	int		damage;
	vec3_t	end;
	trace_t	tr;
	edict_t	*e=NULL;

	while ((e = findclosestreticle(e, ent, CRIPPLE_RANGE)) != NULL)
	{
		if (!G_ValidTarget(ent, e, true))
			continue;
		if (!visible(ent, e))
			continue;
		if (!infront(ent, e))
			continue;

		damage = e->health * (1-(1/(1+0.2*ent->myskills.abilities[CRIPPLE].current_level)));
		if (damage > CRIPPLE_MAX_DAMAGE)
			damage = CRIPPLE_MAX_DAMAGE;
		T_Damage(e, ent, ent, vec3_origin, e->s.origin, vec3_origin, damage, 0, DAMAGE_NO_ABILITIES, MOD_CRIPPLE);

		VectorCopy(e->s.origin, end);
		end[2] += 8192;
		tr = gi.trace(e->s.origin, NULL, NULL, end, e, MASK_SHOT);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_MONSTER_HEATBEAM);
		gi.WriteShort (e-g_edicts);
		gi.WritePosition (e->s.origin);
		gi.WritePosition (tr.endpos);
		gi.multicast (e->s.origin, MULTICAST_PVS);
		break;
	}

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/eleccast.wav"), 1, ATTN_NORM, 0);
	ent->client->ability_delay = level.time + CRIPPLE_DELAY;
	ent->client->pers.inventory[power_cube_index]-=CRIPPLE_COST;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}
*/

void Cmd_Cripple_f (edict_t *ent)
{
	int	ability_level=ent->myskills.abilities[CRIPPLE].current_level;

	if (!G_CanUseAbilities(ent, ability_level, CRIPPLE_COST))
		return;
	if (ent->myskills.abilities[CRIPPLE].disable)
		return;

	//3.0 amnesia disables cripple
	if (que_findtype(ent->curses, NULL, AMNESIA) != NULL)
		return;
	CrippleAttack(ent, ability_level);
}

void lightningbolt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (G_EntExists(other))
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, 
			plane->normal, self->dmg, self->dmg, 0, MOD_LIGHTNING);
	G_FreeEdict(self);
}

void lightningbolt_think (edict_t *self)
{
	// free self if our time is up
	if (level.time >= self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	// rotate animation frames
	self->s.frame++;
	if (self->s.frame > 2)
		self->s.frame = 0;

	//if (!(level.framenum%2))
	//	gi.dprintf("%f\n", VectorLength(self->velocity));

	self->nextthink = level.time + FRAMETIME;
}

void fire_lightningbolt (edict_t *ent, vec3_t start, vec3_t dir, float speed, int damage)
{
	edict_t *bolt;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;

	// create bolt
	bolt = G_Spawn();
	bolt->s.modelindex = gi.modelindex ("models/proj/beam/tris.md2");
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= EF_TAGTRAIL;
	//bolt->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = ent;
	bolt->think = lightningbolt_think;
	bolt->touch = lightningbolt_touch;
	bolt->nextthink = level.time + FRAMETIME;
	bolt->delay = level.time + 10;
	bolt->dmg = damage;
	bolt->classname = "lightning bolt";	
	gi.linkentity (bolt);

	/*
	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/

    gi.sound(ent, CHAN_ITEM, gi.soundindex("abilities/zap2.wav"), 1, ATTN_NORM, 0);
	//ent->client->ability_delay = level.time+BOLT_DELAY;
}

void ProjectileReverseCourse (edict_t *proj)
{
	VectorNegate(proj->movedir, proj->movedir);
	VectorNegate(proj->velocity, proj->velocity);
	vectoangles(proj->velocity, proj->s.angles);
	ValidateAngles(proj->s.angles);
}

void DeflectProjectile (edict_t *self, char *className, float chance, qboolean in_front)
{
	edict_t *e=NULL;

	while((e = G_Find(e, FOFS(classname), className)) != NULL)
	{
		// only use non-friendly projectiles
		if (e->owner && e->owner->inuse && OnSameTeam(self, e->owner))
			continue;

		//Talent: Repel
		// only deflect projectiles away from our front
		if (in_front && !infront(self, e))
			continue;

		// this projectiles has already been redirected back at its owner
		if (e->enemy && e->owner && (e->enemy == e->owner))
			continue;

		// calculate projectile speed and compare distance
		// if the speed is greater than the distance remaining, then we are going
		// to hit next frame, so we need to deflect this shot
		if (VectorLength(e->velocity)*FRAMETIME < entdist(self, e)-(G_GetHypotenuse(self->maxs)+1))
			continue;
		
		if (random() > chance)
			continue;

		// redirect projectile towards its owner
		if (e->owner && e->owner->inuse && e->owner->takedamage && visible(e, e->owner))
		{
			e->enemy = e->owner;
			e->owner = self; // take ownership
			ProjectileReverseCourse(e);
		}
	}	
}

void DeflectProjectiles (edict_t *self, float chance, qboolean in_front)
{
	DeflectProjectile(self, "rocket", chance, in_front);
	DeflectProjectile(self, "bolt", chance, in_front);
	DeflectProjectile(self, "bfg blast", chance, in_front);
	DeflectProjectile(self, "magicbolt", chance, in_front);
	DeflectProjectile(self, "grenade", chance, in_front);
	DeflectProjectile(self, "hgrenade", chance, in_front);
	DeflectProjectile(self, "skull", chance, in_front);
	DeflectProjectile(self, "spike", chance, in_front);
	DeflectProjectile(self, "spikey", chance, in_front);
	DeflectProjectile(self, "fireball", chance, in_front);
	DeflectProjectile(self, "plasma bolt", chance, in_front);
	DeflectProjectile(self, "acid", chance, in_front);
}



void Cmd_Antigrav_f (edict_t *ent)
{
	if ((!ent->inuse) || (!ent->client))
		return;
	if(ent->myskills.abilities[ANTIGRAV].disable)
		return;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[ANTIGRAV].current_level, 0))
		return;

    if (ent->antigrav == true) {
        safe_cprintf(ent, PRINT_HIGH, "Antigrav disabled.\n");
        ent->antigrav = false;
        return;
    }

    if (ent->myskills.abilities[ANTIGRAV].disable)
        return;

    if (vrx_has_flag(ent)) {
        safe_cprintf(ent, PRINT_HIGH, "Can't use this ability while carrying the flag!\n");
        return;
    }

    //3.0 amnesia disables super speed
    if (que_findtype(ent->curses, NULL, AMNESIA) != NULL)
        return;

    safe_cprintf(ent, PRINT_HIGH, "Antigrav enabled.\n");
	ent->antigrav= true;
}

void drop_make_touchable (edict_t *ent);
void meditate_think (edict_t *self)
{
	self->s.effects &= ~EF_PLASMA;
	self->think = drop_make_touchable;
	self->nextthink = level.time + FRAMETIME;
}

void Cmd_Meditate_f (edict_t *ent)
{
	//Talent: Meditation
    int talentLevel = vrx_get_talent_level(ent, TALENT_MEDITATION);

	if (talentLevel < 1)
		return;

	if (ent->client->ability_delay > level.time)
		return;

	ent->manacharging = !ent->manacharging;

	if (ent->manacharging)
	{
		safe_cprintf(ent, PRINT_HIGH, "Charging mana.\n");
	}
	else
	{
		safe_cprintf(ent, PRINT_HIGH, "No longer charging mana.\n");
		ent->client->ability_delay = level.time + 2;
	}

	// old meditation
	/*power_cube = FindItem("Power Cube");

	ent->client->pers.inventory[ITEM_INDEX(power_cube)] += 20*talentLevel;*/

	// ent->holdtime = level.time + 2;

	// ent->client->ability_delay = level.time + 2;
}


#define PURGE_COST		50
#define PURGE_DELAY		5.0

void Cmd_Purge_f (edict_t *ent)
{
	//Talent: Purge
    int talentLevel = vrx_get_talent_level(ent, TALENT_PURGE);

	if (talentLevel < 1)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to upgrade this talent before you can use it!\n");
		return;
	}
	
	if (ent->client->ability_delay > level.time)
	{
		safe_cprintf(ent, PRINT_HIGH, "You must wait %.1f seconds before you can use this talent.\n", 
			ent->client->ability_delay-level.time);
		return;
	}
	if (ent->client->pers.inventory[power_cube_index] < PURGE_COST)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need %d cubes before you can use this talent.\n", 
			PURGE_COST-ent->client->pers.inventory[power_cube_index]);
		return;
	}

	//Give them a short period of total immunity
	ent->client->invincible_framenum = (int)(level.framenum + 0.3f * talentLevel / FRAMETIME); //up to 2 seconds at level 5

	//Give them a short period of curse immunity
	ent->holywaterProtection = level.time + talentLevel; //up to 5 seconds at level 5

	//You can only drink 1/sec
	ent->client->ability_delay = level.time + PURGE_DELAY;

	//Remove all curses
	CurseRemove(ent, 0, 0);

	ent->client->pers.inventory[power_cube_index] -= PURGE_COST;
	ent->client->ability_delay = level.time + PURGE_DELAY;

    gi.sound(ent, CHAN_AUTO, gi.soundindex("abilities/purification.wav"), 1, ATTN_NORM, 0);
}


void Cmd_SelfDestruct_f(edict_t *self)
{
	int damage;

	if (!V_CanUseAbilities(self, SELFDESTRUCT, 0, true))
		return;

	damage = self->myskills.abilities[SELFDESTRUCT].current_level * SELFDESTRUCT_BONUS + SELFDESTRUCT_BASE;

	// do the damage
	T_RadiusDamage(self, self, damage, self, SELFDESTRUCT_RADIUS, MOD_SELFDESTRUCT);
	T_Damage(self, self, self, vec3_origin, self->s.origin, vec3_origin, damage * 0.6, 0, 0, MOD_SELFDESTRUCT);

	// GO BOOM!
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->client->ability_delay = level.time + 1;
	return;
}

void TeleportBehindTarget(edict_t* self, edict_t* target, float dist)
{
	int	i, cost;
	float min, ideal, z;
	vec3_t angles, forward, org, start, end;
	trace_t	tr;

	// start above the target and then later we'll push down
	z = target->absmax[2] + 1;
	VectorCopy(target->s.origin, org);
	org[2] = target->absmin[2] + fabs(self->mins[2]) + 1;

	// calculate angles 180 degrees behind target
	// note: Quake yaw angles are 0 -> 180/-180 -> 0 turning counter-clockwise, AngleCheck and ValidateAngles re-map to a 360 degree radius
	VectorCopy(target->s.angles, angles);
	angles[PITCH] = 0;
	angles[YAW] += 180;
	ValidateAngles(angles);
	ideal = angles[YAW];
	// calculate +/- 45 degrees behind (i.e. opposite target's fov)
	min = angles[YAW] - 45;
	AngleCheck(&min);
	//max = angles[YAW] + 45;

	//gi.dprintf("target yaw %.0f behind %.0f min %.0f\n", target->s.angles[YAW], ideal, min);
	cost = BLINKSTRIKE_INITIAL_COST + BLINKSTRIKE_ADDON_COST * self->myskills.abilities[BLINKSTRIKE].current_level;
	if (BLINKSTRIKE_MIN_COST && cost < BLINKSTRIKE_MIN_COST)
		cost = BLINKSTRIKE_MIN_COST;


	for (i = 0; i < 10; i++)
	{
		if (i)
		{
			// random angle +/- 45 degrees behind target
			angles[YAW] = min + GetRandom(0, 90);
			AngleCheck(&angles[YAW]);
			//gi.dprintf("checking %.0f\n", angles[YAW]);
		}
		// convert angles back to vector
		AngleVectors(angles, forward, NULL, NULL);
		// calculate point behind target
		VectorMA(org, dist, forward, start);
		start[2] = z; // start above target
		// trace to that point behind target, stopping on obstructions (walls)
		tr = gi.trace(org, self->mins, self->maxs, start, target, MASK_SOLID);
		if (tr.startsolid || tr.allsolid)
		{
			//gi.dprintf("trace 1 failed: from target to behind and above %s %s %.1f\n", tr.startsolid?"true":"false", tr.allsolid?"true":"false", tr.fraction);
			continue;
		}
		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, end);
		end[2] = target->absmin[2] + 1; // push down - final z height will be slightly above or at the same level as target
		// trace down
		tr = gi.trace(start, self->mins, self->maxs, end, NULL, MASK_SOLID);
		if (tr.startsolid || tr.allsolid)
		{
			//gi.dprintf("trace 2 failed: pushing down from above\n");
			continue;
		}
		VectorCopy(tr.endpos, start);
		// final trace
		tr = gi.trace(start, self->mins, self->maxs, start, self, MASK_SHOT);
		if (tr.fraction < 1)
		{
			//gi.dprintf("trace 3 failed: final position obstructed\n");
			continue;
		}
		VectorCopy(self->s.origin, self->client->oldpos); // save previous position
		VectorCopy(start, self->s.origin);
		VectorCopy(start, self->s.old_origin);
		self->s.event = EV_PLAYER_TELEPORT;
		gi.linkentity(self);
		//safe_cprintf(self, PRINT_HIGH, "teleport!\n");

		// face the target
		VectorSubtract(target->s.origin, self->s.origin, forward);
		vectoangles(forward, forward);
		self->s.angles[YAW] = forward[YAW];

		// set view angles to target
		if (self->client)
		{
			for (i = 0; i < 3; i++)
				self->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(forward[i] - self->client->resp.cmd_angles[i]);
			VectorCopy(forward, self->client->ps.viewangles);
			VectorCopy(forward, self->client->v_angle);
		}
		self->client->blinkStrike_targ = target; // blinkStrike target
		self->client->tele_timeout = level.framenum + BLINKSTRIKE_FRAMES; // duration of attack before returning to oldpos
		self->client->ability_delay = level.time + BLINKSTRIKE_DELAY;
		self->client->pers.inventory[power_cube_index] -= cost;
		break;
	}
}

void Cmd_BlinkStrike_f(edict_t* self)
{
	int cost;
	edict_t* target=NULL;

	cost = BLINKSTRIKE_INITIAL_COST + BLINKSTRIKE_ADDON_COST * self->myskills.abilities[BLINKSTRIKE].current_level;
	if (BLINKSTRIKE_MIN_COST && cost < BLINKSTRIKE_MIN_COST)
		cost = BLINKSTRIKE_MIN_COST;

	if (!G_CanUseAbilities(self, self->myskills.abilities[BLINKSTRIKE].current_level, cost))
		return;
	if (self->myskills.abilities[BLINKSTRIKE].disable)
		return;

	while ((target = findclosestreticle(target, self, 1024)) != NULL)
	{
		if (!G_EntIsAlive(target))
			continue;
		if (OnSameTeam(self, target))
			continue;
		if (!visible(self, target))
			continue;
		if (!infront(self, target))
			continue;
		TeleportBehindTarget(self, target, 128);
		break;
	}
}