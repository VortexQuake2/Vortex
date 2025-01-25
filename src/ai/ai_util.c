//GHz: this file should contain bot/ai utility functions

#include "g_local.h"
#include "ai_local.h"

qboolean AI_IsProjectile(edict_t* ent)
{
	return ent->clipmask == MASK_SHOT && (ent->solid == SOLID_BBOX  || ent->solid == SOLID_TRIGGER) && ent->s.modelindex && (ent->dmg || ent->radius_dmg);
}

// return true if other is a summons that we own
qboolean AI_IsOwnedSummons(edict_t* self, edict_t* other)
{
	return other->mtype && other->solid == SOLID_BBOX && other->takedamage && other->health > 0 && G_GetSummoner(other) == self;
}

// returns the number of entities that the bot has summoned
int AI_NumSummons(edict_t* self)
{
	int num_summons = 0;
	// hellspawn
	if (self->skull)
		num_summons++;
	// totems
	if (self->totem1)
		num_summons++;
	if (self->totem2)
		num_summons++;
	// non-exhaustive list of other summons
	num_summons += self->num_monsters;
	num_summons += self->num_skeletons;
	num_summons += self->num_autocannon;
	num_summons += self->num_detectors;
	num_summons += self->num_gasser;
	num_summons += self->num_lasers;
	num_summons += self->num_magmine;
	num_summons += self->num_obstacle;
	num_summons += self->num_sentries;
	num_summons += self->num_proxy;
	num_summons += self->num_spikers;
	return num_summons;
}

qboolean AI_ValidMoveTarget(edict_t* self, edict_t* target, qboolean check_range)
{
	// invalid/freed
	if (!target || !target->inuse)
		return false;
	// non-solid or invisible
	if (target->solid == SOLID_NOT || target->svflags & SVF_NOCLIENT)
		return false;
	float	dist = 0;
	if (check_range)
		dist = entdist(self, target);
	// projectiles
	if (AI_IsProjectile(target) && dist <= AI_RANGE_LONG)
		return true;
	// reachable summons
	if (AI_IsOwnedSummons(self, target) && AI_ClearWalkingPath(self, self->s.origin, target->s.origin))
		return true;
	// reachable items
	if (AI_IsItem(target) && AI_ItemIsReachable(self, target->s.origin) && dist <= AI_GOAL_SR_RADIUS)
		return true;
	return false;
}

// returns true if the length of the path specified ahead of the bot is (mostly) straight
// min_dp_value - minimum value of the resulting dot product between each node, with 1.0 being parallel, 0 is perpendicular, and -1 is in the opposite direction
qboolean AI_StraightPath(edict_t* self, float dist, float min_dp_value)
{
	int pos = 0, count = 0;
	float len, total, dot, delta;
	vec3_t v1, v2;

	// distance to next node
	len = total = distance(self->s.origin, nodes[self->ai.next_node].origin);

	// current leg of path is long enough, and the line between two points is always straight
	if (len >= dist)
		return true;

	// find the next node position in the path
	while (self->ai.path.nodes[pos] != self->ai.next_node)
	{
		pos++;
		if (self->ai.path.goalNode == self->ai.path.nodes[pos])
			return false;	// reached the end of the path
	}

	// vector from current node to next node
	VectorSubtract(nodes[self->ai.next_node].origin, nodes[self->ai.current_node].origin, v1);
	VectorNormalize(v1);

	//gi.dprintf("AI_StraightPath: %d (#%d) --> %d (#%d) [len:%.0f]", self->ai.current_node, pos-1, self->ai.next_node, pos, len);

	// check the remaining nodes in our path to see if they are (mostly) parallel/straight
	while (self->ai.path.nodes[pos] != self->ai.goal_node && count < 32)
	{
		VectorSubtract(nodes[self->ai.path.nodes[pos + 1]].origin, nodes[self->ai.path.nodes[pos]].origin, v2);
		len = VectorLength(v2);
		VectorNormalize(v2);
		dot = DotProduct(v2, v1);

		// compute angle difference between the two vectors
		//delta = acos(dot) * 180 / M_PI;
		//if (delta > 180.0f)
		//	delta = 360.0f - delta;

		//gi.dprintf("--> %d (#%d) [len:%.0f dot:%.2f delta:%.1f]",  self->ai.path.nodes[pos+1], pos+1, len, dot, delta);
		if (dot < 0.8)
		{
			//gi.dprintf("\n");
			return false; // next leg of path isn't parallel with the previous one
		}
		total += len;
		if (total >= dist)
		{
			//gi.dprintf("**STRAIGHT PATH COMPLETE!** total:%.0f/%.0f\n", total, dist);
			return true; // path is long enough
		}
		VectorCopy(v2, v1);
		pos++;
		count++;
	}
	//gi.dprintf("\n");
	return false; // reached the end of the path
}

/*
	Calculates the required pitch angle to hit a target with a projectile
	that has a vertical velocity boost.
	 Parameters:
	- v_xy (float): Horizontal velocity component.
	- v_z (float): Vertical velocity component.
	- d (float): Horizontal distance to the target.
	- h (float): Vertical height difference to the target.
	- g (float): Gravity
*/
/*
float BOT_DMclass_ThrowingPitch2(edict_t* self, float v_xy, float v_z)
{
	vec3_t offset, forward, right, start;
	VectorSet(offset, 8, 8, self->viewheight - 8);
	AngleVectors(self->client->v_angle, forward, right, NULL);
	P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

	float g = sv_gravity->value; // Gravity
	float d = Get2dDistance(start, self->enemy->s.origin);// Horizontal distance to the target.
	float h = self->enemy->s.origin[2] - start[2]; // Vertical difference between the projectile's initial height and the target height.
	// Time of flight assuming horizontal distance
	float t = d / v_xy;
	// Calculate the vertical displacement at time t
	float vertical_displacement = v_z * t - 0.5 * g * pow(t, 2);
	// Check if the target height is achievable
	if (h > vertical_displacement)
	{
		gi.dprintf("%s: TARGET UNREACHABLE\n", __func__);
		return -999;// The target is unreachable with the given parameters.
	}
	// Calculate the tangent of the required angle
	float tan_theta = (v_z - g * (t / 2)) / v_xy;
	// Convert the tangent to an angle in degrees
	float angle = atan(tan_theta) * (180 / M_PI);
	gi.dprintf("%s: pitch: %f ", __func__, angle);
	angle *= 0.33;
	if (h > 0) // Enemy is higher than bot
		angle *= -1;
	gi.dprintf("adjusted: %f\n", angle);
	return angle;
}
*/
#define FLT_EPSILON 0x0.000002p0
// equivalent to the function of the same name used in the Unity game engine
//FIXME: move to q_shared.c if this function is used elsewhere
void ProjectOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float sqrMag = DotProduct(normal, normal);
	if (sqrMag < FLT_EPSILON)
		return;
	float dot = DotProduct(normal, p);
	dst[0] = p[0] - normal[0] * dot / sqrMag;
	dst[1] = p[1] - normal[1] * dot / sqrMag;
	dst[2] = p[2] - normal[2] * dot / sqrMag;
}

// calculates the distance between startPos and endPos on both the horizontal (XY) and vertical (Z) plane
// note: the results are *usually* the same as Get2dDistance() and subtracting the Z coordinates of both vectors
//FIXME: move to q_shared.c if this function is used elsewhere
void CalculateDisplacement(vec3_t startAngles, vec3_t startPos, vec3_t endPos, float* horizontal, float* vertical)
{
	vec3_t v, xy_v, up;
	VectorSubtract(endPos, startPos, v);
	*vertical = v[2];
	AngleVectors(startAngles, NULL, NULL, up);
	ProjectOnPlane(xy_v, v, up);
	*horizontal = VectorLength(xy_v);
	//gi.dprintf("%s: horizontal: %.0f vertical: %.0f 2ddist: %.0f\n", __func__, *horizontal, *vertical, Get2dDistance(endPos, startPos));
}
/*
float BOT_DMclass_ThrowingPitch3(edict_t* self, float speed)
{
	vec3_t toTarget, down, forward;

	VectorSubtract(self->enemy->s.origin, self->s.origin, toTarget);
	float g = sv_gravity->value;
	float gSquared = pow(g, 2);

	// down is a vector pointing down equal to the length of sv_gravity
	AngleVectors(self->s.angles, NULL, NULL, down);
	VectorInverse(down);
	down[2] = g;

	float b = pow(speed, 2) + DotProduct(toTarget, down);
	float toTarget_Sqr = VectorLengthSqr(toTarget);
	float discriminant = pow(b, 2) - gSquared * toTarget_Sqr;

	if (discriminant < 0)
	{
		gi.dprintf("%s: target can't be reached\n", __func__);
		return -999;
	}

	float discRoot = sqrtf(discriminant);
	float t_max = sqrtf((b + discRoot) * 2.0 / gSquared);
	float t_min = sqrtf((b - discRoot) * 2.0 / gSquared);
	float t_lowEnergy = sqrtf(sqrtf(toTarget_Sqr * 4.0 / gSquared));

	// convert time-to-hit to ideal projectile launch velocity
	forward[0] = toTarget[0] / t_min - g * t_min / 2.0;
	forward[1] = toTarget[1] / t_min - g * t_min / 2.0;
	forward[2] = toTarget[2] / t_min - g * t_min / 2.0;

	// convert to angles
	//vectoangles(forward, forward);
	float f = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);
	float pitch = (float)(atan2(forward[2], f) * 180 / M_PI);
	//gi.dprintf("gSquared: %.0f b: %f toTarget_Sqr: %f discriminant: %.0f t_min: %f t_max: %f, pitch: %.1f\n",
	//	gSquared, b, toTarget_Sqr, discriminant, t_min, t_max, pitch);
	return pitch;
}
*/

// returns the ideal pitch angle to hit an enemy with a projectile with a fixed speed of v
// note: some of the internal variables may need to be tweaked for GL and lance because of the way Z velocity is added
//FIXME: the calculation tends to miss the mark when enemy is far away and when the short angle is >33 degrees
// it also misses when the vertical projectile speed is different from horizontal speed
//FIXME: angle2 (high angle) is currently unused because it usually hits the sky/ceiling--need to check for this somewhere!
float BOT_DMclass_ThrowingPitch1(edict_t* self, float v)
{
	vec3_t offset, forward, right, start, end;
	VectorSet(offset, 8, 8, self->viewheight - 8);
	AngleVectors(self->client->v_angle, forward, right, NULL);
	P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

	float g = sv_gravity->value;
	float d;// = Get2dDistance(start, self->enemy->s.origin);//Horizontal distance to the target.
	float h;// = self->enemy->s.origin[2] - start[2]; //Vertical difference between the projectile's initial height and the target height.

	G_EntMidPoint(self, start);
	G_EntMidPoint(self->enemy, end);
	CalculateDisplacement(self->s.angles, start, end, &d, &h);
	//gi.dprintf("displacement: h: %.0f v: %.0f\n", h_d, v_d);

	//float h = 1.0*(self->enemy->absmin[2] - self->absmax[2]);
	double discriminant = pow(v, 4) - g * (g * pow(d, 2) + (2 * h * pow(v, 2)));

	if (discriminant <= 0)
	{
		//gi.dprintf("%s: target can't be reached @ %f\n", __func__, d);
		return -999; // target can't be reached
	}

	float angle1 = atan((-pow(v, 2) + sqrt(discriminant)) / (-g * d)) * (180 / M_PI);
	float angle2 = atan((-pow(v, 2) - sqrt(discriminant)) / (-g * d)) * (180 / M_PI);

	angle1 *= -1;
	angle2 *= -1;
	if (angle1 < -33)
		angle1 -= 4; // small adjustment for far-away targets that tend to come up short
	//gi.dprintf("g=%.0f d=%.0f h=%.0f v=%.0f angle1=%.2f angle2=%.2f\n", g, d, h, v, angle1, angle2);
	//if (angle1 < -33) // use high angle for far-away targets
	//	return angle2;//FIXME: need to check vertical clearance before doing this, otherwise we hit the sky!
	//else
	//BOT_DMclass_ThrowingPitch3(self, v);//TESTING
	return angle1;
}

/*
float BOT_DMclass_ThrowingPitch(edict_t* self, float speed)
{
	if (self->enemy == NULL)
		return 0;

	float gravity = sv_gravity->value; // Gravity
	float distanceXYZ = distance(self->s.origin, self->enemy->s.origin); // Distance XY
	float distanceXY = distance(self->s.origin, self->enemy->s.origin); // Distance XYZ
	float height_diff = self->enemy->s.origin[2] - self->s.origin[2]; // Height difference
	float initialSpeed = speed;

	float timeToMaxHeight = sqrtf((initialSpeed * initialSpeed) / (2 * gravity));
	float totalFlightTime = timeToMaxHeight + sqrtf((distanceXYZ / height_diff) + timeToMaxHeight * timeToMaxHeight);

	// Calculate total flight time
	totalFlightTime = sqrtf((2 * distanceXYZ) / sv_gravity->value);

	float horizontalVelocity = distanceXY / totalFlightTime;
	float verticalVelocity = ((horizontalVelocity * horizontalVelocity) + (initialSpeed * initialSpeed)) / (2 * height_diff);

	float launchAngleDegrees = atan2f(verticalVelocity, horizontalVelocity) * (180 / M_PI);

	gi.dprintf("initial launchAngleDegrees: %f\n", launchAngleDegrees);

	launchAngleDegrees += 90;
	if (launchAngleDegrees)
	{
		launchAngleDegrees /= 2;

		if (height_diff > 0)
			launchAngleDegrees -= 90;
	}
	if (launchAngleDegrees > 89 || launchAngleDegrees < -89)
		launchAngleDegrees = 0;

	//Com_Printf("%s %s pitch[%f]\n", __func__, self->enemy->client->pers.netname, launchAngleDegrees);
	gi.dprintf("%s: pitch: %f (%d)\n", __func__, launchAngleDegrees, ANGLE2SHORT(launchAngleDegrees));

	BOT_DMclass_ThrowingPitch1(self, speed);
	BOT_DMclass_ThrowingPitch2(self, speed, speed + 300);

	return launchAngleDegrees;

	//ucmd->angles[PITCH] = launchAngleDegrees;
}
*/

// returns true if the path between start and end is walkable, with no obvious obstructions
qboolean AI_ClearWalkingPath(edict_t* self, vec3_t start, vec3_t end)
{
	float dist;// = distance(start, end);
	int num_steps;// = dist / 36;
	vec3_t path_start, path_end, tr_start, tr_end, tr_pos, dir;
	trace_t tr;

	// find our standing position off of the floor at start
	VectorCopy(start, tr_start);
	tr_start[2] -= 8192;
	tr = gi.trace(start, self->mins, self->maxs, tr_start, self, MASK_SOLID);
	VectorCopy(tr.endpos, tr_start);
	//tr_start[2] += self->absmin[2];

	// find our standing position off of the floor at end (this is as close as we can get!)
	VectorCopy(end, tr_end);
	tr_end[2] -= 8192;
	tr = gi.trace(end, self->mins, self->maxs, tr_end, self, MASK_SOLID);
	VectorCopy(tr.endpos, tr_end);
	//tr_end[2] += self->absmin[2];

	// get 2-D bearing, approximate distance and number of steps
	VectorCopy(tr_start, path_start);
	VectorCopy(tr_end, path_end);
	tr_start[2] = 0;
	tr_end[2] = 0;
	VectorSubtract(tr_end, tr_start, dir);
	dist = VectorLength(dir);
	VectorNormalize(dir);
	num_steps = dist / 36;

	if (num_steps <= 1)
		return true;

	//gi.dprintf("AI_ClearWalkingPath: dist:%f steps:%d\n", dist, num_steps);

	VectorCopy(path_start, tr_pos);
	tr_pos[2] += AI_STEPSIZE;
	path_end[2] += AI_STEPSIZE;

	while (distance(tr_pos, path_end) > 36)
	{
		// try to take a step forward
		VectorMA(tr_pos, 36, dir, tr_end);
		tr = gi.trace(tr_pos, self->mins, self->maxs, tr_end, self, MASK_SOLID);
		if (Get2dDistance(tr_pos, tr.endpos) < 1)
			return false; // can't move forward!
		// push down to floor
		VectorCopy(tr.endpos, tr_start);
		VectorCopy(tr.endpos, tr_end);
		tr_end[2] -= 8192;
		tr = gi.trace(tr_start, self->mins, self->maxs, tr_end, self, MASK_AISOLID);
		// check for hazards
		if (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
			return false; // obstructed by hazards
		// update position
		VectorCopy(tr.endpos, tr_pos);
		tr_pos[2] += AI_STEPSIZE;
		//gi.dprintf("remaining distance: %f\n", distance(tr_pos, path_end));
	}
	return true;
}

// converts the respawn_weapon index value (set in v_menu.c) to WEAP_* index
int AI_RespawnWeaponToWeapIndex(int respawn_weapon)
{
	if (respawn_weapon == 1)
		return WEAP_SWORD;
	else if (respawn_weapon == 11)
		return WEAP_GRENADES;
	else if (respawn_weapon == 13)
		return WEAP_BLASTER;
	else
		return respawn_weapon - 1;
}