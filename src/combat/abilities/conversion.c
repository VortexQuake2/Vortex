#include "g_local.h"

void drone_cleargoal(edict_t *self);

qboolean RestorePreviousOwner (edict_t *ent)
{
	if (!(ent->flags & FL_CONVERTED))
		return false;
	if (!ent->prev_owner || (ent->prev_owner && !ent->prev_owner->inuse))
		return false;
	return ConvertOwner(ent->prev_owner, ent, 0, false, false);
}

qboolean ConvertOwner (edict_t *ent, edict_t *other, float duration, qboolean pack_animal, qboolean print)
{
	int		current_num, max_num;
	edict_t *old_owner, **new_owner;
	qboolean was_packanimal = other->flags & FL_PACKANIMAL;

	// don't convert to a player if they are not a valid target
	//FIXME: this fails on players with godmode :(
	if (ent->client && !G_ValidTarget(NULL, ent, false, true))
	{
	//	gi.dprintf("%s is not a valid target\n", ent->client->pers.netname);
		return false;
	}
	
	if (pack_animal)
	{
		qboolean player_tank = PM_PlayerHasMonster(ent);
		// player isn't morphed
		if (!ent->mtype && !player_tank)
			return false;
	}

	// convert non-boss monsters
	if (!strcmp(other->classname, "drone") && other->monsterinfo.control_cost <= 2)
	{
		current_num = ent->num_monsters;
		if (pack_animal)
		{
			max_num = vrx_get_talent_level(ent, TALENT_PACK_ANIMAL); // Talent: Pack Animal
			current_num = ent->num_packanimals;
		}
		else if (ent->client)
			max_num = MAX_MONSTERS;
		else
			max_num = 999; // no maximum count for world monsters

		if (other->monsterinfo.bonus_flags)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "Can't convert champion and unique monsters\n");
			return false;
		}

		if (current_num + other->monsterinfo.control_cost > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "Insufficient monster slots for conversion\n");
			return false;
		}

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;

		// monsters converted in invasion mode should try hunting/following navi when they switch back
		if (!ent->client && invasion->value)
		{
			other->monsterinfo.aiflags &= ~AI_STAND_GROUND;
			other->monsterinfo.aiflags |= AI_FIND_NAVI;
		}

		// reduce active monster count in invasion if this is a converted world monster
		if (other->activator && other->activator->notify_drone_death) {
			other->activator->notify_drone_death(other);
		}

		if (was_packanimal)
		{
			// reduce pack animal counter
			old_owner->num_packanimals -= other->monsterinfo.control_cost;
			// remove packanimal flag so we know which counter to use
			other->flags &= ~FL_PACKANIMAL;
		}
		else
		{

			// decrement summonable counter for previous owner
			old_owner->num_monsters -= other->monsterinfo.control_cost;
			old_owner->num_monsters_real--;
			// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", other, old_owner->num_monsters_real);
		}
		
		// increment summonable counter for new owner
		if (pack_animal)
		{
			ent->num_packanimals += other->monsterinfo.control_cost;
			other->flags |= FL_PACKANIMAL;
			other->monsterinfo.leader = ent; // follow the player
		}
		else
		{
			ent->num_monsters += other->monsterinfo.control_cost;
			ent->num_monsters_real++;
		}
		// gi.bprintf(PRINT_HIGH, "adding %p (%d)\n", other, ent->num_monsters_real);

		// number of summonable slots in-use
		current_num = old_owner->num_monsters;
		
		// make this monster blink
		//FIXME: we should probably have another effect to indicate change of ownership
		other->monsterinfo.selected_time = level.time + 3.0;	
	}
	else if (pack_animal)
	{
		// pack animal can only convert monsters
		return false;
	}
	else if (!strcmp(other->classname, "hellspawn"))
	{
		if (ent->skull && ent->skull->inuse)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You already have a hellspawn\n");
			return false;
		}

		current_num = max_num = 1;

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;

		// previous owner no longer controls this entity
		old_owner->skull = NULL;

		// new owner should have a link to hellspawn
		ent->skull = other;
	}
	else if (!strcmp(other->classname, "Sentry_Gun"))
	{
		max_num = SENTRY_MAXIMUM;

		if (ent->num_sentries + 2 > max_num + 1)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the sentry limit\n");
			return false;
		}

		// set-up pointers
		old_owner = other->creator;
		new_owner = &other->creator;

		// previous owner no longer controls this entity
		old_owner->num_sentries -= 2;
		ent->num_sentries += 2;
		current_num = old_owner->num_sentries;

		// update stand owner too
		other->sentry->creator = ent;
	}
	else if (!strcmp(other->classname, "msentrygun"))
	{
		max_num = MAX_MINISENTRIES;

		if (ent->num_sentries + 1 > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the sentry limit\n");
			return false;
		}

		// set-up pointers
		old_owner = other->creator;
		new_owner = &other->creator;

		// previous owner no longer controls this entity
		old_owner->num_sentries--;
		ent->num_sentries++;
		current_num = old_owner->num_sentries;

		// update base owner too
		other->owner->creator = ent;
	}
	else if (!strcmp(other->classname, "spikeball"))
	{
		max_num = SPIKEBALL_MAX_COUNT;

		if (ent->num_spikeball + 1 > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spores\n");
			return false;
		}

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;
		other->owner = ent;

		// previous owner no longer controls this entity
		old_owner->num_spikeball--;
		ent->num_spikeball++;
		current_num = old_owner->num_spikeball;
	}
	else if (!strcmp(other->classname, "spiker"))
	{
		max_num = SPIKER_MAX_COUNT;

		if (ent->num_spikers + 1 > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of spikers\n");
			return false;
		}

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;

		// previous owner no longer controls this entity
		old_owner->num_spikers--;
		ent->num_spikers++;
		current_num = old_owner->num_spikers;
	}
	else if (!strcmp(other->classname, "gasser"))
	{
		max_num = GASSER_MAX_COUNT;

		if (ent->num_gasser + 1 > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of gassers\n");
			return false;
		}

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;

		// previous owner no longer controls this entity
		old_owner->num_gasser--;
		ent->num_gasser++;
		current_num = old_owner->num_gasser;
	}
	else if (!strcmp(other->classname, "obstacle"))
	{
		max_num = OBSTACLE_MAX_COUNT;

		if (ent->num_obstacle + 1 > max_num)
		{
			if (print && ent->client)
				safe_cprintf(ent, PRINT_HIGH, "You have reached the maximum amount of obstacles\n");
			return false;
		}

		// set-up pointers
		old_owner = other->activator;
		new_owner = &other->activator;

		// previous owner no longer controls this entity
		old_owner->num_obstacle--;
		ent->num_obstacle++;
		current_num = old_owner->num_obstacle;
	}
	else
		return false; // unsupported entity

	*new_owner = ent;

	// if the new owner is the same as the old one, then remove the converted flag
	if (other->prev_owner && (other->prev_owner == ent))
	{
		other->prev_owner = NULL;
		other->flags &= ~FL_CONVERTED;
		other->removetime = 0; // do not expire
	}
	else
	{
		other->prev_owner = old_owner;
		other->flags |= FL_CONVERTED;
	}

	if (print)
	{
		if (old_owner->client)
			safe_cprintf(old_owner, PRINT_HIGH, "Your %s was converted by %s (%d/%d)\n", 
				V_GetMonsterName(other), ent->client->pers.netname, current_num, max_num);

		if (ent->client)
			safe_cprintf(ent, PRINT_HIGH, "A level %d %s was converted to your side for %.0f seconds\n", other->monsterinfo.level, V_GetMonsterName(other), duration);
	}

	// update HUD layout
	if (ent->client)
		layout_add_tracked_entity(&ent->client->layout, other);
	if (old_owner->client)
		layout_remove_tracked_entity(&old_owner->client->layout, other);

	if (duration > 0)
		other->removetime = level.time + duration;

	// If they had a goal, clear it, now.
	drone_cleargoal(other);

	return true;
}

qboolean CanConvert (edict_t *ent, edict_t *other)
{
	if (!other)
		return false;

	if (!other->inuse || !other->takedamage || other->health<1 || other->deadflag==DEAD_DEAD)
		return false;

	if (OnSameTeam(ent, other))
	{
		// only allow conversion of allied summonables in PvP mode
		if (V_IsPVP() && IsAlly(ent, G_GetSummoner(other)))
			return true;
		else
			return false;
	}

	return true;
}

void Cmd_Conversion_f (edict_t *ent)
{
	int		duration = CONVERSION_INITIAL_DURATION + CONVERSION_ADDON_DURATION * ent->myskills.abilities[CONVERSION].current_level;
	float	chance = CONVERSION_INITIAL_CHANCE + CONVERSION_ADDON_CHANCE * ent->myskills.abilities[CONVERSION].current_level;
	float	range = CONVERSION_INITIAL_RANGE + CONVERSION_ADDON_RANGE * ent->myskills.abilities[CONVERSION].current_level;
	edict_t *e=NULL;

	if (!V_CanUseAbilities(ent, CONVERSION, CONVERSION_COST, true))
		return;

	if (chance > CONVERSION_MAX_CHANCE)
		chance = CONVERSION_MAX_CHANCE;

	while ((e = findclosestreticle(e, ent, range)) != NULL)
	{
		float r = random();

		if (!CanConvert(ent, e))
			continue;
		if (!infront(ent, e))
			continue;
		
	//	gi.dprintf("random = %f chance = %f\n", r, chance);

		if ((chance > r) && ConvertOwner(ent, e, duration, false, true))
            gi.sound(e, CHAN_ITEM, gi.soundindex("abilities/conversion.wav"), 1, ATTN_NORM, 0);
		else
			safe_cprintf(ent, PRINT_HIGH, "Conversion failed.\n");

		ent->client->ability_delay = level.time + CONVERSION_DELAY;
		ent->client->pers.inventory[power_cube_index] -= CONVERSION_COST;

		// write a nice effect so everyone knows we've cast a spell
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_TELEPORT_EFFECT);
		gi.WritePosition (ent->s.origin);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		//  entity made a sound, used to alert monsters
		ent->lastsound = level.framenum;
		return;
	}
}

// Talent: Pack Animal - automatically converts monsters of the same type to your side
// Note: On-hold for now
void think_talent_pack_animal(edict_t* ent) 
{
	int talent_level = vrx_get_talent_level(ent, TALENT_PACK_ANIMAL);
	// talent isn't upgraded
	if (talent_level < 1)
		return;
	// can't convert any more monsters
	if (ent->num_packanimals >= talent_level)
	{
		//gi.dprintf("too many pack animals\n");
		return;
	}
	// time between conversions gets shorter as the talent is upgraded (10s - 5s)
	int cooldown = (int)(6.0f / FRAMETIME) - 10 * talent_level;
	if (level.framenum % cooldown)
		return;
	//gi.dprintf("searching for monsters to convert...\n");
	edict_t* target = NULL;
	// find and convert monsters
	while ((target = findclosestradius(target, ent->s.origin, 1024)) != NULL)
	{
		if (!G_ValidTarget(ent, target, true, true))
			continue;
		// don't target players or their summons
		if (G_GetClient(target))
			continue;
		//if (target->mtype)
		//	gi.dprintf("found %s to convert!\n", V_GetMonsterName(target));
		//else
		//	gi.dprintf("found %s to convert!\n", target->classname);
		ConvertOwner(ent, target, 0, true, false);
		break;
	}
}