#include <stdint.h>
#include "g_local.h"

/* az: override for gi.soundindex because q2pro keeps crashing lol. */

gi_sound_func_t original_sound;
char my_snd_configstrings[MAX_SOUNDS][MAX_QPATH];
uint64_t my_snd_used_index[MAX_SOUNDS];
uint64_t lru_index;

void gi_sound_override (edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
int gi_soundindex_override(char* str);

// to be called after getgameapi
void cs_override_init() {
    cs_reset();

    // save original gi.sound. we can use it
    original_sound = gi.sound;
    gi.sound = gi_sound_override;

    // az: we don't save you. so long, dumb default implementation.
    gi.soundindex = gi_soundindex_override; 
}

void cs_reset() {
    memset(my_snd_configstrings, 0, sizeof my_snd_configstrings);
    memset(my_snd_used_index, 0, sizeof my_snd_used_index);
    lru_index = 1;
}

// override gi.configstring
int gi_soundindex_override(char* str) {
    int i = 0;

    if (!str || !str[0])
        return 0; // az: come on lol

    // az: do basically the same quake 2 already does.
    // q2pro starts counting at 1 in PF_FindIndex so whatever.
    for (i = 1; i < MAX_SOUNDS; i++) {

        // doesn't exist
        if (!my_snd_configstrings[i][0]) {
            strncpy(my_snd_configstrings[i], str, MAX_QPATH);
            gi.configstring(CS_SOUNDS + i, my_snd_configstrings[i]);
            return i;
        }

        // does it exist?
        if (strcmp(my_snd_configstrings[i], str) == 0) {
            return i;
        }
    }

    // az: well at this point quake 2 would crash so let's do this lru thing.
    if (i == MAX_SOUNDS) {
        uint64_t min_value = UINT64_MAX;
        int      min_index = INT_MAX;

        // find the least recently used (smallest my_snd_used_index value)
        for (i = 1; i < MAX_SOUNDS; i++) {
            if (my_snd_used_index[i] < min_value) {
                min_index = i;
                min_value = my_snd_used_index[i];
            }
        }

        // got our least used index, just override it. fuck it
        strncpy(my_snd_configstrings[min_index], str, MAX_QPATH);
        gi.configstring(CS_SOUNDS + min_index, my_snd_configstrings[min_index]);
        return min_index;
    }

    gi.dprintf("soundindex override: couldn't find a valid index for %s? wtf\n", str);
    return 0; // az: whatever.
}

void gi_sound_override (edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs) {
    if (soundindex >= 0 && soundindex < MAX_SOUNDS) {
        my_snd_used_index[soundindex] = lru_index++; // we've used this sound last...
        original_sound(ent, channel, soundindex, volume, attenuation, timeofs);
    }
}

