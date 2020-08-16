

#include "g_local.h"

//K03 Begin
void DeflectProjectiles(edict_t *self, float chance, qboolean in_front);

//void DrawPath(void);
void V_PlayerJump(edict_t *ent) {
    ent->lastsound = level.framenum;

    // make player jumping noise
    if (!ent->mtype && (ent->client->pers.inventory[ITEM_INDEX(FindItem("Stealth Boots"))] < 1)
        && (ent->myskills.abilities[CLOAK].disable || (ent->myskills.abilities[CLOAK].current_level < 1))) {
        gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
        PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
    }

    qboolean has_cloak = ent->myskills.abilities[CLOAK].current_level > 0;
    if (ent->mtype == MORPH_MUTANT) {
        if (mutant_boost(ent) && !has_cloak)
            gi.sound(ent, CHAN_VOICE, gi.soundindex("mutant/mutsght1.wav"), 1, ATTN_NORM, 0);
    } else if (ent->mtype == MORPH_MEDIC && !has_cloak) {
        gi.sound(ent, CHAN_VOICE, gi.soundindex("medic/medsght1.wav"), 1, ATTN_NORM, 0);
    } else if (ent->mtype == MORPH_BERSERK && !has_cloak) {
        gi.sound(ent, CHAN_VOICE, gi.soundindex("berserk/sight.wav"), 1, ATTN_NORM, 0);
    } else if (ent->mtype == MORPH_BRAIN) {
        if (mutant_boost(ent) && !has_cloak)
            gi.sound(ent, CHAN_VOICE, gi.soundindex("brain/brnsght1.wav"), 1, ATTN_NORM, 0);
    } else if ((ent->mtype == M_MYPARASITE) || (ent->mtype == MORPH_BRAIN)) {
        ent->velocity[0] *= 1.66;
        ent->velocity[1] *= 1.66;
        //ent->velocity[2] *= 1.66;
        // FIXME: we probably should do this for any ability that can get the player up in the air
        ent->monsterinfo.jumpup = 1; // player has jumped, cleared on touch-down
    }
}

qboolean CanDoubleJump(edict_t *ent, usercmd_t *ucmd) {
    if (ent->client)
        return (!ent->waterlevel && !ent->groundentity && !(ent->v_flags & SFLG_DOUBLEJUMP) && !vrx_has_flag(ent)
                && (ucmd->upmove > 0) && !ent->client->jump && !ent->myskills.abilities[DOUBLE_JUMP].disable
                && (ent->myskills.abilities[DOUBLE_JUMP].current_level > 0));
    else if (PM_MonsterHasPilot(ent))
        return (!ent->waterlevel && !ent->groundentity && !(ent->v_flags & SFLG_DOUBLEJUMP) && !vrx_has_flag(
                ent->activator)
                && (ucmd->upmove > 0) && !ent->activator->client->jump &&
                !ent->activator->myskills.abilities[DOUBLE_JUMP].disable
                && (ent->activator->myskills.abilities[DOUBLE_JUMP].current_level > 0));
    else
        return false;
}

//K03 End