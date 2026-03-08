#ifndef VORTEXQUAKE2_Q_RECOMPAT_H
#define VORTEXQUAKE2_Q_RECOMPAT_H
#include <stdint.h>

#include "q_shared.h"

enum BoxEdictsResult_t
{
	Keep, // keep the given entity in the result and keep looping
	Skip, // skip the given entity

	End = 64, // stop searching any further

	Flags = End
};


enum goalReturnCode_t {
	Error = 0,
	Started,
	InProgress,
	Finished
};

typedef enum BoxEdictsResult_t (*BoxEdictsFilter_t)(edict_t *, void *);

typedef enum {
    PathReturnCode_ReachedGoal = 0,        // we're at our destination
    PathReturnCode_ReachedPathEnd,         // we're as close to the goal as we can get with a path
    PathReturnCode_TraversalPending,       // the upcoming path segment is a traversal
    PathReturnCode_RawPathFound,           // user wanted ( and got ) just a raw path ( no processing )
    PathReturnCode_InProgress,             // pathing in progress
    PathReturnCode_StartPathErrors,        // any code after this one indicates an error of some kind.
    PathReturnCode_InvalidStart,           // start position is invalid.
    PathReturnCode_InvalidGoal,            // goal position is invalid.
    PathReturnCode_NoNavAvailable,         // no nav file available for this map.
    PathReturnCode_NoStartNode,            // can't find a nav node near the start position
    PathReturnCode_NoGoalNode,             // can't find a nav node near the goal position
    PathReturnCode_NoPathFound,            // can't find a path from the start to the goal
    PathReturnCode_MissingWalkOrSwimFlag   // MUST have at least Walk or Water path flags set!
} PathReturnCode;

typedef enum {
    PathReturnCode_Walk,               // can walk between the path points
    PathReturnCode_WalkOffLedge,       // will walk off a ledge going between path points
    PathReturnCode_LongJump,           // will need to perform a long jump between path points
    PathReturnCode_BarrierJump,        // will need to jump over a low barrier between path points
    PathReturnCode_Elevator            // will need to use an elevator between path points
} PathLinkType;

enum {
    PathFlags_All             = (uint32_t)( -1 ),
    PathFlags_Water           = 1 << 0,  // swim to your goal ( useful for fish/gekk/etc. )
    PathFlags_Walk            = 1 << 1,  // walk to your goal
    PathFlags_WalkOffLedge    = 1 << 2,  // allow walking over ledges
    PathFlags_LongJump        = 1 << 3,  // allow jumping over gaps
    PathFlags_BarrierJump     = 1 << 4,  // allow jumping over low barriers
    PathFlags_Elevator        = 1 << 5   // allow using elevators
};
typedef uint32_t PathFlags;

typedef struct {
    vec3_t     start /*= { 0.0f, 0.0f, 0.0f }*/;
    vec3_t     goal /*= { 0.0f, 0.0f, 0.0f }*/;
    PathFlags   pathFlags /*= PathFlags::Walk*/;
    float       moveDist /*= 0.0f*/;

    struct DebugSettings {
        float   drawTime /*= 0.0f*/; // if > 0, how long ( in seconds ) to draw path in world
    } debugging;

    struct NodeSettings {
        bool    ignoreNodeFlags /*= false*/; // true = ignore node flags when considering nodes
        float   minHeight /*= 0.0f*/; // 0 <= use default values
        float   maxHeight /*= 0.0f*/; // 0 <= use default values
        float   radius /*= 0.0f*/;    // 0 <= use default values
    } nodeSearch;

    struct TraversalSettings {
        float dropHeight /*= 0.0f*/;    // 0 = don't drop down
        float jumpHeight /*= 0.0f*/;    // 0 = don't jump up
    } traversals;

    struct PathArray {
        vec3_t * posArray /*= nullptr*/;  // array to store raw path points
        int64_t           count /*= 0*/;        // number of elements in array
    } pathPoints;
}  PathRequest;

typedef struct {
    int32_t         numPathPoints /*= 0*/;
    float           pathDistSqr /*= 0.0f*/;
    vec3_t          firstMovePoint /*= { 0.0f, 0.0f, 0.0f }*/;
    vec3_t          secondMovePoint /*= { 0.0f, 0.0f, 0.0f }*/;
    PathLinkType    pathLinkType /*= PathLinkType::Walk*/;
    PathReturnCode  returnCode /*= PathReturnCode::StartPathErrors*/;
} PathInfo;

typedef enum
{
    shadow_light_type_point,
    shadow_light_type_cone
} shadow_light_type_t;

typedef struct
{
    shadow_light_type_t lighttype;
    float       radius;
    int         resolution;
    float       intensity /*= 1*/;
    float       fade_start;
    float       fade_end;
    int         lightstyle /*= -1*/;
    float       coneangle /*= 45*/;
    vec3_t      conedirection;
} shadow_light_data_t;

enum server_flags_t
{
    SERVER_FLAGS_NONE           = 0,
    SERVER_FLAG_SLOW_TIME       = 1,
    SERVER_FLAG_INTERMISSION    = 2,
    SERVER_FLAG_LOADING         = 4
};

// todo: cgame?
// todo: pmove
// todo: bots?

typedef struct repro_import_s
{
    uint32_t    tick_rate;
    float       frame_time_s;
    uint32_t    frame_time_ms;

    // broadcast to all clients
    void (*Broadcast_Print)(enum print_type_t printlevel, char *message);

    // print to appropriate places (console, log file, etc)
    void (*Com_Print)(char *msg);

    // print directly to a single client (or nullptr for server console)
    void (*Client_Print)(const edict_t *ent, enum print_type_t printlevel, char *message);

    // center-print to player (legacy function)
    void (*Center_Print)(const edict_t *ent, char *message);

    void (*sound)(const edict_t *ent, enum soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    void (*positioned_sound)(vec3_t origin, edict_t *ent, enum soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    // [Paril-KEX] like sound, but only send to the player indicated by the parameter;
    // this is mainly to handle split screen properly
    void (*local_sound)(edict_t *target, vec3_t* origin, edict_t *ent, enum soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key);

    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    void (*configstring)(int num, const char *string);
    char *(*get_configstring)(int num);

    void (*Com_Error)(const char *message);

    // the *index functions create configstrings and some internal server state
    int (*modelindex)(const char *name);
    int (*soundindex)(const char *name);

    // [Paril-KEX] imageindex can precache both pics for the HUD and
    // textures used for RF_CUSTOMSKIN; to register an image as a texture,
    // the path must be relative to the mod dir and end in an extension
    // ie models/my_model/skin.tga
    int (*imageindex)(const char *name);

    void (*setmodel)(edict_t *ent, const char *name);

    // collision detection
    trace_t (*trace)(vec3_t start, vec3_t* mins, vec3_t* maxs, vec3_t end, edict_t *passent, enum contents_t contentmask);
    // [Paril-KEX] clip the box against the specified entity
    trace_t (*clip)(edict_t *entity, vec3_t start, vec3_t* mins, vec3_t* maxs, vec3_t end, enum contents_t contentmask);
    enum contents_t (*pointcontents)(vec3_t point);
	bool (*inPVS)(vec3_t p1, vec3_t p2, bool portals);
	bool (*inPHS)(vec3_t p1, vec3_t p2, bool portals);
    void (*SetAreaPortalState)(int portalnum, bool open);
    bool (*AreasConnected)(int area1, int area2);

    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent); // call before removing an interactive edict

    // return a list of entities that touch the input absmin/absmax.
    // if maxcount is 0, it will return a count but not attempt to fill "list".
    // if maxcount > 0, once it reaches maxcount, it will keep going but not fill
    // any more of list (the return count will cap at maxcount).
    // the filter function can remove unnecessary entities from the final list; it is illegal
    // to modify world links in this callback.
    size_t (*BoxEdicts)(vec3_t mins, vec3_t maxs, edict_t **list, size_t maxcount, enum solidity_area_t areatype, BoxEdictsFilter_t filter, void *filter_data);

    // network messaging
    void (*multicast)(vec3_t origin, multicast_t to, bool reliable);
    // [Paril-KEX] `dupe_key` is a key unique to a group of calls to unicast
    // that will prevent sending the message on this frame with the same key
    // to the same player (for splitscreen players).
    void (*unicast)(edict_t *ent, bool reliable, uint32_t dupe_key);

    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(char *s);
    void (*WritePosition)(vec3_t pos);
    void (*WriteDir)(vec3_t pos);	  // single byte encoded, very coarse
    void (*WriteAngle)(float f); // legacy 8-bit angle
    void (*WriteEntity)(edict_t *e);

    // managed memory allocation
    void *(*TagMalloc)(size_t size, int tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(int tag);

    // console variable interaction
	cvar_t *(*cvar)(char *var_name, char *value, enum cvar_flags_t flags);
    cvar_t *(*cvar_set)(char *var_name, char *value);
    cvar_t *(*cvar_forceset)(char *var_name, char *value);

    // ClientCommand and ServerCommand parameter access
    int (*argc)();
	char *(*argv)(int n);
	char *(*args)(); // concatenation of all argv >= 1

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    void (*DebugGraph)(float value, int color);

    // Fetch named extension from engine.
    void *(*GetExtension)(char *name);

    // === [KEX] Additional APIs ===

    // bots
    void (*Bot_RegisterEdict)(edict_t * edict);
    void (*Bot_UnRegisterEdict)(edict_t * edict);
    enum goalReturnCode_t (*Bot_MoveToPoint)(edict_t * bot, vec3_t point, float moveTolerance);
    enum goalReturnCode_t (*Bot_FollowActor)(edict_t * bot, edict_t * actor);

    // pathfinding - returns true if a path was found
    bool (*GetPathToGoal)(PathRequest* request, PathInfo* info);

    // localization
    void (*Loc_Print)(edict_t* ent, enum print_type_t level, char* base, char** args, size_t num_args);

    // drawing
    void (*Draw_Line)(vec3_t start, vec3_t end, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Point)(vec3_t point, float size, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Circle)(vec3_t origin, float radius, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Bounds)(vec3_t mins, vec3_t maxs, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Sphere)(vec3_t origin, float radius, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_OrientedWorldText)(vec3_t origin, char * text, rgba_t* color, float size, float lifeTime, bool depthTest);
    void (*Draw_StaticWorldText)(vec3_t origin, vec3_t angles, char * text, rgba_t*  color, float size, float lifeTime, bool depthTest);
    void (*Draw_Cylinder)(vec3_t origin, float halfHeight, float radius, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Ray)(vec3_t origin, vec3_t direction, float length, float size, rgba_t* color, float lifeTime, bool depthTest);
    void (*Draw_Arrow)(vec3_t start, vec3_t end, float size, rgba_t*  lineColor, rgba_t*  arrowColor, float lifeTime, bool depthTest);

    // scoreboard
    void (*ReportMatchDetails_Multicast)(bool is_end);

    // get server frame #
    uint32_t (*ServerFrame)();

    // misc utils
    void (*SendToClipBoard)(char * text);

    // info string stuff
    size_t (*Info_ValueForKey) (char *s, char *key, char *buffer, size_t buffer_len);
    bool (*Info_RemoveKey) (char *s, char *key);
    bool (*Info_SetValueForKey) (char *s, char *key, char *value);
} repro_import_t;

typedef struct repro_export_s {
    int         apiversion;

    // the init function will only be called when a game starts,
    // not each time a level is loaded.  Persistent data for clients
    // and the server can be allocated in init
    void (*PreInit)(void); // [Paril-KEX] called before InitGame, to potentially change maxclients
    void (*Init)(void);
    void (*Shutdown)(void);

    // each new level entered will cause a call to SpawnEntities
    void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

    // Read/Write Game is for storing persistent cross level information
    // about the world state and the clients.
    // WriteGame is called every time a level is exited.
    // ReadGame is called on a loadgame.
    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    char *(*WriteGameJson)(bool autosave, size_t *out_size);
    void (*ReadGameJson)(const char *json);

    // ReadLevel is called after the default map information has been
    // loaded with SpawnEntities
    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    char *(*WriteLevelJson)(bool transition, size_t *out_size);
    void (*ReadLevelJson)(const char *json);

    // [Paril-KEX] game can tell the server whether a save is allowed
    // currently or not.
    bool (*CanSave)(void);

    // [Paril-KEX] choose a free gclient_t slot for the given social ID; for
    // coop slot re-use. Return nullptr if none is available. You can not
    // return a slot that is currently in use by another client; that must
    // throw a fatal error.
    edict_t *(*ClientChooseSlot) (const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore, bool cinematic);
    bool (*ClientConnect)(edict_t *ent, char *userinfo, const char *social_id, bool isBot);
    void (*ClientBegin)(edict_t *ent);
    void (*ClientUserinfoChanged)(edict_t *ent, const char *userinfo);
    void (*ClientDisconnect)(edict_t *ent);
    void (*ClientCommand)(edict_t *ent);
    void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)(bool main_loop);
    // [Paril-KEX] allow the game DLL to clear per-frame stuff
    void (*PrepFrame)(void);

    // ServerCommand will be called when an "sv <command>" command is issued on the
    // server console.
    // The game can issue gi.argc() / gi.argv() commands to get the rest
    // of the parameters
    void (*ServerCommand)(void);

    //
    // global variables shared between game and server
    //

    // The edict array is allocated in the game dll so it
    // can vary in size from one game to another.
    //
    // The size will be fixed when ge->Init() is called
    struct edict_s  *edicts;
    size_t      edict_size;
    uint32_t    num_edicts;     // current number, <= max_edicts
    uint32_t    max_edicts;

    // [Paril-KEX] special flags to indicate something to the server
    enum server_flags_t server_flags;

    // [KEX]: Pmove as export
    void (*Pmove)(pmove_t *pmove); // player movement code called by server & client

    // Fetch named extension from game DLL.
    void *(*GetExtension)(const char *name);

    void    (*Bot_SetWeapon)(edict_t * botEdict, const int weaponIndex, const bool instantSwitch);
    void    (*Bot_TriggerEdict)(edict_t * botEdict, edict_t * edict);
    void    (*Bot_UseItem)(edict_t * botEdict, const int32_t itemID);
    int32_t (*Bot_GetItemID)(const char * classname);
    void    (*Edict_ForceLookAtPoint)(edict_t * edict, const vec3_t point);
    bool    (*Bot_PickedUpItem )(edict_t * botEdict, edict_t * itemEdict);

    // [KEX]: Checks entity visibility instancing
    bool (*Entity_IsVisibleToPlayer)(edict_t* ent, edict_t* player);

    // Fetch info from the shadow light, for culling
    const shadow_light_data_t *(*GetShadowLightData)(int32_t entity_number);


} repro_export_t;

extern repro_import_t gire;

void vrx_repro_getgameapi(repro_import_t *pr, game_import_t *gi);
edict_t *repro_choose_client_slot(const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore, bool cinematic);
void *repro_get_extension(const char *name);
void repro_prep_frame(void);
bool repro_visible_to_player(edict_t* ent, edict_t* player);
const shadow_light_data_t *repro_get_shadow_light_data(int32_t entity_number);

char* repro_write_game_json(bool autosave, size_t *out_size) ;
void repro_read_game_json(const char* json) ;
char* repro_write_level_json(bool autosave, size_t *out_size) ;
void repro_read_level_json(const char* json) ;
void repro_bot_set_weapon(edict_t *botEdict, int weaponIndex, bool instantSwitch);
void repro_bot_trigger_edict(edict_t *botEdict, edict_t *edict);
void repro_bot_use_item(edict_t *botEdict, int32_t itemID);
int32_t repro_bot_get_item_id(const char *classname);
void repro_edict_force_look_at_point(edict_t *edict, const vec3_t point);
bool repro_bot_picked_up_item(edict_t *botEdict, edict_t *itemEdict);

#endif //VORTEXQUAKE2_Q_RECOMPAT_H