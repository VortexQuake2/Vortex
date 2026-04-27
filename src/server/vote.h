#pragma once
#include <stdint.h>

struct bot_votes_s {
    bool enabled;
    uint16_t num_bots;
};

//***************************************************************************************
//***************************************************************************************

struct map_votes_s
{
    qboolean	used; // Paril: in "new" system, this means in progress.
    int			mode;
    int			mapindex;
};

enum votetype_t {
    VT_MAP,
    VT_BOTS
};

enum votestate_t {
    VS_UNFINISHED,
    VS_CHANGE_NOW,	// change immediately
    VS_CHANGE_LATER	// change at the completion of vote duration
};

struct vote_s {
    enum votetype_t votetype;
    union {
        struct bot_votes_s botvote;
        struct map_votes_s mapvote;
    };

    uint64_t endframe;
    edict_t *voter;
    char *smode;
    int numVotes, numVoteNo;
    bool running;

    // player data of who started it
    char		name[24];
    char		ip[16];

    // call on success
    void *udata;
    void (*success)(void*);
};

void CheckPlayerVotes(void);
void vrx_change_map(v_maplist_t *maplist, int mapindex, int gamemode);
int vrx_vote_find_best_map(int mode);
v_maplist_t *vrx_get_map_list(int mode);
int vrx_vote_attempt_mode_change(qboolean endlevel);
void vrx_vote_reset(); // az: just for cleanliness
struct vote_s* vrx_vote_get_current();

void vrx_vote_map(edict_t *ent);									//vote for mode/map
void vrx_vote_bots(edict_t *ent);
void vrx_vote_cmd(edict_t *ent);
bool vrx_can_vote(edict_t *ent);
void _vrx_start_vote(edict_t* ent, enum votetype_t vt, void (*success)(void*), void* udata);
void vrx_start_map_vote(edict_t *ent, int mode, int mapnum);
void vrx_start_bot_vote(edict_t *ent, bool enable, int num_bots);
enum votestate_t vrx_vote_is_done();

bool vrx_show_mapmode_vote_menu(edict_t *ent, int players, int min_players);
void vrx_show_bot_vote_menu(edict_t* self);
void vrx_vote_announce(edict_t* ent, const char* msg);
