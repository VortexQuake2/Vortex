#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

void drone_ai_stand (edict_t *self, float dist);
void drone_ai_run (edict_t *self, float dist);


mframe_t decoy_frames_stand1 [] =
{
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,

    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,

    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,

    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL,
    drone_ai_stand, 0, NULL
};
mmove_t decoy_move_stand = {FRAME_stand01, FRAME_stand40, decoy_frames_stand1, NULL};

void decoy_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &decoy_move_stand;
}
mframe_t decoy_frames_run [] =
{
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL
};
mmove_t decoy_move_run = {FRAME_run1, FRAME_run6, decoy_frames_run, NULL};

void actor_run (edict_t *self);

mframe_t decoy_frames_runandtaunt [] =
{
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL
};
mmove_t decoy_move_runandtaunt = {FRAME_taunt01, FRAME_taunt17, decoy_frames_runandtaunt, actor_run};

mframe_t decoy_frames_runandflipoff [] =
{
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL,
    drone_ai_run, 30, NULL
};
mmove_t decoy_move_runandflipoff = {FRAME_flip01, FRAME_flip12, decoy_frames_runandflipoff, actor_run};

void actor_run (edict_t *self)
{
//	gi.dprintf("actor_run()\n");
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		decoy_stand(self);
	else
		self->monsterinfo.currentmove = &decoy_move_run;
}

char *player_msg[] =
{
	"Watch it",
	"#$@*& you",
	"Get off me",
	"OMG you're a n00b"
};

char *other_msg[] =
{
	"Gay",
	"WTF",
	"OMG",
	"T_T"
};

void drone_pain (edict_t *self, edict_t *other, float kick, int damage);
void actor_pain (edict_t *self, edict_t *other, float kick, int damage)
{
//	char	*s;

//	gi.dprintf("actor_pain()\n");

	//if (self->health < (self->max_health / 2))
	//	self->s.skinnum = 1;

	if ((level.time < self->pain_debounce_time) && (random() > 0.5))
		return;

	self->pain_debounce_time = level.time + GetRandom(3, 10);
//	gi.sound (self, CHAN_VOICE, actor.sound_pain, 1, ATTN_NORM, 0);
	/*
	if (other->client)
		s = HiPrint(va("%s's decoy: %s %s", self->activator->client->pers.netname, 
			player_msg[GetRandom(0, 3)], other->client->pers.netname));
	else
		s = HiPrint(va("%s's decoy: %s %s", self->activator->client->pers.netname, 
			other_msg[GetRandom(0, 3)], other->client->pers.netname));
	gi.bprintf(PRINT_HIGH, "%s\n", s);
	*/

	if (random() < 0.5)
		self->monsterinfo.currentmove = &decoy_move_runandflipoff;
	else
		self->monsterinfo.currentmove = &decoy_move_runandtaunt;
	drone_pain(self, other, kick, damage);
}

void decoy_sword(edict_t* self)
{
	int damage;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = drone_damagelevel(self);
	MonsterAim(self, 1, 0, false, MZ_ROCKET, forward, start);

	if (self->s.frame == FRAME_attack1)
		gi.sound(self, CHAN_WEAPON, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);

	monster_fire_sword(self, start, forward, damage, SABRE_INITIAL_KICK, MZ_ROCKET);
}

void decoy_rocket (edict_t *self)
{
	int		damage, speed;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = drone_damagelevel(self);
	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * self->activator->myskills.level;
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;

	MonsterAim(self, 1, speed, true, MZ_ROCKET, forward, start);
	monster_fire_rocket (self, start, forward, damage, speed, 0);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_ROCKET);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}

void decoy_rail(edict_t* self)
{
	int damage;
	vec3_t	forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = drone_damagelevel(self);

	MonsterAim(self, 1, 0, false, MZ2_ACTOR_MACHINEGUN_1, forward, start);

	monster_fire_railgun(self, start, forward, damage, 100, MZ2_ACTOR_MACHINEGUN_1);
}

void actor_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

mframe_t actor_frames_death1 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,  NULL,
};
mmove_t actor_move_death1 = {FRAME_death101, FRAME_death106, actor_frames_death1, actor_dead};

mframe_t actor_frames_death2 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
};
mmove_t actor_move_death2 = {FRAME_death201, FRAME_death206, actor_frames_death2, actor_dead};

void decoy_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	// check for gib
	if (self->health <= self->gib_health)
	{
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		//ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		//self->deadflag = DEAD_DEAD;
#ifdef OLD_NOLAG_STYLE
		M_Remove(self, false, false);
#else
		if (nolag->value)
			M_Remove(self, false, true);
		else
			M_Remove(self, false, false);
#endif
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;
	self->deadflag = DEAD_DEAD;

	//Talent: Exploding Decoy
    if(damage > 0 && self->activator && self->activator->inuse)// && getTalentLevel(self->activator, TALENT_EXPLODING_DECOY) != -1)
    {
        //int talentLevel = vrx_get_talent_level(self->activator, TALENT_EXPLODING_DECOY);
		int decoyDamage = 0.2 * self->max_health;
		int decoyRadius = 100 + 5 * self->monsterinfo.level;// +talentLevel * 25;

        T_RadiusDamage(self, self->activator, decoyDamage, self, decoyRadius, MOD_DECOY);

        self->takedamage = DAMAGE_NO;
        self->think = BecomeExplosion1;
        self->nextthink = level.time + FRAMETIME;
        return;
    }

	// regular death
	self->takedamage = DAMAGE_YES;
	self->s.modelindex2 = 0;

	n = randomMT() % 2;
	if (n == 0)		self->monsterinfo.currentmove = &actor_move_death1;
	else			self->monsterinfo.currentmove = &actor_move_death2;	
}

mframe_t actor_frames_attack3[] =
{
	drone_ai_run, 30,  decoy_rail,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,   NULL
};
mmove_t actor_move_attack3 = { FRAME_attack1, FRAME_attack8, actor_frames_attack3, actor_run };

mframe_t actor_frames_attack2[] =
{
	drone_ai_run, 30,  decoy_sword,
	drone_ai_run, 30,  decoy_sword,
	drone_ai_run, 30,   decoy_sword,
	drone_ai_run, 30,   decoy_sword,
	drone_ai_run, 30,  decoy_sword,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,   NULL
};
mmove_t actor_move_attack2 = { FRAME_attack1, FRAME_attack8, actor_frames_attack2, actor_run };

mframe_t actor_frames_attack [] =
{
	drone_ai_run, 30,  decoy_rocket,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,  NULL,
	drone_ai_run, 30,   NULL,
	drone_ai_run, 30,   NULL
};
mmove_t actor_move_attack = {FRAME_attack1, FRAME_attack8, actor_frames_attack, actor_run};

void actor_attack(edict_t *self)
{
	int	weap_index;
	float dist = entdist(self, self->enemy);

	// we're dying, get up close!
	if (self->health < 0.3 * self->max_health)
	{
		self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	}
	else if (self->monsterinfo.aiflags & AI_NO_CIRCLE_STRAFE)
	{
		self->monsterinfo.aiflags &= ~AI_NO_CIRCLE_STRAFE;
	}

	if (dist < SABRE_INITIAL_RANGE)
	{
		weap_index = WEAP_SWORD;
		self->s.skinnum = self->activator->s.skinnum | (weap_index << 8);
		// sword attack
		self->monsterinfo.currentmove = &actor_move_attack2;
	}
	else if (dist < 512)
	{
		weap_index = WEAP_ROCKETLAUNCHER;
		self->s.skinnum = self->activator->s.skinnum | (weap_index << 8);
		// rocket attack
		self->monsterinfo.currentmove = &actor_move_attack;
	}
	else
	{
		weap_index = WEAP_RAILGUN;
		self->s.skinnum = self->activator->s.skinnum | (weap_index << 8);
		// rail attack
		self->monsterinfo.currentmove = &actor_move_attack3;
	}

	self->monsterinfo.attack_finished = level.time + 0.9;
}

/*QUAKED misc_actor (1 .5 0) (-16 -16 -24) (16 16 32)
*/

char* V_GetClassSkin(edict_t* ent);
void decoy_assign_class_skin(edict_t* self)
{
	int skin_number = maxclients->value + self->s.number - 1;
	char* c_skin = va("decoy\\%s\0", V_GetClassSkin(self->activator));
	gi.configstring(CS_PLAYERSKINS + skin_number, c_skin);
	self->s.skinnum = skin_number;
	//gi.dprintf("set class skin %s\n", c_skin);
}

//FIXME: this won't work correctly for player morphs--need to default to some other model, either class model or grunt
void decoy_copy (edict_t *self)
{
	edict_t *target = self->activator;

	if (!self->activator || !self->activator->inuse || !self->activator->client)
		return;

	// decoys will use class skins (if defined) for morphed players
	if (PM_PlayerHasMonster(self->activator) || self->activator->s.modelindex != 255)
	{
		//gi.dprintf("player-morph\n");
		//target = self->activator->owner;
		//self->owner = target;

		self->s.modelindex = 255;
		decoy_assign_class_skin(self);
		self->s.modelindex2 = 255;
	}
	else
	{
		//gi.dprintf("not a player morph\n");
		//target = self->activator;
		//self->owner = self->activator;
		// copy everything from our owner
		//gi.setmodel(self, self->model);
		self->s.skinnum = target->s.skinnum;
		self->model = target->model;
		self->s.modelindex = target->s.modelindex;
		self->s.modelindex2 = target->s.modelindex2;
	}
	
	self->s.effects = target->s.effects;
	self->s.renderfx = target->s.renderfx;
}

void decoy_melee(edict_t* self)
{

}

char* V_GetClassSkin(edict_t* ent);
void init_drone_decoy (edict_t *self)
{
	decoy_copy(self);
	self->monsterinfo.leader = self->activator;//always follow our leader!
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->mtype = M_DECOY;
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	self->health =  250 * self->activator->myskills.abilities[DECOY].current_level;
	self->max_health = self->health;
	self->gib_health = -1.5 * BASE_GIB_HEALTH;
	self->mass = 200;

	self->pain = actor_pain;
	self->die = decoy_die;

	self->monsterinfo.stand = decoy_stand;
	self->monsterinfo.run = actor_run;
	self->monsterinfo.attack = actor_attack;
	self->monsterinfo.melee = decoy_melee;
	self->monsterinfo.control_cost = M_DEFAULT_CONTROL_COST;
	self->monsterinfo.cost = 25;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = 0;
	gi.linkentity (self);

	self->monsterinfo.currentmove = &decoy_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
}

qboolean MirroredEntitiesExist (edict_t *ent);

void decoy_togglesolid(edict_t* ent, qboolean make_solid)
{
	edict_t* e = NULL;

	e = DroneList_Iterate();

	// search for other drones
	while (e)
	{
		if (e && e->activator && e->activator == ent && e->mtype == M_DECOY)
		{
			if (make_solid)
				e->owner = NULL;
			else
				e->owner = e->activator;
		}

		e = DroneList_Next(e);
	}
}

void Cmd_Decoy_f (edict_t *ent)
{
	char* s;
	edict_t *ret;

	if (ent->deadflag == DEAD_DEAD)
		return;

	if (debuginfo->value)
		gi.dprintf("DEBUG: %s just called Cmd_Decoy_f()\n", ent->client->pers.netname);

	s = gi.args();
	if (!Q_strcasecmp(s, "remove"))
	{
		RemoveAllDrones(ent, true);
		return;
	}

	if (!Q_strcasecmp(s, "solid"))
	{
		decoy_togglesolid(ent, true);
		return;
	}

	if (!Q_strcasecmp(s, "notsolid"))
	{
		decoy_togglesolid(ent, false);
		return;
	}

	if (!V_CanUseAbilities(ent, DECOY, M_DEFAULT_COST, true))
		return;

	if (MirroredEntitiesExist(ent))
	{
		safe_cprintf(ent, PRINT_HIGH, "You already have decoys out!\n");
		return;
	}

	ret = vrx_create_new_drone(ent, 20, false, true, 0);
	if (ret)
	{
		ret->health_cache += (int)(0.50 * ret->max_health) + 1;
		ret->armor_cache += (int)(0.50 * ret->monsterinfo.max_armor) + 1;
		ret->monsterinfo.regen_delay1 = level.framenum + 10;
		ret->monsterinfo.level = ent->myskills.abilities[DECOY].current_level;
	}
}
