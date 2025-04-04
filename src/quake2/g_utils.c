// g_utils.c -- misc utility functions for game module

#include "g_local.h"


void G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t *G_Find (edict_t *from, int fieldofs, char *match)
{
	char	*s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius(const edict_t *from, vec3_t const org, float rad)
{
	vec3_t	eorg;
	int		j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		if (VectorLength(eorg) > rad)
			continue;
		return from;
	}

	return NULL;
}

/*
=================
findclosestradius

Returns the closest entity that have origins within a spherical area

findclosestradius (prev_edict, origin, radius)
=================
*/
edict_t *findclosestradius (edict_t *prev_ed, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;
	edict_t *from;
	edict_t *found = NULL;
	float	found_rad, prev_rad, vlen;
	qboolean prev_found = false;

	rad *= rad;

	if (prev_ed) {
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (prev_ed->s.origin[j] + (prev_ed->mins[j] + prev_ed->maxs[j])*0.5);
		prev_rad = VectorLengthSqr(eorg);
	} else
	{
		prev_rad = rad + 1;
	}
	found_rad = 0;

	for (from = g_edicts; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		vlen = VectorLengthSqr(eorg);
		if (vlen > rad) // found edict is outside scanning radius
			continue;
		if ((vlen < prev_rad) && (prev_ed))  // found edict is closer than the previously returned edict
			continue; // thus this edict must have been returned in an earlier call
		if ((vlen == prev_rad) && (!prev_found)) // several edicts may be at the same range
			continue; // from the center of scan, so if the current edict is "in front of" 
		if (from == prev_ed) // the previously returned one, it must have been returned
			prev_found = true; // in an earlier call
		if ((!found) || (vlen <= found_rad)) {
			found = from;
			found_rad = vlen;
		}
	}

	return found;
}


/*
=================
findclosestradius

Returns the closest entity that have origins within a spherical area

findclosestradius (prev_edict, origin, radius)
=================
*/
/*edict_t *findclosestradius_monmask (edict_t *prev_ed, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;
	edict_t *from;
	edict_t *found = NULL;
	float	found_rad, prev_rad, vlen;
	qboolean prev_found = false;

	if (prev_ed) {
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (prev_ed->s.origin[j] + (prev_ed->mins[j] + prev_ed->maxs[j])*0.5);
		prev_rad = VectorLength(eorg);
	} else
	{
		prev_rad = rad + 1;
	}
	found_rad = 0;

	for (from = g_edicts; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		if (!G_ValidTargetEnt (from, true))
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		vlen = VectorLength(eorg);
		if (vlen > rad) // found edict is outside scanning radius
			continue;
		if ((vlen < prev_rad) && (prev_ed))  // found edict is closer than the previously returned edict
			continue; // thus this edict must have been returned in an earlier call
		if ((vlen == prev_rad) && (!prev_found)) // several edicts may be at the same range
			continue; // from the center of scan, so if the current edict is "in front of" 
		if (from == prev_ed) // the previously returned one, it must have been returned
			prev_found = true; // in an earlier call
		if ((!found) || (vlen <= found_rad)) {
			found = from;
			found_rad = vlen;
		}
	}

	return found;
}*/

// same as above but without the solidity check
edict_t *findclosestradius1 (edict_t *prev_ed, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;
	edict_t *from;
	edict_t *found = NULL;
	float	found_rad, prev_rad, vlen;
	qboolean prev_found = false;

	if (prev_ed) {
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (prev_ed->s.origin[j] + (prev_ed->mins[j] + prev_ed->maxs[j])*0.5);
		prev_rad = VectorLength(eorg);
	} else
		prev_rad = rad + 1;
	found_rad = 0;

	for (from = g_edicts ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
		vlen = VectorLength(eorg);
		if (vlen > rad) // found edict is outside scanning radius
			continue;
		if ((vlen < prev_rad) && (prev_ed))  // found edict is closer than the previously returned edict
			continue; // thus this edict must have been returned in an earlier call
		if ((vlen == prev_rad) && (!prev_found)) // several edicts may be at the same range
			continue; // from the center of scan, so if the current edict is "in front of" 
		if (from == prev_ed) // the previously returned one, it must have been returned
			prev_found = true; // in an earlier call
		if ((!found) || (vlen <= found_rad)) {
			found = from;
			found_rad = vlen;
		}
	}

	return found;
}

edict_t *findreticle (edict_t *from, edict_t *ent, float range, int degrees, qboolean vis)
{
	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		if (vis && !visible(ent, from))
			continue;
		if (entdist(ent, from) > range)
			continue;
		if (!infov(ent, from, degrees))
			continue;
		return from;
	}

	return NULL;
}

/*
=================
findclosestreticle

Returns the entity that matches the classname and is closest to the reticle

findclosestreticle (ent, radius, classname)
=================
*/

edict_t *findclosestreticle (edict_t *prev_ed, edict_t *ent, float rad)
{
	float	fdot=0, dot, pdot=0;
	vec3_t	vec, forward;
	edict_t	*from, *found=NULL;

	if (prev_ed)
	{
		VectorSubtract (prev_ed->s.origin, ent->s.origin, vec);
		VectorNormalize (vec);
		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		pdot = DotProduct(vec, forward);
	}

	for (from = g_edicts; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		if (from == ent)
			continue;

		// only search for targets within specified radius
		VectorSubtract (from->s.origin, ent->s.origin, vec);
		if (VectorLength(vec) > rad)
			continue;

		VectorNormalize (vec);

		AngleVectors (ent->client->v_angle, forward, NULL, NULL);
		dot = DotProduct (vec, forward);

		// if there is a previous edict, we should only be looking at targets that
		// are worse than the previous dot product, since we're adding more criteria
		// we are working backwards now
		if (prev_ed && ((int)(dot*100000) >= (int)(pdot*100000))) // converted to int to prevent rounding errors
			continue;

		// is this edict "better" than the last one?
		if (!fdot || (dot > fdot)) 
		{ 
			// check to compare best dot product to the last one found
			fdot = dot; // update the best dot product found
			found = from; // update the closest ent to reticle
		}
	}
	
	if (found == prev_ed) // don't get caught in infinite loop!
		return NULL;
	else
		return found;
}

/*
=================
FindPlayerByName

Returns the client with matching name

FindPlayerByName (name)
=================
*/

edict_t *FindPlayerByName(char *name)
{
	int		i;
	edict_t *player, *found = NULL;

	for (i = 1; i <= maxclients->value; i++)
	{
		player = &g_edicts[i];
		if (!player || !player->inuse)
			continue;
		if (Q_strcasecmp(name, player->client->pers.netname) != 0)
			continue;
		if (!G_IsSpectator(player))
			found = player;
	}
	return found;
}

edict_t *FindPlayer(char *s)
{
	int		i, p_index = 0;
	edict_t *player;

	// names can't be shorter than 3 characters, so this is most likely a player index
	if (strlen(s) < 3)
	{
		// if the index is more than maxclients, it's invalid
		p_index = atoi(s) + 1;
		if (p_index >= maxclients->value)
			return NULL;
	}

	for (i = 1; i <= maxclients->value; i++)
	{
		player = &g_edicts[i];
		
		if (!player || !player->inuse || G_IsSpectator(player))
			continue;

		// search by index value
		if (p_index)
		{
			// match
			if (p_index == i)
				return player;

			continue;
		}
		
		// does the name match?
		if (!Q_strcasecmp(s, player->client->pers.netname))
			return player;
	}

	return NULL;
}

/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES	8

edict_t *G_PickTarget (char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.dprintf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[randomMT() % num_choices];
}

void Think_Delay (edict_t *ent)
{
	G_UseTargets (ent, ent->activator);
	G_FreeEdict (ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets (edict_t *ent, edict_t *activator)
{
	edict_t		*t;

//
// check for a delay
//
	if (ent->delay)
	{
	// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.dprintf ("Think_Delay with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}
	
	
//
// print the message
//
	if ((ent->message) && !(activator->svflags & SVF_MONSTER))
	{
		safe_centerprintf (activator, "%s", ent->message);
		if (ent->noise_index)
			gi.sound (activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);
		else
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}

//
// kill killtargets
//
	if (ent->killtarget)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->killtarget)))
		{
			G_FreeEdict (t);
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using killtargets\n");
				return;
			}
		}
	}

//
// fire targets
//
	if (ent->target)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") &&
				(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating")))
				continue;

			if (t == ent)
			{
				gi.dprintf ("WARNING: Entity used itself.\n");
			}
			else
			{
				if (t->use)
					t->use (t, ent, activator);
			}
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*tv (float x, float y, float z)
{
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char	*vtos (vec3_t v)
{
	static	int		index;
	static	char	str[8][32];
	char	*s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1)&7;

	Com_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}


vec3_t VEC_UP		= {0, -1, 0};
vec3_t MOVEDIR_UP	= {0, 0, 1};
vec3_t VEC_DOWN		= {0, -2, 0};
vec3_t MOVEDIR_DOWN	= {0, 0, -1};

void G_SetMovedir (vec3_t angles, vec3_t movedir)
{
	if (VectorCompare (angles, VEC_UP))
	{
		VectorCopy (MOVEDIR_UP, movedir);
	}
	else if (VectorCompare (angles, VEC_DOWN))
	{
		VectorCopy (MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors (angles, movedir, NULL, NULL);
	}

	VectorClear (angles);
}

float vectoyaw (vec3_t vec)
{
	float	yaw;
	
	if (vec[YAW] == 0 && vec[PITCH] == 0)
		yaw = 0;
	else
	{
		yaw = (float) (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);//K03
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}

void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (float) (atan2(value1[1], value1[0]) * 180 / M_PI);//K03
		if (yaw < 0)
			yaw += 360;

		forward = sqrtf (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (float) (atan2(value1[2], forward) * 180 / M_PI);//K03
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

//GHz: returns an angle between two vectors from 0 to 180
// note: vectors v1 and v2 should be normalized
float vectoanglediff1(vec3_t v1, vec3_t v2)
{
	float delta = acos(DotProduct(v2, v1)) * 180 / M_PI;
	if (delta > 180.0f)
		delta = 360.0f - delta;
	return delta;
}

//GHz: returns an angle between two vectors from 0 to 180 (ccw/left) or -0 to -180 (cw/right) degrees
// thus providing both the difference in angle between vectors but also the direction to rotate v1 to meet v2
// note: vectors v1 and v2 should be normalized
float vectoanglediff2(vec3_t v1, vec3_t v2)
{
	//float angle = vectoyaw(v2) - vectoyaw(v1);
	float angle = (atan2(v2[YAW], v2[PITCH]) - atan2(v1[YAW], v1[PITCH])) * 180 / M_PI;
	if (angle > 180.0f)
		angle -= 360.0f;
	else if (angle < -180.0f)
		angle += 360.0f;
	return angle;
}

char *G_CopyString (char *in)
{
	char	*out;
	
	out = vrx_malloc (strlen(in)+1, TAG_LEVEL);
	strcpy (out, in);
	return out;
}

void dummy_think (edict_t *self)
{
	// dummy think routine does nothing
	// it is only here to prevent game errors
}

void G_InitEdict (edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;

	// taken from qwazzywabbit's implementation at tastyspleen
	e->memchain = NULL;
	e->think = dummy_think;
	e->nextthink = 0;
}

// The free-edict list.  Meant to vastly speed up G_Spawn().
edict_t *g_freeEdictsH = NULL;
edict_t *g_freeEdictsT = NULL;

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn (void)
{
	int			i;
	edict_t		*e;

	// If the free-edict queue can help, let it.
	while (g_freeEdictsH != NULL)
	{
		// Remove the first item.
		e = g_freeEdictsH;
		g_freeEdictsH = g_freeEdictsH->memchain;
		if (g_freeEdictsH == NULL)
			g_freeEdictsT = NULL;

		// If it's in use, get another one.
		if (e->inuse)
			continue;

		// If it's safe to use it, do so.
		if (e->freetime < 2 || level.time - e->freetime > 0.5)
		{
			G_InitEdict (e);
			return e;
		}

		// If we can't use it, we won't be able to use any of these -- anything
		// after it in the queue was freed even later.
		else
			break;
	}

	e = &g_edicts[(int)maxclients->value+1];
	for ( i=maxclients->value+1 ; i<globals.num_edicts ; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && ( e->freetime < 2 || level.time - e->freetime > 0.5 ) )
		{
			G_InitEdict (e);
			// gi.dprintf("allocating ent %d\n", i);
			return e;
		}
	}
	
	if (i == game.maxentities)
	{
//GHz START
		// list the entities so we have some clue as to what caused the crash
		PrintNumEntities(true);
//GHz END
		gi.error ("ED_Alloc: no free edicts");
	}
		
	globals.num_edicts++;

	G_InitEdict (e);
//GHz START
	// added this to prevent game errors
	e->think = dummy_think;
//GHz END
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/

void G_FreeEdict (edict_t *ed)
{
	gi.unlinkentity (ed); // unlink from world|

	if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
		return;
//GHz START
	dom_checkforflag(ed);
	//CTF_RecoverFlag(ed);
//GHz END
	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;

	// Put this edict into the free-edict queue. [QwazzyWabbit]
	if (g_freeEdictsH == NULL)
		g_freeEdictsH = ed;
	else
		g_freeEdictsT->memchain = ed;
	g_freeEdictsT = ed;
	ed->memchain = NULL;
	// end [QW]	
}

void G_FreeAnyEdict (edict_t *ed)
{
	gi.unlinkentity (ed);		// unlink from world

	dom_checkforflag(ed);

	ed->think = NULL;//GHz
	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;

		// Put this edict into the free-edict queue. [QwazzyWabbit]
	if (g_freeEdictsH == NULL)
		g_freeEdictsH = ed;
	else
		g_freeEdictsT->memchain = ed;
	g_freeEdictsT = ed;
	ed->memchain = NULL;
	// end [QW]	
}


/*
============
G_TouchTriggers

============
*/
void	G_TouchTriggers (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;

	// dead things don't activate triggers!
	if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))
		return;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		hit->touch (hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void	G_TouchSolids (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (ent->touch)
			ent->touch (hit, ent, NULL, NULL);
		if (!ent->inuse)
			break;
	}
}
/*
void testent_think (edict_t *self)
{
	trace_t tr;

	if (level.time > self->delay)
	{
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void SpawnDebugEnt (vec3_t start)
{
	edict_t *testent;

	testent = G_Spawn();
	testent->think = testent_think;
	testent->delay = level.time + 60;
	testent->classname = "testent";
	testent->s.modelindex = gi.modelindex("models/items/c_head/tris.md2");
	testent->s.effects |= EF_BLASTER;
	VectorCopy(start, testent->s.origin);
	testent->nextthink = level.time + FRAMETIME;
	gi.linkentity(testent);
}
*/



/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/

qboolean KillBox (edict_t *ent)
{
	trace_t		tr;

	while (1)
	{
		// 3.7 first attempt to teleport away anything nearby
		G_TeleportNearbyEntities(ent->s.origin, 64, false, ent);

		// first check
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_SHOT);
		if (tr.fraction == 1) // nothing here
			break;

		// try to kill it
		T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if it isn't gibbed by now, fail
		if (tr.ent->solid)
			return false;
	}

	return true; // all clear
}

qboolean KillBoxMonsters (edict_t *ent)
{
	trace_t		tr;

	while (1)
	{
		// 3.7 first attempt to teleport away anything nearby
		G_TeleportNearbyEntities(ent->s.origin, 64, false, ent);

		// first check
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_SHOT);
		if (tr.fraction == 1) // nothing here
			break;

		// try to kill it
		if (!ent->client && !ent->mtype) // not a player or morphed
			T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if it isn't gibbed by now, fail
		if (tr.ent->solid)
			return false;
	}

	return true; // all clear
}

/*
qboolean KillBox (edict_t *ent)
{
	trace_t		tr;

	while (1)
	{
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;

		// nail it
		T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

		// if we didn't kill it, fail
		if (tr.ent->solid)
			return false;
	}

	return true;		// all clear
}
*/

/*
=============
G_EntExists

Returns true if the entity is currently in-use by the game
engine, can take damage, and is currently solid
=============
*/
qboolean G_EntExists(const edict_t *ent) {
    return (ent && ent->inuse && ent->takedamage && ent->solid != SOLID_NOT);
}

qboolean G_ClientExists(edict_t *player) {
  return ((player) && (player->client) && (player->inuse));
}

/*
=============
G_EntIsAlive

Return true if the entity exists and is alive

For clients, we also check to see if they have
just respawned, and if so, the function returns
false
=============
*/
qboolean G_EntIsAlive(const edict_t *ent)
{
	// entity must exist and be in-use
	if (!ent || !ent->inuse)
		return false;
	// if this is not a player-monster (morph), then the entity must be solid/damageable
	if (!PM_PlayerHasMonster(ent) && (!ent->takedamage || (ent->solid == SOLID_NOT)))
		return false;
	// entity must be alive
	if ((ent->health < 1) || (ent->deadflag == DEAD_DEAD))
		return false;
	// if this is a client, they must be done spawning
	if (ent->client && (ent->client->respawn_time > level.time))
		return false;
	return true;
}


void G_RunFrames (edict_t *ent, int start_frame, int end_frame, qboolean reverse)
{
	if (reverse)
	{
		if ((ent->s.frame > start_frame) && (ent->s.frame <= end_frame))
			ent->s.frame--;
		else
			ent->s.frame = end_frame;
	}
	else
	{
		if ((ent->s.frame < end_frame) && (ent->s.frame >= start_frame))
			ent->s.frame++;
		else
			ent->s.frame = start_frame;
	}
}

float distance (vec3_t p1, vec3_t p2)
{
	vec3_t v;

	VectorSubtract(p1, p2, v);
	return VectorLength(v);
}

void AngleCheck (float *val)
{
	if (*val < 0)
		*val += 360;
	else if (*val > 360)
		*val -= 360;
}

void ValidateAngles (vec3_t angles)
{
	if (angles[0] < 0)
		angles[0] += 360;
	else if (angles[0] > 360)
		angles[0] -= 360;
	if (angles[1] < 0)
		angles[1] += 360;
	else if (angles[1] > 360)
		angles[1] -= 360;
	if (angles[2] < 0)
		angles[2] += 360;
	else if (angles[2] > 360)
		angles[2] -= 360;
}

int Get_KindWeapon (gitem_t	*it)
{
	if(it == NULL) return WEAP_BLASTER;

	if(it->weaponthink		== Weapon_Shotgun)		return WEAP_SHOTGUN;
	else if(it->weaponthink == Weapon_SuperShotgun) return WEAP_SUPERSHOTGUN;
	else if(it->weaponthink == Weapon_Machinegun)	return WEAP_MACHINEGUN;
	else if(it->weaponthink == Weapon_Chaingun)		return WEAP_CHAINGUN;
	else if(it->weaponthink == Weapon_Grenade)		return WEAP_GRENADES;
	else if(it->weaponthink == Weapon_GrenadeLauncher)	return WEAP_GRENADELAUNCHER;
	else if(it->weaponthink == Weapon_RocketLauncher)	return WEAP_ROCKETLAUNCHER;
	else if(it->weaponthink == Weapon_HyperBlaster) return WEAP_HYPERBLASTER;
	else if(it->weaponthink == Weapon_Railgun)		return WEAP_RAILGUN;
	else if(it->weaponthink == Weapon_BFG)			return WEAP_BFG;
	else return WEAP_BLASTER;
}

edict_t *G_GetClient(const edict_t *ent)
{
    if (ent == NULL)
        return NULL;

	if (!ent->inuse)
		return NULL;

	// do client check first to avoid returning non-client
	if (ent->client)
		return ent;
	else if (ent->activator && ent->activator->inuse && ent->activator->client)
		return ent->activator;
	else if (ent->creator && ent->creator->inuse && ent->creator->client)
		return ent->creator;
	else if (ent->owner && ent->owner->inuse && ent->owner->client)
		return ent->owner;
	else
		return NULL;
}

edict_t *G_GetSummoner (const edict_t *ent)
{
	if (!ent->inuse)
		return NULL;

	// do client check first to avoid returning non-client
	if (ent->client)
		return ent;

    if (ent->creator && ent->creator->inuse)
        return ent->creator;
    else if (ent->activator && ent->activator->inuse)
		return ent->activator;
	else if (ent->owner && ent->owner->inuse)
		return ent->owner;
	else
		return NULL;
}

qboolean G_ValidTargetEnt(edict_t *self, edict_t *target, qboolean alive) 
{
    if (alive) {
        if (!G_EntIsAlive(target))
            return false;
    } else if (!G_EntExists(target))
        return false;

    //4.2 G_EntIsAlive() will return true for player-monsters, but we never want to target them
	if (!target->takedamage || (target->solid == SOLID_NOT) || (target->solid == SOLID_BSP))
		return false;
	// don't target players with invulnerability
	//if (target->client && (target->client->invincible_framenum > level.framenum))
	//	return false;
	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;
	// don't target players in chat-protect
	if (!ptr->value && (target->flags & FL_CHATPROTECT))
		return false;
	// non-bots don't target entities with FL_NOTARGET set
	if (target->flags & FL_NOTARGET && (!self || !self->ai.is_bot))
		return false;
	// don't target spawning world monsters
	if (target->activator && !target->activator->client && (target->svflags & SVF_MONSTER) 
		&& (target->deadflag != DEAD_DEAD) && (target->nextthink-level.time > 2*FRAMETIME))
		return false;
	// don't target cloaked players
	if (target->svflags & SVF_NOCLIENT && target->mtype != M_FORCEWALL)
		return false;
	if (target->flags & FL_GODMODE)
		return false;
	//if (que_typeexists(target->curses, CURSE_FROZEN))
	//	return false;
	return true;
}

/*
=============
G_ValidTarget
returns true if the target is valid, but does not check allied/team status

self		the entity searching for a target (used for visibility and team checks
target		the entity we are checking
vis			whether or not a visibility check should be run
alive		true if a valid target must be alive
=============
*/

qboolean G_ValidTarget(const edict_t *self, const edict_t *target, qboolean vis, qboolean alive)
{
	if (!target)
		return false;

	if (trading->value && !(target->flags & FL_NO_TRADING_PROTECT))
		return false;

	// check for targets that require medic healing
	if (self && self->mtype == M_MEDIC)
	{
		if (M_ValidMedicTarget(self, target))
			return true;
	}

	if (!G_ValidTargetEnt(self, target, alive))
		return false;

	if (self)
	{
		if (vis && !visible(self, target))
			return false;
		if (OnSameTeam(self, target))
			return false;
	}
	return true;
}

//4.1 Same as above, but returns true if self and target are allied with each other.
qboolean G_ValidAlliedTarget (edict_t *self, edict_t *target, qboolean vis)
{
	if (!G_EntIsAlive(target))
		return false;
	// don't target spawning players
	if (target->client && (target->client->respawn_time > level.time))
		return false;
	// don't target players in chat-protect
	if (!ptr->value && target->client && (target->flags & FL_CHATPROTECT))
		return false;
	// don't target spawning world monsters
	if (target->activator && !target->activator->client && (target->svflags & SVF_MONSTER) 
		&& (target->deadflag != DEAD_DEAD) && (target->nextthink-level.time > 2*FRAMETIME))
		return false;
	// don't target cloaked players
	if (target->client && target->svflags & SVF_NOCLIENT)
		return false;
	if (vis && !visible(self, target))
		return false;
	return (!(target->flags & FL_GODMODE) /* && !que_typeexists(target->curses, CURSE_FROZEN) */
		&& OnSameTeam(self, target));
}

qboolean ClientCanConnect (edict_t *ent, char *userinfo)
{
	int		i;
	char	*value;
	edict_t	*find;

	for_each_player(find, i)
	{
		// if ((check_dupeip->value > 0) && (find != ent) 
		// 	&& !Q_stricmp(ent->client->pers.current_ip, find->client->pers.current_ip))
		// {
		// 	Info_SetValueForKey(userinfo, "rejmsg", "Duplicate IPs not permitted.");
		// 	return false;
		// }
		if (check_dupename->value > 0 && find != ent 
			&& !Q_stricmp(ent->client->pers.netname, find->client->pers.netname))
		{
			Info_SetValueForKey(userinfo, "rejmsg", "Same names not permitted.");
			return false;
		}
	}

	// check for assholes trying to BSOD win9x users with invalid skins
	value = Info_ValueForKey (userinfo, "skin");
	if (strcmp(value, "/con/con") == 0)
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Malicious user skin detected.");
		return false;
	}

	// check for assholes trying to BSOD win9x users with invalid skins
	if (strstr(value, "/com1") || strstr(value, "/com2") ||	strstr(value, "/com3") || strstr(value, "/com4") ||
		strstr(value, "/com5") || strstr(value, "/com6") || strstr(value, "/com7") || strstr(value, "/com8") ||
	    strstr(value, "/lpt1") || strstr(value, "/lpt2") ||	strstr(value, "/lpt3") || strstr(value, "/lpt4") ||
		strstr(value, "/lpt5") || strstr(value, "/lpt6") || strstr(value, "/lpt7") || strstr(value, "/lpt8") ||
		strstr(value, "/con"))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Malicious user skin detected.");
		return false;
	}
	return true;
}

int crand (void)
{
	if (random() > 0.5)
		return 1;
	return -1;
}

/*
=============
G_IsValidLocation

Returns true if the spot (or box) is empty
=============
*/
qboolean G_IsValidLocation (edict_t *ignore, vec3_t point, vec3_t mins, vec3_t maxs) {
	return (gi.trace(point, mins, maxs, point, ignore, MASK_SHOT).fraction==1.0);
}

/*
=============
G_IsClearPath

Returns true if the path is clear from obstruction
=============
*/
qboolean G_IsClearPath (edict_t *ignore, int mask, vec3_t spot1, vec3_t spot2) {
	return (gi.trace(spot1, NULL, NULL, spot2, ignore, mask).fraction == 1.0);
}

// an improved version of G_IsClearPath that allows for 2 entities to be ignored
qboolean G_ClearPath (edict_t *ignore1, edict_t *ignore2, int mask, vec3_t spot1, vec3_t spot2)
{
	trace_t tr;

	// start the trace
	tr = gi.trace(spot1, NULL, NULL, spot2, ignore1, mask);

	// is there anything blocking this path?
	if (tr.fraction == 1.0)
		return true;

	// if bumped into something we should ignore, then move on
	if (tr.ent && (tr.ent == ignore2))
		// trace again from where we left off
		return (gi.trace(tr.endpos, NULL, NULL, spot2, ignore2, mask).fraction == 1.0);
	return false;
}

void G_EntViewPoint (const edict_t *ent, vec3_t point)
{
	vec3_t	start;

	VectorCopy(ent->s.origin, point);

	if (ent->viewheight > 0)
	{
		// add viewheight
		point[2] += ent->viewheight;
	}
	else
	{
		// the entity is attached to a wall or ceiling (e.g. minisentry, proxy)
		if (ent->movetype == MOVETYPE_NONE)
		{
			// az note: moved from outside to inside because this trace doesn't matter
			// for the _majority_ of cases, that are movetype != NONE
			// and it was being made for entities that _definitely_ have
			// a movetype other than none
			trace_t tr;

			// trace to the floor to determine if we are on the ground
			VectorCopy(ent->s.origin, start);
			start[2] = ent->absmin[2] - 1;
			tr = gi.trace(point, NULL, NULL, start, ent, MASK_SOLID);

			if (tr.fraction == 1) {
				// use the midpoint instead
				G_EntMidPoint(ent, point);
				return;
			}
		}
		
		// normal entity is touching the ground and/or is moveable
		
		
		// find a point just below the top of bounding box
		VectorCopy(ent->s.origin, start);
		start[2] = ent->absmax[2] - 8;

		// if this point is below or equal to our starting point, then abort
		if (start[2] <= ent->s.origin[2])
			return;
		

		VectorCopy(start, point);
	}
}

// returns true if there are no obstructions between shooter and target
// start is optional and is only used if shooter is NULL
qboolean G_ClearShot (const edict_t *shooter, vec3_t start, const edict_t *target)
{
	vec3_t org, end;
	trace_t tr;

	G_EntMidPoint(target, end);

	if (shooter)
	{
		G_EntViewPoint(shooter, org);
		tr = gi.trace(org, NULL, NULL, end, shooter, MASK_SHOT);
	}
	else
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);

	if (tr.ent && tr.ent == target)
		return true;
	return false;
}

void G_EntMidPoint (const edict_t *ent, vec3_t point)
{
	int	midheight;

	VectorCopy(ent->s.origin, point);
	// get half the height of the actual bbox
	//midheight = 0.5 * (ent->maxs[2] + fabsf(ent->mins[2]));
	midheight = 0.5 * (ent->absmax[2] - ent->absmin[2]);
	point[2] = ent->absmin[2]+midheight;
}

qboolean visible1 (edict_t *ent1, edict_t *ent2)
{
	vec3_t	from;
	edict_t *ignore;
	trace_t	tr;

	// dont go thru BSP or forcewall
	ignore = ent1;
	VectorCopy(ent1->s.origin, from);
	while (ignore)
	{
		tr = gi.trace (from, NULL, NULL, ent2->s.origin, ignore, MASK_SHOT);
		if (G_EntExists(tr.ent))
		{
			if (tr.ent == ent2)
				return true;
			if (tr.ent->mtype == M_FORCEWALL)
				return false;
			ignore = tr.ent;
		}
		else
		{
			ignore = NULL;
		}
		
		// sometimes there is more than one entity at the same position
		if (VectorCompare(from, tr.endpos))
			return false; // we didn't get anywhere, so fail

		VectorCopy(tr.endpos, from);
		// we've reached our target
		if (VectorCompare(from, ent2->s.origin))
			break;
	}
	return (tr.fraction == 1.0);
}

void stuffcmd(edict_t *ent, char *s) 	
{
	//gi.dprintf("running stuffcmd on %s\n", ent->classname);
	if(ent->svflags & SVF_MONSTER || ent->ai.is_bot) return;

   	gi.WriteByte (svc_stufftext);
	gi.WriteString (s);
    gi.unicast (ent, true);	
}

// return true if the point is to the right of us
qboolean toright(edict_t* self, vec3_t point)
{
	vec3_t	vec;
	float	dot, value;
	vec3_t	right;

	AngleVectors(self->s.angles, NULL, right, NULL);
	VectorSubtract(point, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(right, vec);
	//value = 1.0 - (float)degrees / 180.0;
	//gi.dprintf("dot: %f ", dot);
	if (dot >= 0)
	{
		//gi.dprintf("right\n");
		return true;
	}
	//gi.dprintf("left\n");
	return false;
}

qboolean nearfov (edict_t *ent, edict_t *other, int vertical_degrees, int horizontal_degrees)
{
	int		delta;
	vec3_t	forward, right, start, offset;
	vec3_t	old_angles, new_angles;

	if (!visible(ent, other))
		return false;

	if (ent->client)
	{
		// get view origin
		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 0, 8,  ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
		// copy current viewing angles
		VectorCopy(ent->client->v_angle, old_angles);
		ValidateAngles(old_angles);
	}
	else
	{
		VectorCopy(ent->s.origin, start);
		// fix for retarded chick muzzle location
		if ((ent->svflags & SVF_MONSTER) && (start[2] < ent->absmin[2]+32))
			start[2] += 32;
		// copy current viewing angles
		VectorCopy(ent->s.angles, old_angles);
		ValidateAngles(old_angles);
	}

	// get vector to target
	VectorSubtract(other->s.origin, start, forward);
	VectorNormalize(forward);
	// convert to angles
	vectoangles(forward, new_angles);
	ValidateAngles(new_angles);

	if (horizontal_degrees)
	{
		delta = fabsf(old_angles[YAW]-new_angles[YAW]);
		if (delta > 180)
			delta = 360-delta;
		//gi.dprintf("delta %d horizontal\n", delta);
		if (delta > horizontal_degrees)
			return false;
	}

	if (vertical_degrees)
	{
		//if (ent->client)
			delta = fabsf(old_angles[PITCH]-new_angles[PITCH]);
		//else
		//	delta = fabsf(ent->s.angles[PITCH]-forward[PITCH]);
		if (delta > 180)
			delta = 360-delta;
		//gi.dprintf("delta %d vertical\n", delta);
		if (delta > vertical_degrees)
			return false;
	}

	return true;
}

char *vrx_encrypt_string (char *text, qboolean decrypt)
{
	int i;

	if (!text)
		return NULL;

	for (i=0; i<(int)strlen(text) ; i++)
		if (decrypt)
		{
			if ((byte)text[i] > 127)
				text[i]=(byte)text[i]-128;
		}
		else if ((byte)text[i] < 128)
		{
			text[i]=(byte)text[i]+128;
		}

	return text;
}

void PrintNumEntities (qboolean list)
{
	int		i=0;
	edict_t *e;

	for (e=g_edicts; e<&g_edicts[globals.num_edicts]; e++) 
	{
		if(!e->inuse)
			continue;
		if (list)
		{
			gi.dprintf("Printing list of entities in-use:\n");
			if (e->classname && (strlen(e->classname) > 0))
				gi.dprintf("%d) %s\n", i, e->classname);
		}
		i++;
	}

	gi.dprintf("INFO: %d entities in use (%d%c capacity).\n",
		i, (int)(100*((float)i/MAX_EDICTS)), '%');
}

int numNearbyEntities (edict_t *ent, float dist, qboolean vis)
{
	int		i=0;
	edict_t *e;

	if (!ent)
		return 0;

	for (e=g_edicts; e<&g_edicts[globals.num_edicts]; e++) 
	{
		if(!e->inuse)
			continue;
		if (e == ent)
			continue;
		if (vis && !visible(ent, e))
			continue;
		if (dist && (entdist(ent, e) > dist))
			continue;
		i++;
	}

	return i;
}

// GHz: correctly converts a float to an int via proper rounding!
int floattoint (float input)
{
	//return ((int)(input*100+50)/100);
	return (input >= 0) ? ((long)(input + 0.5)) : ((long)(input - 0.5));
}

void G_PrintGreenText (char *text)
{
	char *msg = HiPrint(text);
	gi.bprintf(PRINT_HIGH, "%s\n", msg);
	vrx_free(msg);
}

qboolean G_IsSpectator(const edict_t *ent)
{
	if (!ent->client)
		return false;

	if (ent->client->pers.spectator || ent->client->resp.spectator)
		return true;
	if ((ent->movetype == MOVETYPE_NOCLIP) 
		&& !(ent->owner && ent->owner->inuse)//3.9 added player-monster exception
		&& !(ent->flags & FL_WORMHOLE))//4.2 added wormhole exception
		return true;

	return false;
}

void G_TeleportNearbyEntities(vec3_t point, float radius, qboolean vis, edict_t *ignore)
{
	edict_t *e;

	if (INVASION_OTHERSPAWNS_REMOVED)
		return; // don't do this in invasion mode

	for (e=g_edicts; e<&g_edicts[globals.num_edicts]; e++) 
	{
		if(!G_EntExists(e))
			continue;

		// don't teleport player spawns
		if (pvm->value && e->mtype == INVASION_PLAYERSPAWN)
            continue;
        if (ctf->value && e->mtype == CTF_PLAYERSPAWN)
            continue;
        if (tbi->value && e->mtype == TBI_PLAYERSPAWN)
            continue;

        if (e == ignore)
            continue;
        if (G_IsSpectator(e))
            continue;
        if (vrx_has_flag(e))
            continue; // dont teleport flag carriers
        if (vis && !G_IsClearPath(ignore, MASK_SOLID, point, e->s.origin))
            continue;
        if (radius && (distance(point, e->s.origin) > radius))
            continue;

        vrx_find_random_spawn_point(e, false);
    }
}

char *G_GetTruncatedIP (edict_t *player)
{
	int		len, pos;
	char	buf[100];
	char	*loc, *ip;

	ip = player->client->pers.current_ip;
	len = strlen(ip);

	// can't determine IP address
	if (len < 1)
		return "";

	loc = strrchr(ip, '.');
	pos = loc-ip;

	// create the new string
	strncpy(buf, ip, pos);
	buf[pos] = '\0';

	return va("%s.*", buf);
}

void G_ResetPlayerState (edict_t *ent)
{
	int		i;
	edict_t	*cl_ent;

	// reset a single player's state
	if (ent && ent->inuse && ent->client && !G_IsSpectator(ent))
	{
		vrx_reset_player_state(ent);
	}
	// reset all players' state
	else
	{
		for (i=0 ; i<game.maxclients ; i++) 
		{
			cl_ent = g_edicts+1+i;
			if (cl_ent && cl_ent->inuse && !G_IsSpectator(cl_ent))
				vrx_reset_player_state(cl_ent);
	
		}
	}
}

// returns number of entities with matching classname that are owned by ent
int G_GetNumSummonable (edict_t *ent, char *classname)
{
	int		numEnt=0;
	edict_t *e=NULL;

	if (!classname || strlen(classname) < 1)
		return 0;

	//gi.dprintf("g_getnumsummonable()\n");

	while((e = G_Find(e, FOFS(classname), classname)) != NULL)
	{
		if (!e->inuse || (G_GetClient(e) != ent))
			continue;
		//gi.dprintf("found %s, owner=%s\n", classname, G_GetClient(e)->client->pers.netname);
		numEnt++;
	}

	return numEnt;
}

// NOTE: for bounding boxes, you must get the values of mins and maxs (or double the output of one)
// to get the hypotenuse across the entire horizontal bounding box
int G_GetHypotenuse (vec3_t v)
{
	return floattoint(sqrt(((v[0]*v[0]) + (v[1]*v[1]))));
}

int G_GetHypotenuse1(float a, float b)
{
	return floattoint(sqrt(((a * a) + (b * b))));
}

float G_GetBoxHypotenuse(vec3_t size)
{
	return sqrt(((size[0]*size[0]) + (size[1]*size[1]) + (size[2]*size[2])));
}

// attempts to move box away from plane at start using the smallest move based on box size
// returns the plane's pitch angle
float G_PushAwayFromPlane(vec3_t start, trace_t tr, vec3_t mins, vec3_t maxs)
{
	float	pitch, dist = 0;
	vec3_t	angles, size;

	// calculate worst case distance
	VectorSet(size, fabs(mins[0]) + maxs[0], fabs(mins[1]) + maxs[1], fabs(mins[2]) + maxs[2]);
	// distance is half of the longest line (i.e diagonal) that can fit within our mins/maxs box
	float max_dist = 0.5 * G_GetBoxHypotenuse(size) + 1;

	// calculate the pitch angle of the clipped solid
	vectoangles(tr.plane.normal, angles);
	//gi.dprintf("yaw: %.0f\n", angles[YAW]);
	pitch = fabs(angles[PITCH]);
	if (pitch == 270) // ceiling
		dist = maxs[2] + 1;
	else if (pitch == 0) // wall
	{
		// wall is at a 90 degree angle
		if (fmodf(angles[YAW], 90) == 0)
			dist = maxs[1] + 1;
		else
			dist = max_dist;
	}
	else if (pitch == 90) // floor
		dist = fabs(mins[2]) + 1;
	else // irregular pitch (non right angle)
	{
		dist = max_dist;
	}
	//gi.dprintf("pitch %.1f yaw %.0f dist %.1f\n", pitch, angles[YAW], dist);
	// try to move away from the obstruction just enough to clear it
	// note: if this doesn't work, we could try to move backward from aiming vector 'forward' but we won't know how far!
	VectorMA(start, dist, tr.plane.normal, start);
	//VectorMA(start, -dist, forward, start);
	return pitch;
}

// returns true if the box defined by mins and maxs is clear of obstruction at least range distance from ent
// note: range should be no less than the minimum distance between the two entities where the hitboxes don't overlap/intersect
// mode - use PROJECT_HITBOX_NEAR if you want the box defined by mins and maxs to be traced between start and ending positions
//        use PROJECT_HITBOX_FAR if you want the box to be traced only at the ending position
//		  use PROJECT_HITBOX_FLOOR if you want the box to be traced to the floor using MASK_SOLID
//        default is PROJECT_HITBOX_FAR if mode is not set (0)
qboolean G_GetSpawnLocation (edict_t *ent, float range, vec3_t mins, vec3_t maxs, vec3_t start, vec3_t normal, int mode, qboolean ignore_self_clip)
{
	int		mask1 = MASK_SHOT;
	int		mask2 = MASK_MONSTERSOLID;
	int		addheight = ent->viewheight;// -8;
	int		aimheight;
	int		minheight;
	vec3_t	forward, right, offset, end;
	//vec3_t	muzzleloc,aimspot;//for testing
	trace_t	tr;

	if (!mode)
		mode = PROJECT_HITBOX_FAR;
	else if (mode == PROJECT_HITBOX_FLOOR)
		mask1 = mask2 = MASK_SOLID;

	// get starting position and forward vector
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	if (mode == PROJECT_HITBOX_NEAR)
	{
		// fix for spawning entities taller than player
		minheight = abs((int)mins[2]) + 1;
		if (PM_PlayerHasMonster(ent))
			aimheight = fabs(ent->owner->mins[2]) + addheight;
		else
			aimheight = fabs(ent->mins[2]) + addheight;
		// if the aim starting position is lower than the origin of the entity we're trying to spawn, then raise aimheight to the minimum height
		if (aimheight < minheight)
		{
			//gi.dprintf("current aimheight: %d minimum: %d\n", aimheight, minheight);
			addheight += minheight - aimheight;
			//gi.dprintf("adjusting aimheight up %d\n", addheight);
		}
	}
	VectorSet(offset, 0, 7,  addheight); // note: player's viewheight is 22
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	//VectorCopy(start, muzzleloc);// for testing/debugging

	// get ending position
	VectorCopy(start, end);
	VectorMA(start, range, forward, end);

	if (mode == PROJECT_HITBOX_FAR || mode == PROJECT_HITBOX_FLOOR)
		tr = gi.trace(start, NULL, NULL, end, ent, mask1); // don't trace box
	else
		tr = gi.trace(start, mins, maxs, end, ent, mask1); // trace box
	if (tr.allsolid || tr.startsolid)
	{
		//gi.dprintf("trace1: allsolid %d startsolid %d fraction %.1f\n", tr.allsolid, tr.startsolid, tr.fraction);
		return false;
	}

	// copy the normal if the trace touched worldspawn (i.e. a wall or ceiling)
	VectorCopy(tr.endpos, start);
	if (normal && tr.fraction < 1 && tr.ent && tr.ent->inuse && tr.ent == world)
		VectorCopy(tr.plane.normal, normal);

	float pitch=0;
	if (tr.fraction < 1 && (mode == PROJECT_HITBOX_FAR || mode == PROJECT_HITBOX_FLOOR))
	{
		pitch = G_PushAwayFromPlane(start, tr, mins, maxs);
		/*
		float	pitch, dist = 0;
		vec3_t	angles, size;

		// calculate the pitch angle of the clipped solid
		vectoangles(tr.plane.normal, angles);
		gi.dprintf("yaw: %.0f\n", angles[YAW]);
		pitch = angles[PITCH] * -1;
		if (pitch == 270) // ceiling
			dist = maxs[2] + 1;
		else if (pitch == 0) // wall
			dist = maxs[1] + 1;
		else if (pitch == 90) // floor
			dist = fabs(mins[2]) + 1;
		else // irregular pitch (non right angle)
		{
			VectorSet(size, fabs(mins[0]) + maxs[0], fabs(mins[1]) + maxs[1], fabs(mins[2]) + maxs[2]);
			// distance is "worst case" i.e. (half of) the longest line that can fit within our mins/maxs box
			dist = 0.5 * G_GetBoxHypotenuse(size) + 1;
			
		}
		gi.dprintf("pitch %.1f dist %.1f\n", pitch, dist);
		// try to move away from the obstruction just enough to clear it
		// note: if this doesn't work, we could try to move backward from aiming vector 'forward' but we won't know how far!
		VectorMA(start, dist, tr.plane.normal, start);
		//VectorMA(start, -dist, forward, start);
		*/
	}

	//VectorCopy(start, aimspot);//for testing
	//G_DrawDebugTrail(muzzleloc, aimspot);

	if (mode == PROJECT_HITBOX_FLOOR)
	{
		// trace down to the floor
		//VectorCopy(tr.endpos, start);
		VectorCopy(start, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, ent, mask1);
		if (tr.allsolid || tr.startsolid)
		{
			//gi.dprintf("trace2: allsolid %d startsolid %d fraction %.1f\n", tr.allsolid, tr.startsolid, tr.fraction);
			return false;
		}
		VectorCopy(tr.endpos, start);
		// push away from the floor, but not if we already did so
		if (pitch != 90)
			G_PushAwayFromPlane(start, tr, mins, maxs);
	}

	// we need to make picked up/previously picked up entities solid to player so that the trace function works
	tr = gi.trace(start, mins, maxs, start, NULL, mask2);
	//G_DrawDebugTrail(aimspot, tr.endpos);
	//gi.dprintf("ignore %d tr.ent %s allsolid %d startsolid %d fraction %.1f\n", ignore_self_clip, tr.ent->classname, tr.allsolid, tr.startsolid, tr.fraction);
	//gi.dprintf("tr.endpos: %.0f %.0f %.0f\n", tr.endpos[0], tr.endpos[1], tr.endpos[2]);
	if (tr.fraction < 1)
	{
		// ignore clipping against self (set ignore_self_clip to true if it's OK if trace intersects calling entity)
		// note: this could possibly cause multiple entities to spawn on top of each other if they all occupy the same space
		// barrels take measures to prevent this from occuring by not allowing the player to spawn/drop a barrel while inside one previously spawned
		if (tr.ent && tr.ent == ent && ignore_self_clip)
			return true;
		//gi.dprintf("trace fail\n");
		return false;
	}
	return true;
}

void G_LaserThink (edict_t *self)
{
	// must have an owner
	if (!self->owner || !self->owner->inuse)
	{
		G_FreeEdict(self);
		return;
	}

	// update position
	VectorCopy(self->pos2, self->s.origin);
	VectorCopy(self->pos1, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}

void G_DrawLaser (edict_t *ent, vec3_t v1, vec3_t v2, int laser_color, int laser_size)
{
	edict_t *laser;

	laser = G_Spawn();
	laser->movetype	= MOVETYPE_NONE;
	laser->solid = SOLID_NOT;
	laser->s.renderfx = RF_BEAM|RF_TRANSLUCENT;
	laser->s.modelindex = 1;
	laser->classname = "laser";
	laser->s.frame = laser_size; // beam diameter
    laser->owner = ent;
	laser->s.skinnum = laser_color;//0xd0d1d2d3 - bright green

    laser->think = G_LaserThink;
	VectorCopy(v2, laser->s.origin);
	VectorCopy(v1, laser->s.old_origin);
	VectorCopy(v2, laser->pos2);
	VectorCopy(v1, laser->pos1);
	gi.linkentity(laser);
	laser->nextthink = level.time + FRAMETIME;
}

void G_DrawLaserBBox (edict_t *ent, int laser_color, int laser_size)
{
	vec3_t origin, p1, p2;

	VectorCopy(ent->s.origin, origin);

	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);

	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	G_DrawLaser(ent, p1, p2, laser_color, laser_size);
}

void G_DrawBoundingBox (edict_t *ent)
{
	vec3_t origin, p1, p2;

	VectorCopy(ent->s.origin, origin);
	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);

	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);

	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
}

void G_DrawDebugTrail(vec3_t start, vec3_t end)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(end, MULTICAST_ALL);
}

edict_t *G_FindEntityByMtype (int mtype, edict_t *from)
{
	if (from)
		from++;
	else
		from = g_edicts;

	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (from && from->inuse && (from->mtype == mtype))
			return from;
	}

	return NULL;
}

float Get2dDistance (vec3_t v1, vec3_t v2)
{
	return (sqrt((v2[0]-v1[0])*(v2[0]-v1[0])+(v2[1]-v1[1])*(v2[1]-v1[1])));
}

const char* Date()
{
	static char buf[100];
	time_t now;
	struct tm *ltime;

	now = time(NULL);
	ltime = localtime(&now);
	strftime(buf, 100, "%x", ltime);
	return buf;
}

const char* Time()
{
	static char buf[100];
	time_t now;
	struct tm *ltime;

	now = time(NULL);
	ltime = localtime(&now);
	strftime(buf, 100, "%X", ltime);
	return buf;
}

// note: this creates lots of tempents that can cause lagginess/overflows if you overdo it!
void G_DrawSparks(vec3_t start, vec3_t end, int primary_color, int secondary_color, int num, float dist_between_sparks, int min_rad, int max_rad)
{
	int		i, j, loops;
	float	dist;
	vec3_t	org, forward;

	// 0 = black, 8 = grey, 15 = white, 16 = light brown, 20 = brown, 57 = light orange, 66 = orange/red, 73 = maroon
	// 106 = pink, 113 = light blue, 119 = blue, 123 = dark blue, 200 = pale green, 205 = dark green, 209 = bright green
	// 217 = white, 220 = yellow, 226 = orange, 231 = red/orange, 240 = red, 243 = dark blue

	VectorSubtract(end, start, forward);
	dist = VectorLength(forward);
	VectorNormalize(forward);
	//VectorMA(start, -8, forward, start);
	//VectorCopy(start, org);
	// 
	// calculate number of loops needed to cover the distance between start and end
	loops = dist / dist_between_sparks;
	if (loops < 1)
		loops = 1;

	for (j = 0; j < loops; j++)
	{
		for (i = 0; i < num; i++)
		{
			VectorCopy(start, org);
			org[0] += crandom() * GetRandom(min_rad, max_rad);
			org[1] += crandom() * GetRandom(min_rad, max_rad);
			//org[2] += crandom() * GetRandom(0, (int) self->dmg_radius);

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_LASER_SPARKS);
			gi.WriteByte(1); // number of sparks
			gi.WritePosition(org);
			gi.WriteDir(vec3_origin);
			//gi.WriteByte(GetRandom(200, 209)); // color
			if (random() <= 0.33)
				gi.WriteByte(secondary_color);
			else
				gi.WriteByte(primary_color);
			gi.multicast(org, MULTICAST_PVS);
		}
		// move spawn point forward
		VectorMA(start, dist_between_sparks, forward, start);
	}
}


/*
=================
G_NearbyEnts
Returns number of entities within a spherical area
=================
*/
int G_NearbyEnts(vec3_t const org, float rad, qboolean is_visible)
{
	int num_ents = 0;
	for (edict_t *e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		if (!e->inuse)
			continue;
		//if (e->solid == SOLID_NOT)
		//	continue;
		if (e->svflags & SVF_NOCLIENT)
			continue;
		if (distance(org, e->s.origin) > rad)
			continue;
		if (is_visible && !G_IsClearPath(NULL, MASK_SOLID, org, e->s.origin))
			continue;
		num_ents++;
	}
	//gi.dprintf("%s: rad: %.0f ents: %d", __func__, rad, num_ents);
	return num_ents;
}

// creates an arrow at start pointing in the specified direction and returns a pointer to the temporary entity
edict_t* G_CreateArrowMarker(vec3_t start, vec3_t dir, int skinnum, float duration)
{
	vec3_t angles;
	edict_t* arrow;

	arrow = G_Spawn();
	arrow->movetype = MOVETYPE_NONE;
	arrow->solid = SOLID_NOT;
	//arrow->s.effects |= EF_ROTATE;
	arrow->s.modelindex = 1; // must be non-zero
	arrow->classname = "arrow marker";
	arrow->s.modelindex = gi.modelindex("models/misc_corner1/tris.md2");
	arrow->s.skinnum = skinnum;// 0: yellow, 3: white, 4: red
	arrow->think = G_FreeEdict;
	VectorCopy(start, arrow->s.origin);
	vectoangles(dir, angles);
	VectorCopy(angles, arrow->s.angles);
	gi.linkentity(arrow);
	arrow->nextthink = level.time + duration;
	return arrow;
}