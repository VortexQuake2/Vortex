/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

//	local to acebot files
//==========================================================

#include "ai_nodes_local.h"
#include "ai_weapons.h"
//#include "AStar.h" //jabot092

extern cvar_t				*bot_enable;//GHz: set to 1 to enable bots
extern cvar_t				*bot_dropnodes;//GHz: set to 1 to allow players to automatically drop nodes in the map used for bot navigation
extern cvar_t				*bot_autospawn;//GHz: set to number of bots that will automatically spawn at the beginning of each map
//bot debug_chase options
extern  cvar_t				*bot_showpath;
extern  cvar_t				*bot_showcombat;
extern  cvar_t				*bot_showsrgoal;
extern  cvar_t				*bot_showlrgoal;
extern	cvar_t				*bot_debugmonster;

//----------------------------------------------------------

#define MAX_BOTS 64
#define AI_GOAL_SR_RADIUS		200
#define MAX_BOT_SKILL			5		//skill levels graduation

// Platform states: 
#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

// Bot state types
#define BOT_STATE_STAND			0
#define BOT_STATE_MOVE			1
#define BOT_STATE_WANDER		2
#define BOT_STATE_ATTACK		3
#define BOT_STATE_DEFEND		4
#define BOT_STATE_MOVEATTACK	5//GHz


#define BOT_MOVE_LEFT			0
#define BOT_MOVE_RIGHT			1
#define BOT_MOVE_FORWARD		2
#define BOT_MOVE_BACK			3


//acebot_items.c players table
//----------------------------------------------------------
extern int	num_AIEnemies;
extern edict_t *AIEnemies[MAX_EDICTS];		// pointers to all players in the game


//Debug & creating and linking nodes
//----------------------------------------------------------
typedef struct
{
	qboolean	debugMode;

	qboolean	showPLinks;
	edict_t		*plinkguy;

	qboolean	debugChased;
	edict_t		*chaseguy;
} ai_devel_t;
extern ai_devel_t	AIDevel;



//----------------------------------------------------------

//game
//----------------------------------------------------------
void		CopyToBodyQue (edict_t *ent);
void		Use_Plat (edict_t *ent, edict_t *other, edict_t *activator);
void		ClientThink (edict_t *ent, usercmd_t *ucmd);
qboolean		SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles);
qboolean	ClientConnect (edict_t *ent, char *userinfo);

// bot_spawn.c
//----------------------------------------------------------
void		BOT_Respawn(edict_t *ent);


// ai_main.c
//----------------------------------------------------------
void		AI_Think (edict_t *ent);
void		AI_PickLongRangeGoal(edict_t *ent);
void		AI_Frame (edict_t *self, usercmd_t *ucmd);
void		AI_SetUpMoveWander( edict_t *ent );
void		AI_ResetNavigation(edict_t *ent);
void		AI_ResetWeights(edict_t *ent);


// ai_items.c
//----------------------------------------------------------
float		AI_ItemWeight(edict_t *ent, edict_t *item);
qboolean	AI_ItemIsReachable(edict_t *self,vec3_t goal);
qboolean	AI_IsItem(edict_t* it);//GHz
void AI_InitEnemiesList();//GHz

// ai_movement.c
//----------------------------------------------------------
void AI_ChangeAngle (edict_t *ent);
qboolean	AI_MoveToGoalEntity(edict_t *self, usercmd_t *ucmd);
qboolean	AI_SpecialMove(edict_t *self, usercmd_t *ucmd);
qboolean	AI_CanMove(edict_t *self, int direction);
qboolean	AI_IsLadder(vec3_t origin, vec3_t v_angle, vec3_t mins, vec3_t maxs, edict_t *passent);
qboolean	AI_IsStep (edict_t *ent);
float		AI_ForwardVelocity(edict_t* self);//GHz

// ai_navigation.c
//----------------------------------------------------------
int			AI_FindCost(int from, int to, int movetypes);
int			AI_FindClosestReachableNode( vec3_t origin, edict_t *passent, int range, int flagsmask );
int			AI_FindClosestHiddenNode(edict_t* ent, int range, int flagsmask);//GHz
int			AI_FindFarthestHiddenNode(edict_t* ent, int range, int flagsmask);//GHz
void		AI_SetGoal(edict_t *self, int goal_node, qboolean LongRange);
qboolean	AI_FollowPath(edict_t *self);


// ai_nodes.c
//----------------------------------------------------------
qboolean	AI_DropNodeOriginToFloor( vec3_t origin, edict_t *passent );
void		AI_InitNavigationData(void);
int			AI_FlagsForNode( vec3_t origin, edict_t *passent );
float		AI_Distance( vec3_t o1, vec3_t o2 );

void AITools_AddBotRoamNode(void);
char* AI_NodeString(int nodetype);//GHz


// ai_tools.c
//----------------------------------------------------------
void		AIDebug_SetChased(edict_t *ent);
void		AITools_DrawPath(edict_t *self, int node_from, int node_to);
//void		AITools_DrawLine(vec3_t origin, vec3_t dest);
void		AITools_InitEditnodes( void );
void		AITools_InitMakenodes( void );
void		AITools_SaveNodes( void );
qboolean	AI_LoadPLKFile( char *mapname );
void		AIDebug_ToogleBotDebug(void);//GHz
void		AI_RemoveMapNodes(void);//GHz

// ai_links.c
//----------------------------------------------------------
qboolean	AI_VisibleOrigins (vec3_t spot1, vec3_t spot2);
int			AI_LinkCloseNodes(void);
int			AI_FindLinkType(int n1, int n2);
qboolean	AI_AddLink( int n1, int n2, int linkType );
qboolean	AI_PlinkExists(int n1, int n2);
int			AI_PlinkMoveType(int n1, int n2);
int			AI_findNodeInRadius (int from, vec3_t org, float rad, qboolean ignoreHeight);
char		*AI_LinkString( int linktype );
int			AI_GravityBoxToLink(int n1, int n2);

//bot_status.c
//----------------------------------------------------------
qboolean	AI_CanPick_Ammo (edict_t *ent, gitem_t *item);
qboolean	AI_CanUseArmor (gitem_t *item, edict_t *other);
void		AI_BotRoamFinishTimeouts(edict_t *self);


//bot_classes
//----------------------------------------------------------
void		BOT_DMclass_InitPersistant(edict_t *self);
qboolean	BOT_ChangeWeapon (edict_t *ent, gitem_t *item);
void		M_default_Spawn (void);

//ai_weapons.c
//----------------------------------------------------------
void		AI_InitAIWeapons (void);
float		AI_GetWeaponRangeWeightByDistance(edict_t *self, int weapmodelIndex, float distance);//GHz
float		AI_GetWeaponProjectileVelocity(edict_t* ent, int weapmodelIndex);//GHz
qboolean	AI_IsLadder(vec3_t origin, vec3_t v_angle, vec3_t mins, vec3_t maxs, edict_t *passent);

//bot_abilities
//----------------------------------------------------------
int BOT_DMclass_ChooseAbility(edict_t* self);
void BOT_DMclass_FireAbility(edict_t* self, int ability_index);
void AI_InitAIAbilities(void);
void BOT_DMclass_UseBoost(edict_t* self);
void BOT_DMclass_UseBlinkStrike(edict_t* self);
void BOT_DMclass_UseSkeleton(edict_t* self);
void BOT_DMclass_UseGolem(edict_t* self);
qboolean BOT_DMclass_UseTball(edict_t* self, qboolean forget_enemy);

//ai_util.c
//----------------------------------------------------------
qboolean AI_IsProjectile(edict_t* ent);
qboolean AI_IsOwnedSummons(edict_t* self, edict_t* other);
qboolean AI_ValidMoveTarget(edict_t* self, edict_t* target, qboolean check_range);
qboolean AI_StraightPath(edict_t* self, float dist, float min_dp_value);
qboolean AI_ClearWalkingPath(edict_t* self, vec3_t start, vec3_t end);
float BOT_DMclass_ThrowingPitch1(edict_t* self, float v);
int AI_RespawnWeaponToWeapIndex(int respawn_weapon);
int AI_NumSummons(edict_t* self);