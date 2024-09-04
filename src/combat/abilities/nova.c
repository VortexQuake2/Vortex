#include "g_local.h"

void nova_think (edict_t *self)
{
	self->s.frame+=2;
	if (level.time > self->delay)
		G_FreeEdict(self);
	self->nextthink = level.time + FRAMETIME;
}

void NovaExplosionEffect (vec3_t org)
{
	edict_t *tempent;

	tempent = G_Spawn();
	tempent->s.modelindex = gi.modelindex ("models/objects/nova/tris.md2");
	tempent->think = nova_think;
	tempent->nextthink = level.time + FRAMETIME;
	tempent->s.effects |= EF_PLASMA;
	tempent->delay = level.time + 0.7;
	VectorCopy(org, tempent->s.origin);
	gi.linkentity(tempent);
}

#define NOVA_RADIUS				150
#define NOVA_DEFAULT_DAMAGE		50
#define NOVA_ADDON_DAMAGE		30
#define NOVA_DELAY				0.3
#define FROSTNOVA_RADIUS		150
	
void Cmd_Nova_f (edict_t *ent, int frostLevel, float skill_mult, float cost_mult)
{
	int		damage, cost=NOVA_COST*cost_mult;

	if (!G_CanUseAbilities(ent, ent->myskills.abilities[NOVA].current_level, cost))
		return;
	if (ent->myskills.abilities[NOVA].disable)
		return;

	damage = (NOVA_DEFAULT_DAMAGE+NOVA_ADDON_DAMAGE*ent->myskills.abilities[NOVA].current_level)*skill_mult;

	T_RadiusDamage(ent, ent, damage, ent, NOVA_RADIUS, MOD_NOVA);

	//Talent: Frost Nova
	if(frostLevel > 0)
	{
		edict_t	*target = NULL;

		while ((target = findradius(target, ent->s.origin, FROSTNOVA_RADIUS)) != NULL)
		{
			if (target == ent)
				continue;
			if (!target->takedamage)
				continue;
			if (!visible1(ent, target))
				continue;
			if (OnSameTeam(ent, target)) //  we won't freeze friendly/owned entities
				continue;

            //curse with holyfreeze
			target->chill_level = 2 * frostLevel;
			target->chill_time = level.time + 3.0;	//3 second duration

			if (random() > 0.5)
                gi.sound(target, CHAN_ITEM, gi.soundindex("abilities/blue1.wav"), 1, ATTN_NORM, 0);
			else
                gi.sound(target, CHAN_ITEM, gi.soundindex("abilities/blue3.wav"), 1, ATTN_NORM, 0);
		}
	}		

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	NovaExplosionEffect(ent->s.origin);
    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/novaelec.wav"), 1, ATTN_NORM, 0);

	ent->client->ability_delay = level.time + NOVA_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

//Talent: Frost Nova
void Cmd_FrostNova_f (edict_t *ent, float skill_mult, float cost_mult)
{
    int slot = vrx_get_talent_slot(ent, TALENT_FROST_NOVA);
	talent_t *talent;
            
	if (slot == -1)
		return;

	talent = &ent->myskills.talents.talent[slot];

	if(talent->upgradeLevel > 0)
	{
		if(talent->delay < level.time)
		{
			Cmd_Nova_f(ent, talent->upgradeLevel, skill_mult, cost_mult);
			talent->delay = level.time + 2.2;	// 2 second recharge.
		}
		else safe_cprintf(ent, PRINT_HIGH, va("You can't cast another frost nova for %0.1f seconds.\n", talent->delay - level.time));
	}
	else 
	{
		safe_cprintf(ent, PRINT_HIGH, "You must upgrade frost nova before you can use it.\n");
	}	
}
