#ifndef VORTEXQUAKE2_V_CHARACTERIO_H
#define VORTEXQUAKE2_V_CHARACTERIO_H

#define SAVEMETHOD_MYSQL 2
#define SAVEMETHOD_SQLITE 3

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

typedef struct {
    int item_id;
    item_t item;
} stash_item_t;

typedef struct {
    // locks stashes, refuses to open if it is locked
    qboolean (*open_stash) (edict_t* ent);

    // unlocks stashes. does nothing if it is not locked.
    qboolean(*close_stash) (edict_t* ent);

    // unlocks stashes. same as close_stash except it takes an id.
    qboolean(*close_stash_by_id) (int owner_id);

    // event shows what item was stored.
    // this call will take the item out of the player inventory.
    qboolean (*store) (edict_t* ent, int itemindex);

    // event shows what item was stored.
    // this call expects it to be put into inventory
    // when the event is called.
    qboolean (*take) (edict_t* ent, int stash_index);

    // the items returned on the on_page event must be freed by the callback.
    qboolean (*get_page) (edict_t* ent, int page, int numitems);
} stash_io_t;

extern stash_io_t vrx_stash_io;

/* v_characterio.c */
void vrx_init_char_io();
void vrx_close_char_io();

#endif //VORTEXQUAKE2_V_CHARACTERIO_H

