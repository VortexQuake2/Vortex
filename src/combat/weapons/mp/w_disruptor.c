#include "g_local.h"

//
// ===== Weapon Fire =====
//

static void weapon_disruptor_fire(edict_t *ent)
{
    vec3_t offset, start;
    vec3_t forward, right;
    vec3_t mins, maxs, end;
    trace_t tr;
    edict_t *enemy = NULL;

    int damage = DISRUPTOR_INITIAL_DAMAGE +
    ( DISRUPTOR_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_DISRUPTOR].mods[0].current_level );
    int speed = DISRUPTOR_INITIAL_SPEED +
    ( DISRUPTOR_ADDON_SPEED * ent->myskills.weapons[WEAPON_DISRUPTOR].mods[1].current_level );

    if (is_quad)
        damage *= 4;

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorScale(forward, -2, ent->client->kick_origin);
    ent->client->kick_angles[0] = -1;

    VectorSet(offset, 24, 8, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    VectorSet(mins, -16, -16, -16);
    VectorSet(maxs, 16, 16, 16);
    VectorMA(start, 8192, forward, end);

    tr = gi.trace(start, NULL, NULL, end, ent, MASK_SHOT);
    if ((tr.ent != world) && G_ValidTarget(ent, tr.ent, false, true))
        enemy = tr.ent;
    else
    {
        tr = gi.trace(start, mins, maxs, end, ent, MASK_SHOT);
        if ((tr.ent != world) && G_ValidTarget(ent, tr.ent, false, true))
            enemy = tr.ent;
    }

    fire_disruptor(ent, start, forward, damage, speed, enemy);

    gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/disint2.wav"), 1, ATTN_NORM, 0);

    if (ent->myskills.weapons[WEAPON_DISRUPTOR].mods[4].current_level < 1)
    {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_TRACKER | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
        PlayerNoise(ent, start, PNOISE_WEAPON);
    }

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;

    ent->client->ps.gunframe++;
}


//
// ===== Weapon Think =====
//

void Weapon_Disruptor(edict_t *ent)
{
    static int pause_frames[] = { 14, 19, 23, 0 };
    static int fire_frames[] = { 5, 0 };

    Weapon_Generic(ent, 4, 9, 29, 34, pause_frames, fire_frames, weapon_disruptor_fire);
}

