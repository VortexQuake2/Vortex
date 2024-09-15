#include "g_local.h"

qboolean IsValidChaseTarget (edict_t *ent)
{
	if (!ent->inuse)
		return false;

	// don't chase spectators
	if (ent->client)
	{
		if (G_IsSpectator(ent))
			return false;
	}
	else
	{
		//gi.dprintf("not a client\n");
		// the non-player entity must have the chaseable flag set
		if (!(ent->flags & FL_CHASEABLE))
			return false;
		// don't chase dead monsters
		if ((ent->health < 1) || (ent->deadflag == DEAD_DEAD))
			return false;
	}

	// don't chase any entity that is in noclip
	if ((ent->movetype == MOVETYPE_NOCLIP) 
		&& !(ent->flags & FL_WORMHOLE))//4.2 added wormhole exception
	{
		//gi.dprintf("noclip\n");
		return false;
	}

	return true;
}

void DisableChaseCam(edict_t *ent)
{
	ent->client->chase_target = NULL;
	ent->client->ps.gunindex = 0;
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	ent->client->ps.pmove.pm_flags &= ~PMF_DUCKED; // quit ducked.
}

void UpdateChaseCam (edict_t *ent)
{
	int			i;
	edict_t		*old, *targ;
	vec3_t		start, goal, pivot;
	vec3_t		angles, forward, right;
	trace_t		tr;
	qboolean	eyecam=false;
	
	if (!ent->client->chase_target)
		return;
	if (!G_IsSpectator(ent))
		return;

	//gi.dprintf("updating chase for %s\n", ent->client->pers.netname);

	// is our chase target no longer valid?
	if (!IsValidChaseTarget(ent->client->chase_target))
	{
		old = ent->client->chase_target;
		ChaseNext(ent); // try to find a new chase target
		if (ent->client->chase_target == old) 
		{
			// switch out of chase-cam mode
			DisableChaseCam(ent);
			return;
		}
	}

	
	targ = ent->client->chase_target;

	if (PM_MonsterHasPilot(targ))
		targ = targ->owner;

	VectorCopy(targ->s.origin, start);

	if (ent->client->chasecam_mode)
		eyecam = true;

retry_eyecam:
	if (eyecam) {
		// use client's viewing angle
		if (targ->client)
			VectorCopy(targ->client->v_angle, angles);
		// use non-client's angles
		else
			VectorCopy(targ->s.angles, angles);
	}
	else
	{
		// get the requested angles
		VectorCopy(ent->client->resp.cmd_angles, angles);
	}

	// if we're chasing a non-client entity that has a valid enemy
	// within our sights, then modify our viewing pitch
	if (eyecam && !targ->client && G_ValidTarget(targ, targ->enemy, true, true)
		&& infov(targ, targ->enemy, 90))
	{
		VectorSubtract(targ->enemy->s.origin, targ->s.origin, forward);
		vectoangles(forward, forward);
		angles[PITCH] = forward[PITCH];
		//gi.dprintf("pitch %d\n", (int)forward[PITCH]);
	}

	AngleVectors (angles, forward, right, NULL);
	VectorNormalize(forward);

	if (eyecam)
	{
        // save current player fov
        float fov = ent->client->ps.fov;

        if (targ->viewheight)
            start[2] += targ->viewheight;
        else
            start[2] = targ->absmax[2] - 8;
        VectorMA(start, targ->maxs[1] + 16, forward, start);

        // update HUD
        // az: don't show weapons with any of these classes
        if (targ->client && targ->myskills.class_num != CLASS_KNIGHT && !vrx_is_morphing_polt(targ))
            ent->client->ps = targ->client->ps;
        else
            ent->client->ps.gunindex = 0;
        // restore player's fov (don't use target's fov)
        ent->client->ps.fov = fov;
    }
	else
	{
		ent->client->ps.pmove.pm_flags &= ~PMF_DUCKED;
		ent->client->ps.gunindex = 0;

		// special conditions for upside-down minisentry
		if (targ->owner && (targ->mtype == M_MINISENTRY) 
			&& (targ->owner->style == SENTRY_FLIPPED))
		{
			start[2] = targ->absmin[2]-16;
		}
		else
		{
			if (targ->viewheight)
				start[2] += targ->viewheight;
			else
				start[2] = targ->absmax[2]-8;
			
			start[2] += 16; // az: put him a little bit above
			// from 16 to 24
			VectorCopy(start, pivot);
			VectorMA(start, (targ->mins[1]-64), forward, start);
		}
	}

	// jump animation lifts
	if (!targ->groundentity)
		start[2] += 16;
	tr = gi.trace(targ->s.origin, NULL, NULL, start, targ, MASK_SOLID);
	VectorCopy(tr.endpos, start);
	if (tr.fraction < 1)
	{
		if (eyecam)
			VectorMA(start, -12, forward, start);
		else
			VectorMA(start, 12, forward, start);
	}
	VectorCopy(start, goal);

	// pad for floors and ceilings
	VectorCopy(goal, start);
	start[2] += 6;
	tr = gi.trace(goal, vec3_origin, vec3_origin, start, targ, MASK_SOLID);
	if (tr.fraction < 1) {
		VectorCopy(tr.endpos, goal);
		goal[2] -= 6;
	}

	VectorCopy(goal, start);
	start[2] -= 6;
	tr = gi.trace(goal, vec3_origin, vec3_origin, start, targ, MASK_SOLID);
	if (tr.fraction < 1) {
		VectorCopy(tr.endpos, goal);
		goal[2] += 6;
	}

	// az: if the goal's too close to the entity, use the eye cam.
	if (!eyecam) {
		vec3_t dist;
		VectorSubtract(goal, pivot, dist);

		vec_t len = VectorLength(dist);
		if (len < 24) {
			eyecam = true;
			goto retry_eyecam;
		}
	}

	// az: PM_DEAD seems to jitter the chaser in q2pro
	if (eyecam)
		ent->client->ps.pmove.pm_type = PM_FREEZE;
	else
		ent->client->ps.pmove.pm_type = PM_SPECTATOR;

	VectorCopy(goal, ent->s.origin);
	
	if (targ->deadflag) 
	{
		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		if (targ->client)
			ent->client->ps.viewangles[YAW] = targ->client->killer_yaw;
		else
			ent->client->ps.viewangles[YAW] = 0;
	} 
	else 
	{
		if (!eyecam) {
			VectorSubtract(pivot, goal, angles);
			VectorNormalize(angles);
			vectoangles(angles, angles);
		}

		VectorCopy(angles, ent->client->ps.viewangles);
		VectorCopy(angles, ent->client->v_angle);
	}

	for (i=0 ; i<3 ; i++) {
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(angles[i] - ent->client->resp.cmd_angles[i]);
	}

	ent->viewheight = 0;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent) //GHz
{
	int i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target-g_edicts;
	do {
		i++;
		if (i > globals.num_edicts)
			i = 1;
		e = g_edicts + i;
		if (!IsValidChaseTarget(e))
			continue;
		break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void ChasePrev(edict_t *ent) //GHz
{
	int		i;
	edict_t *e;

	if (!ent->client->chase_target)
		return;
	
	i = ent->client->chase_target-g_edicts;
	do {
		i--;
		if (i < 1)
			i = globals.num_edicts;
		e = g_edicts + i;
		if (!IsValidChaseTarget(e))
			continue;
		break;
	} while (e != ent->client->chase_target);
	
	ent->client->chase_target = e;
	ent->client->update_chase = true;
}

void GetChaseTarget (edict_t *ent)//GHz
{
	int		i;
	edict_t *other;

	for (i=1; i<globals.num_edicts; i++) {
		other = &g_edicts[i];
		if (other == ent)
			continue;
		if (!IsValidChaseTarget(other))
			continue;
		ent->client->chase_target = other;
		ent->client->update_chase = true;
		UpdateChaseCam(ent);
		return;
	}
	safe_centerprintf(ent, "Nothing to chase.\n");
}
			

