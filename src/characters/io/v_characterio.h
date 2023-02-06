#ifndef VORTEXQUAKE2_V_CHARACTERIO_H
#define VORTEXQUAKE2_V_CHARACTERIO_H

typedef struct {
    qboolean (*save_player)(edict_t* ent); // saves player, but does not change playing status.
    qboolean (*save_close_player)(edict_t* ent); // like save_player, but changes playing status and unlocks the player for use in other servers.
    qboolean (*save_player_runes)(edict_t* ent); // only saves runes, must not change playing status.
    qboolean (*load_player)(edict_t* ent); // if successful, it "locks" the player in a playing state.
    qboolean (*character_exists) (edict_t* ent); // checking char existence should be skipped if this function is null.
    qboolean (*is_loading) (edict_t* ent); // if it's already loading... don't bother calling load_player again.
    void (*handle_status)(edict_t* ent); // if not null, this is a multithreaded io system.
} char_io_t;

extern char_io_t vrx_char_io;

void vrx_init_char_io();
void vrx_close_char_io();

#endif //VORTEXQUAKE2_V_CHARACTERIO_H
