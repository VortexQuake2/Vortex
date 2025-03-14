#include "g_local.h"

//void chill_target_sound(edict_t* self);
void chill_target(edict_t* self, edict_t* target, int chill_level, float duration);

void nova_sparks(edict_t* self)
{
	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue
	vec3_t forward, start;
	VectorCopy(self->s.origin, start);
	for (int i = 0; i < 18; i++)
	{
		AngleVectors(self->s.angles, forward, NULL, NULL);
		VectorMA(self->s.origin, 64, forward, start);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_LASER_SPARKS);
		gi.WriteByte(4); // number of sparks
		gi.WritePosition(start);
		gi.WriteDir(vec3_origin);
		//gi.WriteByte(GetRandom(200, 209)); // color
		//if (random() <= 0.33)
		//	gi.WriteByte(217);
		//else
			gi.WriteByte(113);
		gi.multicast(start, MULTICAST_PVS);

		self->s.angles[YAW] += 20;
	}
}

void nova_think (edict_t *self)
{
	self->s.frame+=2;
	//nova_sparks(self);
	if (level.time > self->delay)
	{
		//nova_sparks(self);
		G_FreeEdict(self);
	}

	self->nextthink = level.time + FRAMETIME;
}

void NovaExplosionEffect (vec3_t org)
{
	edict_t *tempent;

	tempent = G_Spawn();
	tempent->s.modelindex = gi.modelindex ("models/objects/nova/tris.md2");
	tempent->think = nova_think;
	tempent->nextthink = level.time + FRAMETIME;
	tempent->s.effects |= /*EF_PLASMA |*/ EF_HALF_DAMAGE | EF_FLAG2 | EF_SPINNINGLIGHTS;
	tempent->delay = level.time + 0.7;
	VectorCopy(org, tempent->s.origin);
	gi.linkentity(tempent);
}

#define NOVA_RADIUS				150
#define NOVA_DEFAULT_DAMAGE		50
#define NOVA_ADDON_DAMAGE		30
#define NOVA_DELAY				0.3

void fire_nova(edict_t* inflictor, edict_t *attacker, int damage, float radius, int chillLevel, float chillDuration)
{
	edict_t* target = NULL;

	if (chillLevel > 0 && chillDuration > 0)
	{
		// chill nearby enemies
		while ((target = findradius(target, inflictor->s.origin, radius)) != NULL)
		{
			if (!G_ValidTarget(attacker, target, false, false))
				continue;
			if (!visible(inflictor, attacker))
				continue;

			//slow them
			chill_target(inflictor, target, chillLevel, chillDuration);
		}
		// damage them
		T_RadiusDamage(inflictor, attacker, damage, attacker, radius, MOD_ICEBOLT);//FIXME: MoD
	}
	else
	{
		T_RadiusDamage(inflictor, attacker, damage, attacker, radius, MOD_NOVA);
	}

	NovaExplosionEffect(inflictor->s.origin);
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

	fire_nova(ent, ent, damage, radius, 2 * skill_level, chill);

	// write a nice effect so everyone knows we've cast a spell
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("abilities/novaice.wav"), 1, ATTN_NORM, 0);

	//Talent: Wizardry - makes spell timer ability-specific instead of global
	int talentLevel = vrx_get_talent_level(ent, TALENT_WIZARDRY);
	if (talentLevel > 0)
	{
		ent->myskills.abilities[NOVA].delay = level.time + FROST_NOVA_DELAY;
		ent->client->ability_delay = level.time + FROST_NOVA_DELAY * (1 - 0.2 * talentLevel);
	}
	else
		ent->client->ability_delay = level.time + FROST_NOVA_DELAY/* * cost_mult*/;

	ent->client->pers.inventory[power_cube_index] -= cost;

	// calling entity made a sound, used to alert monsters
	ent->lastsound = level.framenum;
}

