#include "g_local.h"

//
// ===== Weapon Fire =====
//

static void weapon_chainfist_fire(edict_t *ent)
{
    vec3_t start, dir;
    vec3_t forward, right;
    int frame = ent->client->ps.gunframe;
    int damage = 30 + 2 * ent->myskills.weapons[WEAPON_CHAINFIST].mods[0].current_level;
    int kick = 80;

    if (!(ent->client->buttons & BUTTON_ATTACK))
    {
        if ((frame == 13) || (frame == 23) || (frame >= 32))
        {
            ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");
            ent->client->ps.gunframe = 33;
            return;
        }
    }

    if (is_quad)
        damage *= 4;

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorSet(start, 0, 0, ent->viewheight - 4);
    P_ProjectSource(ent->client, ent->s.origin, start, forward, right, start);
    VectorCopy(forward, dir);

    if (ent->client->buttons & BUTTON_ATTACK)
    {
        if (fire_player_melee(ent, start, dir, 32, damage, kick, MOD_HIT))
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sawhit.wav"), 1, ATTN_NORM, 0);
    }

    PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
    ent->client->ps.gunframe++;
    ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");

    if (ent->client->buttons & BUTTON_ATTACK)
    {
        if (ent->client->ps.gunframe >= 32)
            ent->client->ps.gunframe = 7;
    }
}

//
// ===== Weapon Think =====
//

void Weapon_ChainFist(edict_t *ent)
{
    static int pause_frames[] = {0};
    static int fire_frames[] = {
        5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
        29, 30, 31, 32, 0
    };

    if (ent->client->weaponstate == WEAPON_DROPPING)
        ent->client->weapon_sound = 0;
    else if ((ent->client->weaponstate == WEAPON_FIRING) && (ent->client->buttons & BUTTON_ATTACK))
        ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");
    else
        ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");

    Weapon_Generic(ent, 4, 32, 57, 60, pause_frames, fire_frames, weapon_chainfist_fire);
}
