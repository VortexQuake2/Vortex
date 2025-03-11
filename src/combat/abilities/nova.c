#include "g_local.h"

//void chill_target_sound(edict_t* self);
void chill_target(edict_t* self, edict_t* target, int chill_level, float duration);

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

void fire_nova(edict_t* self, int damage, float radius, int chillLevel, float chillDuration)
{
	edict_t* target = NULL;

	if (chillLevel > 0 && chillDuration > 0)
	{
		// chill nearby enemies
		while ((target = findradius(target, self->s.origin, radius)) != NULL)
		{
			if (!G_ValidTarget(self, target, true, false))
				continue;

			//slow them
			chill_target(self, target, chillLevel, chillDuration);
		}
		// damage them
		T_RadiusDamage(self, self, damage, self, radius, MOD_ICEBOLT);//FIXME: MoD
	}
	else
	{
		T_RadiusDamage(self, self, damage, self, radius, MOD_NOVA);
	}

	NovaExplosionEffect(self->s.origin);
}

void Cmd_FrostNova_f (edict_t *ent, float skill_mult, float cost_mult)
{
	int	skill_level, damage, cost=FROST_NOVA_COST*cost_mult;
	float radius, chill;

	if (!V_CanUseAbilities(ent, NOVA, cost, true))
		return;

	skill_level = ent->myskills.abilities[NOVA].current_level;
	damage = (FROST_NOVA_INITIAL_DAMAGE + FROST_NOVA_ADDON_DAMAGE * skill_level) * (skill_mult * vrx_get_synergy_mult(ent, NOVA));
	radius = FROST_NOVA_INITIAL_RADIUS + FROST_NOVA_ADDON_RADIUS * skill_level;
	chill = (FROST_NOVA_INITIAL_CHILL + FROST_NOVA_ADDON_CHILL * skill_level) * skill_mult;

	fire_nova(ent, damage, radius, 2 * skill_level, chill);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/novaice.wav"), 1, ATTN_NORM, 0);

	ent->client->ability_delay = level.time + FROST_NOVA_DELAY/* * cost_mult*/;
	ent->client->pers.inventory[power_cube_index] -= cost;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

