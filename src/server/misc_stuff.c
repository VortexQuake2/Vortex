#include "../quake2/g_local.h"


void KickPlayerBack(edict_t *ent)
{
	edict_t *other=NULL;

	while ((other = findradius(other, ent->s.origin, 175)) != NULL)
	{
		if (!other->takedamage)
			continue;
		if (!other->client)
			continue;
		if (!visible(ent, other))
			continue;
		if (other == ent)
			continue;
		ent->client->invincible_framenum = level.time + 2.0; // Add 2.0 seconds of invincibility
	}
}

void GetScorePosition () 
{ 
     int i, j, k; 
     int sorted[MAX_CLIENTS]; 
     int sortedscores[MAX_CLIENTS]; 
     int score, total, last_score, last_pos=1; 
     gclient_t *cl; 
     edict_t *cl_ent; 

     // sort the clients by score 
     total = 0; 
     for (i=0 ; i<game.maxclients ; i++)
	 {
          cl_ent = g_edicts + 1 + i; 
          if (!cl_ent->inuse) 
               continue; 
          score = game.clients[i].resp.score; 
          for (j=0 ; j<total ; j++)
		  {
               if (score > sortedscores[j]) 
               break; 
          } 
          for (k=total ; k>j ; k--) 
          { 
               sorted[k] = sorted[k-1]; 
               sortedscores[k] = sortedscores[k-1]; 
          } 
          sorted[j] = i; 
          sortedscores[j] = score; 
          total++; 
     }
	 last_score = sortedscores[0];

     for (i=0 ; i<total ; i++)
	 {
          cl = &game.clients[sorted[i]]; 
          cl_ent = g_edicts + 1 + sorted[i];

		  if (last_score != sortedscores[i])
			last_pos++;

		  //cl_ent->client->ps.stats[STAT_RANK] = last_pos;
		  //cl_ent->client->ps.stats[STAT_TOTALPLAYERS] = total;

     } 
} 

int GetRandom(int min,int max)
{	
	return (rand() % (max+1-min)+min);
}

qboolean findspawnpoint (edict_t *ent)
{
  vec3_t loc = {0,0,0};
  vec3_t floor;
  int i;
  int j = 0;
  int k = 0;
  trace_t tr;
  do {
    j++;
    for (i = 0; i < 3; i++)
      loc[i] = rand() % (8192 + 1) - 4096;
    if (gi.pointcontents(loc) == 0)
    {
      VectorCopy(loc, floor);
      floor[2] = -4096;
      tr = gi.trace (loc, vec3_origin, vec3_origin, floor, NULL, MASK_SOLID);
      k++;
      if (tr.contents & MASK_WATER)
        continue; 
	  if (tr.contents & CONTENTS_SOLID)
		continue;//Don't start the entity in a solid. It could be troublesome.
      VectorCopy (tr.endpos, loc);
      loc[2] += ent->maxs[2] - ent->mins[2]; // make sure the entity can fit!
    }
  } while (gi.pointcontents(loc) > 0 && j < 1000 && k < 500);
  if (j >= 1000 || k >= 500)
    return false;
  VectorCopy(loc,ent->s.origin);
  VectorCopy(loc,ent->s.old_origin);
 // gi.dprintf("found valid spot on try %d\n", j);
  return true;
}

csurface_t* FindSky()
{
	trace_t	tr;
	int mask,j,i;
	vec3_t start, end;

	mask = (MASK_MONSTERSOLID|MASK_PLAYERSOLID|MASK_SOLID);

	for (j=0;j<10000;j++)
	{
		// get a random position within a map
		for (i=0;i<3;i++)
			start[i] = rand() % (8192 + 1) - 4096;
		// is the point good?
		if (gi.pointcontents(start) != 0)
			continue;
		VectorCopy(start, end);

		// check above
		end[2] += 8192;
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (tr.surface->flags & (SURF_SKY)) //3.49 light flag indicates no-monster area
		{
			return tr.surface;
		}
	}
	return NULL;
}

qboolean FindValidSpawnPoint (edict_t *ent, qboolean air)
{
	int		i, j=0, mask;
	vec3_t	start, end, forward, right;
	trace_t	tr;

	//gi.dprintf("FindValidSpawnPoint()\n");

	mask = (MASK_MONSTERSOLID|MASK_PLAYERSOLID|MASK_SOLID);

	for (j=0;j<50000;j++)
	{
		// get a random position within a map
		for (i=0;i<3;i++)
			start[i] = rand() % (8192 + 1) - 4096;
		// is the point good?
		if (gi.pointcontents(start) != 0)
			continue;
		if (!air)
		{
			// then trace to the floor
			VectorCopy(start, end);
			end[2] -= 8192;
			tr = gi.trace(start, NULL, NULL, end, NULL, mask);
			// dont spawn on someone's head!
			if (tr.ent && tr.ent->inuse && (tr.ent->health > 0))
				continue;
			// add the ent's height
			VectorCopy(tr.endpos, start);
			start[2] += abs(ent->mins[2]) + 1;
			// is the point good?
			if (gi.pointcontents(start) != 0)
				continue;
		}

		// dont spawn outside of map!
		// check beneath us
		VectorCopy(start, end);
		end[2] -= 8192;
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (tr.surface->flags & (SURF_SKY|SURF_LIGHT)) //3.49 light flag indicates no-monster area
			continue;
		// check in front of us
		AngleVectors(ent->s.angles, forward, right, NULL);
		VectorMA(start, 8192, forward, end);
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (tr.surface->flags & SURF_SKY)
			continue;
		// check behind
		VectorMA(start, -8192, forward, end);
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (tr.surface->flags & SURF_SKY)
			continue;
		// check right
		VectorMA(start, 8192, right, end);
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (tr.surface->flags & SURF_SKY)
			continue;
		// check left
		VectorMA(start, -8192, right, end);
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
		if (!tr.surface || (tr.surface->flags & SURF_SKY))
			continue;

		// then check our final position
		tr = gi.trace(start, ent->mins, ent->maxs, start, NULL, mask);
		if (tr.startsolid || tr.allsolid || tr.fraction != 1.0)
			continue;
		if (tr.contents & mask)
			continue;

		// az: Hey cool, the position is good to go. Let's check that it's not behind any players.
		for (i=1; i <=maxclients->value; i++)
		{
			edict_t *cl = &g_edicts[i];
			if (cl->inuse && !cl->client->pers.spectator && !cl->client->resp.spectator)
			{
				vec3_t cl_forward;
				vec3_t r_vec;
				AngleVectors(cl->client->ps.viewangles, cl_forward, NULL, NULL);
				VectorSubtract(cl->s.origin, ent->s.origin, r_vec);
				if (DotProduct(r_vec, cl_forward) >= 0 && visible(ent, cl))
					continue;
			}
		}
		break;
	}

	VectorCopy(start, ent->s.origin);
	VectorCopy(start, ent->s.old_origin);
	gi.linkentity(ent);
	return true;
}

qboolean TeleportNearArea (edict_t *ent, vec3_t point, int area_size, qboolean air)
{
	int		i, j;
	vec3_t	start, end;
	trace_t	tr;

	for (i=0; i<50000; i++) {
		for (j=0; j<3; j++) {
			VectorCopy(point, start);
			start[j] += rand() % (area_size + 1) - 0.5*area_size;
			if (gi.pointcontents(start) != 0)
				continue;
			if (!air)
			{
				// then trace to the floor
				VectorCopy(start, end);
				end[2] -= 8192;
				tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SHOT);
				// dont spawn on someone's head!
				if (tr.ent && tr.ent->inuse && (tr.ent->health > 0))
					continue;
				// add the ent's height
				VectorCopy(tr.endpos, start);
				start[2] += abs(ent->mins[2]) + 1;
				// is the point good?
				if (gi.pointcontents(start) != 0)
					continue;
			}
			// can we "see" the point?
			tr = gi.trace(start, NULL, NULL, point, NULL, MASK_SOLID);
			if (tr.fraction < 1)
				continue;
			tr = gi.trace(start, ent->mins, ent->maxs, start, NULL, MASK_SHOT);
			if (tr.startsolid || tr.allsolid || tr.fraction != 1)
				continue;
			if (tr.contents & MASK_SHOT)
				continue;
			ent->s.event = EV_PLAYER_TELEPORT;
			VectorClear(ent->velocity);
			VectorCopy(start, ent->s.origin);
			VectorCopy(start, ent->s.old_origin);
			gi.linkentity(ent);
			//gi.dprintf("teleported near area\n");
			return true;
		}
	}

//	gi.dprintf("failed to teleport near area\n");
	return false;
}


void Check_full(edict_t *ent)
{
	gitem_t	*item;
	int		index;

	item = FindItem("Bullets");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_bullets)
			ent->client->pers.inventory[index] = ent->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_shells)
			ent->client->pers.inventory[index] = ent->client->pers.max_shells;
	}

	item = FindItem("Cells");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_cells)
			ent->client->pers.inventory[index] = ent->client->pers.max_cells;
	}

	item = FindItem("Grenades");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_grenades)
			ent->client->pers.inventory[index] = ent->client->pers.max_grenades;
	}

	item = FindItem("Rockets");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_rockets)
			ent->client->pers.inventory[index] = ent->client->pers.max_rockets;
	}

	item = FindItem("Slugs");
	if (item)
	{
		index = ITEM_INDEX(item);
		if (ent->client->pers.inventory[index] > ent->client->pers.max_slugs)
			ent->client->pers.inventory[index] = ent->client->pers.max_slugs;
	}
}

float entdist(edict_t *ent1, edict_t *ent2)
{
	vec3_t	vec;

	VectorSubtract(ent1->s.origin, ent2->s.origin, vec);
	return VectorLength(vec);
}



qboolean TeleportNearTarget (edict_t *self, edict_t *target, float dist)
{
	int		i;
	float	yaw;
	vec3_t	forward, start, end;
	trace_t	tr;

	// check 8 angles at 45 degree intervals
	for(i=0;i<8;i++)
	{
		yaw = anglemod(i*45);
		forward[0] = cos(DEG2RAD(yaw));
		forward[1] = sin(DEG2RAD(yaw));
		forward[2] = 0;
		// trace from target
		VectorMA(target->s.origin, (target->maxs[0]+self->maxs[0]+dist), forward, end);
		tr = gi.trace(target->s.origin, NULL, NULL, end, target, MASK_MONSTERSOLID);
		// trace to floor
		VectorCopy(tr.endpos, start);
		VectorCopy(tr.endpos, end);
		end[2] -= abs(self->mins[2]) + 32;
		tr = gi.trace(start, NULL, NULL, end, NULL, MASK_MONSTERSOLID);
		// we dont want to teleport off a ledge
		if (tr.fraction == 1.0 && !(self->flags & FL_FLY))
			continue;
		// check for valid position
		VectorCopy(tr.endpos, start);
		start[2] += abs(self->mins[2]) + 1;
		tr = gi.trace(start, self->mins, self->maxs, start, NULL, MASK_MONSTERSOLID);
		if (!(tr.contents & MASK_MONSTERSOLID))
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BOSSTPORT);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PVS);

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BOSSTPORT);
			gi.WritePosition (start);
			gi.multicast (start, MULTICAST_PVS);

			VectorCopy(start, self->s.origin);
			gi.linkentity(self);
			return true;
		}
	}
	return false;
}

qboolean TeleportNearPoint (edict_t *self, vec3_t point)
{
	int		i;
	float	yaw, dist=1;
	vec3_t	forward, start, end;
	trace_t	tr;

	// check 16 angles at 22.5 degree intervals
	while (dist < 256)
	{
		for(i=0;i<8;i++)
		{
			yaw = anglemod(i*22.5);
			forward[0] = cos(DEG2RAD(yaw));
			forward[1] = sin(DEG2RAD(yaw));
			forward[2] = 0;
			// trace from point
			VectorMA(point, (self->maxs[0]+abs(self->mins[0])+dist), forward, end);
			tr = gi.trace(point, NULL, NULL, end, NULL, MASK_SOLID);
			// trace to floor
			VectorCopy(tr.endpos, start);
			VectorCopy(tr.endpos, end);
			end[2] -= abs(self->mins[2]) + 128;
			tr = gi.trace(start, NULL, NULL, end, NULL, MASK_SOLID);
			// we dont want to teleport off a ledge
			if (tr.fraction == 1.0 && !(self->flags & FL_FLY))
				continue;
			// check for valid position
			VectorCopy(tr.endpos, start);
			start[2] += abs(self->mins[2]) + 1;
			tr = gi.trace(start, self->mins, self->maxs, start, NULL, (MASK_PLAYERSOLID|MASK_MONSTERSOLID));
			if (!(tr.contents & (MASK_PLAYERSOLID|MASK_MONSTERSOLID)))
			{
				self->s.event = EV_PLAYER_TELEPORT;
				VectorClear(self->velocity);
				VectorCopy(start, self->s.origin);
				gi.linkentity(self);
				return true;
			}
			dist += self->maxs[0]+abs(self->mins[0])+1;
		}
	}
	return false;
}

void WriteToLogFile (char *char_name, char *s)  
{  
     char     buf[512];  
     char     path[256];  
     FILE     *fptr;  

     if (strlen(char_name) < 1)  
          return;  
  
     //Create the log message  
     sprintf(buf, "%s %s [%s]: %s", CURRENT_DATE, CURRENT_TIME, "Offline", s);  
  
     //determine path  
     #if defined(_WIN32) || defined(WIN32)  
          sprintf(path, "%s\\%s.log", save_path->string, V_FormatFileName(char_name));  
     #else  
          sprintf(path, "%s/%s.log", save_path->string, V_FormatFileName(char_name));  
     #endif  
  
     if ((fptr = fopen(path, "a")) != NULL) // append text to log  
     {  
          //3.0 make sure there is a line feed  
          if (buf[strlen(buf)-1] != '\n')  
               strcat(buf, "\n");  
  
          fprintf(fptr, buf);  
          fclose(fptr);  
          return;  
     }  
     gi.dprintf("ERROR: Failed to write to player log.\n");  
}

void vrx_write_to_logfile(edict_t *ent, char *s)
{  
     char     *ip, buf[512];  
     char     path[256];  
     FILE     *fptr;  

     if (strlen(ent->client->pers.netname) < 1)  
          return;  
  
     //Create the log message  
     ip = Info_ValueForKey (ent->client->pers.userinfo, "ip");  
     sprintf(buf, "%s %s [%s]: %s", CURRENT_DATE, CURRENT_TIME, ip, s);  
  
     //determine path  
     #if defined(_WIN32) || defined(WIN32)  
          sprintf(path, "%s\\%s.log", save_path->string, V_FormatFileName(ent->client->pers.netname));  
     #else  
          sprintf(path, "%s/%s.log", save_path->string, V_FormatFileName(ent->client->pers.netname));  
     #endif  
  
     if ((fptr = fopen(path, "a")) != NULL) // append text to log  
     {  
          //3.0 make sure there is a line feed  
          if (buf[strlen(buf)-1] != '\n')  
               strcat(buf, "\n");  
  
          fprintf(fptr, buf);  
          fclose(fptr);  
          return;  
     }  
     gi.dprintf("ERROR: Failed to write to player log.\n");  
}

void WriteServerMsg (char *s, char *error_string, qboolean print_msg, qboolean save_to_logfile)  
{
	cvar_t	*port;
	char	buf[512];
	char	path[256];
	FILE	*fptr;  
 
     // create the log message 
     sprintf(buf, "%s %s %s: %s", CURRENT_DATE, CURRENT_TIME, error_string, s);
	 if (print_msg)
		 gi.dprintf("* %s *\n", buf);

	 if (!save_to_logfile)
		 return;
  
	 port = gi.cvar("port" , "0", CVAR_SERVERINFO);

     //determine path  
     #if defined(_WIN32) || defined(WIN32)  
          sprintf(path, "%s\\%d.log", game_path->string, (int)port->value);  
     #else  
          sprintf(path, "%s/%d.log", game_path->string, (int)port->value);  
     #endif  

     if ((fptr = fopen(path, "a")) != NULL) // append text to log  
     {  
          //3.0 make sure there is a line feed  
          if (buf[strlen(buf)-1] != '\n')  
               strcat(buf, "\n");  
  
          fprintf(fptr, buf);  
          fclose(fptr);  
          return;  
     }  
     gi.dprintf("ERROR: Failed to write to server log.\n");  
}

qboolean G_StuffPlayerCmds(edict_t *ent, const char *s)
{
	char *dst = ent->client->resp.stuffbuf;

	if (strlen(s)+strlen(dst) > 500)
	{
		//gi.dprintf("buffer full\n");
		return false; // don't overfill the buffer
	}
	
	strcat(dst, s);

	//gi.dprintf("%s", dst);
	return true;
}

void StuffPlayerCmds (edict_t *ent)
{
	int		i, num;
	int		end=0, start=0;

	// if the buffer is empty, then no cmds need to be stuffed
	if (strlen(ent->client->resp.stuffbuf) < 1)
		return;

	// initialize pointer
	if (!ent->client->resp.stuffptr || (ent->client->resp.stuffptr == &ent->client->resp.stuffbuf[0]))
		ent->client->resp.stuffptr = &ent->client->resp.stuffbuf[500];

	// get position
	num = 500 - (&ent->client->resp.stuffbuf[500]-ent->client->resp.stuffptr);

	// search backwards
	for (i=num; i>=0; i--)
	{
		// find the end
		if (!end && (ent->client->resp.stuffbuf[i] == '\n'))
		{
			//gi.dprintf("found end at %d\n", i);
			end = i;
			continue;
		}

		// find the start
		if (!start && (ent->client->resp.stuffbuf[i] == '\n'))
		{
			start = i+1;
			//gi.dprintf("found start at %d\n", start);
			break;
		}
	}

	stuffcmd(ent, &ent->client->resp.stuffbuf[start]);
	//gi.dprintf("stuffed to client: %s", &ent->client->resp.stuffbuf[start]);

	for (i=end; i>=start; i--)
	{
	//	gi.dprintf("zeroed %d\n", i);
		ent->client->resp.stuffbuf[i] = 0;
	}

	// update pointer
	ent->client->resp.stuffptr = &ent->client->resp.stuffbuf[start];	
}

// Paril
// Fixed this so it only cprintf's once.
void V_PrintSayPrefix (edict_t *speaker, edict_t *listener, char *text)
{
	int groupnum;
	char temp[2048];

	if (!dedicated->value)
		return;

	if (G_IsSpectator(speaker))
	{
		gi.cprintf(listener, PRINT_CHAT, "(Spectator) %s", text);
		return;
	}

	temp[0] = 0;
	// if they have a title, print it
	if (strcmp(speaker->myskills.title, ""))
		Com_sprintf (temp, sizeof(temp), "%s ", speaker->myskills.title);
		//safe_cprintf(listener, PRINT_HIGH, "%s ", speaker->myskills.title);

	if (ctf->value && speaker->teamnum)
	{
		groupnum = CTF_GetGroupNum(speaker, NULL);
		if (listener && (speaker->teamnum == listener->teamnum))
		{
			if (groupnum == GROUP_ATTACKERS)
				Com_sprintf (temp, sizeof(temp), "%s(Offense) ", temp);
				//safe_cprintf(listener, PRINT_CHAT, "(Offense) ");
			else if (groupnum == GROUP_DEFENDERS)
				Com_sprintf (temp, sizeof(temp), "%s(Defense) ", temp);
//				safe_cprintf(listener, PRINT_CHAT, "(Defense) ");
		}
		else
		{
			Com_sprintf (temp, sizeof(temp), "%s(%s) ", temp, CTF_GetTeamString(speaker->teamnum));
			//safe_cprintf(listener, PRINT_CHAT, "(%s) ", CTF_GetTeamString(speaker->teamnum));
		}
	}
	Com_sprintf (temp, sizeof(temp), "%s%s", temp, text);

	gi.cprintf (listener, PRINT_CHAT, "%s", temp);
}
