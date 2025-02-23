#include "g_local.h"

void rock_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	G_FreeEdict(self);
}

void ThrowRockChunks(edict_t* self, char* modelname, float speed, vec3_t origin)
{
	edict_t* chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy(origin, chunk->s.origin);
	gi.setmodel(chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA(self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random() * 600;
	chunk->avelocity[1] = random() * 600;
	chunk->avelocity[2] = random() * 600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + GetRandom(1, 3);
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "rockchunks";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = rock_die;
	//chunk->s.effects |= EF_HALF_DAMAGE;
	gi.linkentity(chunk);
}

void rock_remove(edict_t* self)
{
	// remove rock entity next frame
	self->think = G_FreeEdict;
	self->nextthink = level.time + FRAMETIME;
}

void rock_explode(edict_t* self, cplane_t* plane)
{
	if (vrx_spawn_nonessential_ent(self->s.origin))
	{
		// throw chunks of rock
		int n = randomMT() % 5;
		while (n--)
			ThrowRockChunks(self, "models/objects/debris2/tris.md2", 2, self->s.origin);
	}
	// particle effects at impact site
	//SpawnDamage(TE_ELECTRIC_SPARKS, self->s.origin, plane->normal);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(8);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(plane->normal);
	gi.WriteByte(SPLASH_BROWN_WATER);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	rock_remove(self);
	self->exploded = true;
}

void golem_stone_hit(edict_t* self);

void rock_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (ent->exploded)
		return;

	// remove rock if owner dies or becomes invalid or if we touch a sky brush
	if (!G_EntIsAlive(ent->owner) || (surf && (surf->flags & SURF_SKY)))
	{
		rock_remove(ent);
		return;
	}

	if (other == ent->owner)
		return;

	if (G_EntExists(other))
	{
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 100, 0, MOD_MAGICBOLT);//FIXME: change MOD
	}

	//gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/coldimpact1.wav"), 1, ATTN_NORM, 0);
	golem_stone_hit(ent);

	rock_explode(ent, plane);

}

void fire_rocks(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float speed)
{
	edict_t* rock;
	vec3_t	dir;
	vec3_t	forward;

	//  entity made a sound, used to alert monsters
	self->lastsound = level.framenum;
	// get aiming angles
	vectoangles(aimdir, dir);
	// get directional vectors
	AngleVectors(dir, forward, NULL, NULL);

	// spawn grenade entity
	rock = G_Spawn();
	VectorCopy(start, rock->s.origin);
	//rock->s.effects |= EF_BLASTER;//EF_FLAG1;
	rock->movetype = MOVETYPE_TOSS;
	rock->clipmask = MASK_SHOT;
	rock->solid = SOLID_BBOX;
	VectorSet(rock->mins, -4, -4, 4);
	VectorSet(rock->maxs, 4, 4, 4);

	// randomize model
	if (random() > 0.5)
		rock->s.modelindex = gi.modelindex("models/proj/proj_golemrock1/tris.md2");//gi.modelindex ("models/objects/flball/tris.md2");
	else
		rock->s.modelindex = gi.modelindex("models/proj/proj_golemrock2/tris.md2");
	// randomize skin
	rock->s.skinnum = GetRandom(0, 9);

	rock->owner = self;
	rock->touch = rock_touch;
	rock->think = G_FreeEdict;
	rock->dmg = damage;
	rock->classname = "rock";
	gi.linkentity(rock);
	rock->nextthink = level.time + 10;
	VectorCopy(dir, rock->s.angles);

	// adjust velocity
	VectorScale(aimdir, speed, rock->velocity);

	// push up
	if (self->client && !self->ai.is_bot && !self->lockon)//GHz: don't boost vertical velocity for bots as it will affect ballistic calculations (i.e. finding the right pitch to hit the target)
		rock->velocity[2] += 150;
	// make it spin/roll
	VectorSet(rock->avelocity, 0, 0, 600);
	// play a sound
	//gi.sound(rock, CHAN_WEAPON, gi.soundindex("abilities/firecast.wav"), 1, ATTN_NORM, 0);
}

float BOT_DMclass_ThrowingPitch1(edict_t* self, float v);

void monster_fire_rocks(edict_t* self)
{
	int damage, radius, speed;
	float slvl, duration, pitch;
	vec3_t	forward, start, v = { 0,0,0 };

	if (!G_EntExists(self->enemy))
		return;

	slvl = drone_damagelevel(self);

	damage = 50 * slvl;
	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	//radius = ICEBOLT_INITIAL_RADIUS + ICEBOLT_ADDON_RADIUS * slvl;
	speed = ICEBOLT_INITIAL_SPEED + ICEBOLT_ADDON_SPEED * slvl;
	//duration = ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, 0, forward, start);

	// TESTING
	//vec3_t angles;
	//vectoangles(forward, angles);
	//if (angles[PITCH] < -180)
	//	angles[PITCH] += 360;
	//gi.dprintf("forward pitch: %.0f\n", angles[PITCH]);
	// TESTING

	//VectorSubtract(self->enemy->s.origin, self->s.origin, forward);
	//VectorNormalize(forward);
	pitch = BOT_DMclass_ThrowingPitch1(self, speed);
	if (pitch < -90)
		pitch = -45;
	//gi.dprintf("correct pitch: %.0f\n", pitch);
	v[PITCH] = pitch;
	AngleVectors(v, v, NULL, NULL);
	forward[2] = v[2];

	// TESTING
	//vectoangles(forward, angles);
	//if (angles[PITCH] < -180)
	//	angles[PITCH] += 360;
	//gi.dprintf("modified pitch: %.0f\n", angles[PITCH]);
	// TESTING


	//fire_icebolt(self, start, forward, damage, radius, speed, (int)(2 * slvl), duration);
	fire_rocks(self, start, forward, damage, speed);
	// Play sound effect
	//gi.sound(self, CHAN_WEAPON, gi.soundindex("spells/coldcast.wav"), 1, ATTN_NORM, 0);
}