#include "g_local.h"

//
// ===== Weapon Fire =====
//

void weapon_etf_rifle_fire(edict_t *ent)
{
    vec3_t forward, right, angles, start, offset;
    int i;
    int damage = ETFRIFLE_INITIAL_DAMAGE +
    ( RAILGUN_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_ETFRIFLE].mods[0].current_level );
    int speed = ETFRIFLE_INITIAL_SPEED +
    ( ETFRIFLE_ADDON_SPEED * ent->myskills.weapons[WEAPON_ETFRIFLE].mods[2].current_level );
    int kick = 3;

    if (ent->myskills.weapons[WEAPON_ETFRIFLE].mods[3].current_level < 1) {
        kick *= 2;
    }

    vec3_t kick_origin, kick_angles;

    if (ent->myskills.weapons[WEAPON_ETFRIFLE].mods[4].current_level)
        is_silenced = MZ_SILENCED;

    if (!(ent->client->buttons & BUTTON_ATTACK))
    {
        ent->client->ps.gunframe = 8;
        return;
    }

    if (ent->client->pers.inventory[ent->client->ammo_index] < ent->client->pers.weapon->quantity)
    {
        VectorClear(ent->client->kick_origin);
        VectorClear(ent->client->kick_angles);
        ent->client->ps.gunframe = 8;

        if (level.time >= ent->pain_debounce_time)
        {
            gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            ent->pain_debounce_time = level.time + 1;
        }
        NoAmmoWeaponChange(ent);
        return;
    }

    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    for (i = 0; i < 3; i++)
    {
        kick_origin[i] = crandom() * 0.85f;
        kick_angles[i] = crandom() * 0.85f;
    }

    VectorCopy(kick_origin, ent->client->kick_origin);
    VectorCopy(kick_angles, ent->client->kick_angles);
    VectorAdd(ent->client->v_angle, kick_angles, angles);
    AngleVectors(angles, forward, right, NULL);

    if (ent->client->ps.gunframe == 6)
        VectorSet(offset, 15, 8, ent->viewheight - 8);
    else
        VectorSet(offset, 15, 6, ent->viewheight - 8);

    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    fire_flechette(ent, start, forward, damage, speed, kick);

    if (ent->myskills.weapons[WEAPON_ETFRIFLE].mods[4].current_level < 1) {
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_ETF_RIFLE | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);

        PlayerNoise(ent, start, PNOISE_WEAPON);
    }

    ent->client->vrr.gun_fire_time = level.time + 0.1;
    if (ent->client->buttons & BUTTON_ATTACK)
    {
        if (ent->client->ps.gunframe == 6)
            ent->client->ps.gunframe = 7;
        else
            ent->client->ps.gunframe = 6;
    }
    else
        ent->client->ps.gunframe = 8;
    ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
}

//
// ===== Weapon Think =====
//

void Weapon_ETF_Rifle(edict_t *ent)
{
    static int pause_frames[] = {18, 28, 0};
    static int fire_frames[] = {5, 6, 7, 0};

    if (ent->client->weaponstate == WEAPON_FIRING)
    {
        if (ent->client->pers.inventory[ent->client->ammo_index] <= 0)
            ent->client->ps.gunframe = 8;
    }

    Weapon_Generic(ent, 4, 7, 37, 41, pause_frames, fire_frames, weapon_etf_rifle_fire);
}
