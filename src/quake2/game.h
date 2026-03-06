#ifndef GAMEHEAD
#define GAMEHEAD
// game.h -- game dll information visible to server

#define	GAME_API_VERSION 2023
#define CGAME_API_VERSION 2022
#include <stdint.h>

#include "q_shared.h"

// edict->svflags
enum svflags_t : uint32_t
{
	SVF_NONE        = 0,          // no serverflags
	SVF_NOCLIENT    = 1 << 0,   // don't send entity to clients, even if it has effects
	SVF_DEADMONSTER = 1 << 1,   // treat as CONTENTS_DEADMONSTER for collision
	SVF_MONSTER     = 1 << 2,   // treat as CONTENTS_MONSTER for collision
#ifdef VRX_REPRO
	SVF_PLAYER      = 1 << 3,   // [Paril-KEX] treat as CONTENTS_PLAYER for collision
	SVF_BOT         = 1 << 4,   // entity is controlled by a bot AI.
	SVF_NOBOTS      = 1 << 5,   // don't allow bots to use/interact with entity
	SVF_RESPAWNING  = 1 << 6,   // entity will respawn on it's next think.
	SVF_PROJECTILE  = 1 << 7,   // treat as CONTENTS_PROJECTILE for collision
	SVF_INSTANCED   = 1 << 8,   // entity has different visibility per player
	SVF_DOOR        = 1 << 9,   // entity is a door of some kind
	SVF_NOCULL      = 1 << 10,  // always send, even if we normally wouldn't
	SVF_HULL        = 1 << 11   // always use hull when appropriate (triggers, etc; for gi.clip)
#endif
};



// edict->solid values

typedef enum
{
    SOLID_NOT,			// no interaction with other objects
    SOLID_TRIGGER,		// only touch when inside, after moving
    SOLID_BBOX,			// touch on edge
    SOLID_BSP			// bsp clip, touch on edge
} solid_t;

// bitflags for STAT_LAYOUTS
enum layout_flags_t : int16_t
{
	LAYOUTS_LAYOUT		      = 0, // svc_layout is active; escape remapped to putaway
	LAYOUTS_INVENTORY	      = 1, // inventory is active; escape remapped to putaway
	LAYOUTS_HIDE_HUD	      = 2, // hide entire hud, for cameras, etc
	LAYOUTS_INTERMISSION      = 3, // intermission is being drawn; collapse splitscreen into 1 view
	LAYOUTS_HELP              = 4, // help is active; escape remapped to putaway
	LAYOUTS_HIDE_CROSSHAIR	  = 5 // hide crosshair only
};


//===============================================================

// link_t is only used for entity area links now
typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

#define	MAX_ENT_CLUSTERS	16


typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

//===============================================================

//
// functions provided by the main engine
//
typedef struct
{
	// special messages
	void	(*bprintf) (int printlevel, const char *fmt, ...);
	void	(*dprintf) (const char *fmt, ...);
	void	(*cprintf) (const edict_t *ent, int printlevel, const char *fmt, ...);
	void	(*centerprintf) (const edict_t *ent,  const char *fmt, ...);
	void	(*sound) (const edict_t *ent, enum soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
	void	(*positioned_sound) (vec3_t origin, edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void	(*configstring) (int num, const char *string);

	void	(*error) (char *fmt, ...);

	// new names can only be added during spawning
	// existing names can be looked up at any time
	int		(*modelindex) (const char *name);
	int		(*soundindex) (const char *name);
	int		(*imageindex) (const char *name);

	void	(*setmodel) (edict_t *ent, const char *name);

	// collision detection
	trace_t	(*trace) (vec3_t start, const vec3_t mins, const vec3_t maxs, vec3_t end, const edict_t *passent, int contentmask);
	int		(*pointcontents) (vec3_t point);
	qboolean	(*inPVS) (vec3_t p1, vec3_t p2);
	qboolean	(*inPHS) (vec3_t p1, vec3_t p2);
	void		(*SetAreaPortalState) (int portalnum, qboolean open);
	qboolean	(*AreasConnected) (int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void	(*linkentity) (edict_t *ent);
	void	(*unlinkentity) (edict_t *ent);		// call before removing an interactive edict
	int		(*BoxEdicts) (vec3_t mins, vec3_t maxs, edict_t **list,	int maxcount, int areatype);
	void	(*Pmove) (pmove_t *pmove);		// player movement code common with client prediction

	// network messaging
	void	(*multicast) (vec3_t origin, multicast_t to);
	void	(*unicast) (edict_t *ent, qboolean reliable);
	void	(*WriteChar) (int c);
	void	(*WriteByte) (int c);
	void	(*WriteShort) (int c);
	void	(*WriteLong) (int c);
	void	(*WriteFloat) (float f);
	void	(*WriteString) (char *s);
	void	(*WritePosition) (vec3_t pos);	// some fractional bits
	void	(*WriteDir) (vec3_t pos);		// single byte encoded, very coarse
	void	(*WriteAngle) (float f);

	// managed memory allocation
	void	*(*TagMalloc) (size_t size, int tag);
	void	(*TagFree) (void *block);
	void	(*FreeTags) (int tag);

	// console variable interaction
	cvar_t	*(*cvar) (const char *var_name, const char *value, int flags);
	cvar_t	*(*cvar_set) (const char *var_name, const char *value);
	cvar_t	*(*cvar_forceset) (const char *var_name, const char *value);

	// ClientCommand and coneole command parameter checking
	int		(*argc) (void);
	char	*(*argv) (int n);
	char	*(*args) (void);

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void	(*AddCommandString) (const char *text);

	void	(*DebugGraph) (float value, int color);
} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	int			apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void		(*Init) (void);
	void		(*Shutdown) (void);

	// each new level entered will cause a call to SpawnEntities
	void		(*SpawnEntities) (char *mapname, char *entstring, char *spawnpoint);

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void		(*WriteGame) (char *filename, qboolean autosave);
	void		(*ReadGame) (char *filename);

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities, so any stored client spawn spots will
	// be used when the clients reconnect.
	void		(*WriteLevel) (char *filename);
	void		(*ReadLevel) (char *filename);

	qboolean	(*ClientConnect) (edict_t *ent, char *userinfo/*, qboolean loadgame*/);
	void		(*ClientBegin) (edict_t *ent, qboolean loadgame);
	void		(*ClientUserinfoChanged) (edict_t *ent, char *userinfo);
	void		(*ClientDisconnect) (edict_t *ent);
	void		(*ClientCommand) (edict_t *ent);
	void		(*ClientThink) (edict_t *ent, usercmd_t *cmd);

	void		(*RunFrame) (void);

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void		(*ServerCommand) (void);

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	// 
	// The size will be fixed when ge->Init() is called
	struct edict_s	*edicts;
	int			edict_size;
	int			num_edicts;		// current number, <= max_edicts
	int			max_edicts;
} game_export_t;

game_export_t *GetGameApi (game_import_t *import);
#endif