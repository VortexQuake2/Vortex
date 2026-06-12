#include "g_local.h"
#include "../../quake2/monsterframes/m_player.h"

//
// ===== Weapon Fire =====
//

void weapon_railgun_fire(edict_t* ent) {
    vec3_t start;
    vec3_t forward, right;
    vec3_t offset;
    int kick = 200;

    //K03 Begin
    int damage =
    RAILGUN_INITIAL_DAMAGE + RAILGUN_ADDON_DAMAGE * ent->client->resp.pstats.weapons[WEAPON_RAILGUN].mods[0].current_level;
    //K03 End

    if (is_quad) {
        damage *= 4;
        kick *= 4;
    }

    // sniper shots deal massive damage
    if (ent->client->weapon_mode) {
        damage *= 3;
        is_silenced = MZ_SILENCED;
    }

    AngleVectors(ent->client->v_angle, forward, right, NULL);

    VectorScale(forward, -3, ent->client->kick_origin);
    ent->client->kick_angles[0] = -3;

    VectorSet(offset, 0, 7, ent->viewheight - 8);

    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    fire_rail(ent, start, forward, damage, kick);

    if (ent->client->resp.pstats.weapons[WEAPON_RAILGUN].mods[4].current_level < 1) {
        // send muzzle flash
        gi.WriteByte(svc_muzzleflash);
        gi.WriteShort(ent - g_edicts);
        gi.WriteByte(MZ_RAILGUN | is_silenced);
        gi.multicast(ent->s.origin, MULTICAST_PVS);
    }
    //K03 End

    ent->client->ps.gunframe++;
    //K03 Begin
    if (ent->client->resp.pstats.weapons[WEAPON_RAILGUN].mods[4].current_level < 1)
        PlayerNoise(ent, start, PNOISE_WEAPON);
    //K03 End

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index]--;

    ent->client->refire_frames = 0;
}

//
// ===== Weapon Think =====
//

void Weapon_Railgun(edict_t* ent) {
    static int pause_frames[] = { 56, 0 };
    static int fire_frames[] = { 4, 0 };
    const int fire_last = 18;

    if (ent->mtype)
        return;

    // are we in sniper mode?
    if (ent->client->weapon_mode) {
        // fire when button is released
        if (!(ent->client->buttons & BUTTON_ATTACK)) {
            if (ent->client->ps.gunframe > (fire_last + 1) && ent->client->refire_frames >= 2 * (fire_last - 2))
                ent->client->buttons |= BUTTON_ATTACK;
            else
                ent->client->refire_frames = 0;
            ent->client->snipertime = 0;//4.5
            lasersight_off(ent);
        }
        else {
            // charge up weapon
            if (ent->client->refire_frames == 2 * (fire_last - 2))
                gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
            // make sure laser sight is on to alert enemies
            if ((ent->client->refire_frames >= 2 * (fire_last - 2)) && !ent->lasersight)
                lasersight_on(ent);
            // dont fire yet
            ent->client->buttons &= ~BUTTON_ATTACK;
            ent->client->latched_buttons &= ~BUTTON_ATTACK;
            ent->client->refire_frames++;
            ent->client->snipertime = level.time + FRAMETIME;
        }
    }
    Weapon_Generic(ent, 3, fire_last, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
}
