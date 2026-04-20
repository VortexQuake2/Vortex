#include "g_local.h"
#include "../../../quake2/monsterframes/m_player.h"

#define HEATBEAM_DM_DMG 15
#define HEATBEAM_SP_DMG 15

//
// ===== Weapon Fire =====
//

void PlasmaBeam_Fire(edict_t *ent)
{
    vec3_t start;
    vec3_t forward, right;
    vec3_t offset;
    int damage = PLASMABEAM_INITIAL_DAMAGE +
    ( PLASMABEAM_ADDON_DAMAGE * ent->myskills.weapons[WEAPON_PLASMABEAM].mods[0].current_level );
    int kick = 75;
    qboolean firing;
    qboolean has_ammo;

    firing = (ent->client->buttons & BUTTON_ATTACK) != 0;
    has_ammo = ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity;

    if (!firing || !has_ammo)
    {
        ent->client->ps.gunframe = 13;
        ent->client->weapon_sound = 0;
        #ifdef VRX_REPRO
        ent->client->ps.gunskin = 0;
        #endif

        if (firing && !has_ammo)
        {
            if (level.time >= ent->pain_debounce_time)
            {
                gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                ent->pain_debounce_time = level.time + 1;
            }
            NoAmmoWeaponChange(ent);
        }
        return;
    }

    if (ent->client->ps.gunframe > 12)
        ent->client->ps.gunframe = 8;
    else
        ent->client->ps.gunframe++;

    if (ent->client->ps.gunframe == 12)
        ent->client->ps.gunframe = 8;

    ent->client->weapon_sound = gi.soundindex("weapons/bfg__l1a.wav");
    #ifdef VRX_REPRO
    ent->client->ps.gunskin = 1;
    #endif //VRX_REPRO

    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    VectorClear(ent->client->kick_origin);
    VectorClear(ent->client->kick_angles);

    AngleVectors(ent->client->v_angle, forward, right, NULL);
    VectorSet(offset, 7, 2, ent->viewheight - 3);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    VectorSet(offset, 2, 7, -3);
    fire_heat(ent, start, forward, offset, damage, kick, false);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_HEATBEAM | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);

    PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;

    ent->client->anim_priority = ANIM_ATTACK;
    if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
    {
        ent->s.frame = FRAME_crattak1 - 1;
        ent->client->anim_end = FRAME_crattak9;
    }
    else
    {
        ent->s.frame = FRAME_attack1 - 1;
        ent->client->anim_end = FRAME_attack8;
    }
}

//
// ===== Weapon Think =====
//

void Weapon_Heatbeam(edict_t *ent)
{
    static int pause_frames[] = {35, 0};
    static int fire_frames[] = {9, 10, 11, 12, 0};

    if (ent->client->weaponstate != WEAPON_FIRING)
    {
        ent->client->weapon_sound = 0;
        #ifdef VRX_REPRO
        ent->client->ps.gunskin = 0;
        #endif //VRX_REPRO
    }

    Weapon_Generic(ent, 8, 12, 42, 47, pause_frames, fire_frames, PlasmaBeam_Fire);
}
