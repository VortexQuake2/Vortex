#include "g_local.h"



qboolean curse_add(edict_t *target, edict_t *caster, int type, int curse_level, float duration);

void EatCorpses(edict_t *ent) {
    int value, gain;
    vec3_t forward, right, start, end, offset;
    trace_t tr;
    qboolean sound = false;

    if (ent->myskills.abilities[FLESH_EATER].disable)
        return;

    if (!V_CanUseAbilities(ent, FLESH_EATER, 0, false))
        return;

    value = CORPSEEATER_RANGE;
    if (ent->mtype)
        value += ent->maxs[1] - 16;

    // get starting position
    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0, 7, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    // get ending position and trace
    VectorMA(start, value, forward, end);
    tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);

    if (G_EntExists(tr.ent) && (level.time > ent->corpseeater_time)) {
        //Get a multiplier of how much higher the player can go over their max health
        float maxhp = 1.0 + (CORPSEEATER_ADDON_MAXHEALTH * ent->myskills.abilities[FLESH_EATER].current_level);

        //Talent: Cannibalism
        int talentLevel = vrx_get_talent_level(ent, TALENT_CANNIBALISM);
        if (talentLevel > 0) maxhp += 0.1 * talentLevel;    //+0.1x per upgrade

        //Figure out what their max health is
        maxhp *= ent->max_health;

        if (tr.ent->health < 1 && !(tr.ent->flags & FL_UNDEAD)) {
            // kill the corpse
            T_Damage(tr.ent, tr.ent, tr.ent, vec3_origin, tr.ent->s.origin, vec3_origin,
                     10000, 0, DAMAGE_NO_PROTECTION, 0);

            //heal the player
            gain = CORPSEEATER_INITIAL_HEALTH +
                   (CORPSEEATER_ADDON_HEALTH * ent->myskills.abilities[FLESH_EATER].current_level);
            if (ent->health < maxhp) {
                ent->megahealth = NULL;
                ent->health += gain;
                if (ent->health > maxhp)
                    ent->health = maxhp;

                // sync up player-monster's health
                if (PM_PlayerHasMonster(ent))
                    ent->owner->health = ent->health;
            }
            sound = true;
        } else if (!OnSameTeam(ent, tr.ent)) {
            float chance;

            //damage the target
            VectorSubtract(tr.ent->s.origin, ent->s.origin, forward);
            value = CORPSEEATER_INITIAL_DAMAGE +
                    (CORPSEEATER_ADDON_DAMAGE * ent->myskills.abilities[FLESH_EATER].current_level);
            gain = 0.5 *
                   T_Damage(tr.ent, ent, ent, forward, tr.endpos, tr.plane.normal, value, -value, 0, MOD_CORPSEEATER);

            //heal the player
            if (ent->health < maxhp) {
                ent->megahealth = NULL;
                ent->health += gain;
                if (ent->health > maxhp)
                    ent->health = maxhp;

                // sync up player-monster's health
                if (PM_PlayerHasMonster(ent))
                    ent->owner->health = ent->health;
            }
            sound = true;

            //Talent: Fatal Wound
            talentLevel = vrx_get_talent_level(ent, TALENT_FATAL_WOUND);
            chance = 0.1 * talentLevel;
            if (talentLevel > 0 && random() <= chance) {
                curse_add(tr.ent, ent, BLEEDING, 30, 10.0);
                if (tr.ent->client)
                    safe_cprintf(tr.ent, PRINT_HIGH, "You have been fatally wounded!\n");
            }
        }

        if (sound) gi.sound(ent, CHAN_ITEM, gi.soundindex("brain/brnpain2.wav"), 1, ATTN_NORM, 0);
        ent->corpseeater_time = level.time + CORPSEEATER_DELAY;
    }
}