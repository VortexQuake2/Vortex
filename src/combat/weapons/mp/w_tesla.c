#include "g_local.h"

#define TESLA_TIMER             3.0
#define TESLA_MINSPEED          600
#define TESLA_MAXSPEED          900

//
// ===== Weapon Fire =====
//

static void weapon_tesla_fire(edict_t *ent, qboolean held)
{
    vec3_t offset, start, forward, right, angles;
    int damage = 3 + ent->client->resp.pstats.weapons[WEAPON_TESLA].mods[0].current_level;
    float radius = 128 + 4 * ent->client->resp.pstats.weapons[WEAPON_TESLA].mods[1].current_level;
    float timer;
    int speed;
    int min_speed;
    int max_speed;

    (void) held;

    if (is_quad)
        damage *= 4;

    VectorCopy(ent->client->v_angle, angles);
    if (angles[PITCH] < -62.5f)
        angles[PITCH] = -62.5f;

    VectorSet(offset, 0, 0, ent->viewheight - 22);
    AngleVectors(angles, forward, right, NULL);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    timer = ent->client->grenade_time - level.time;
    if (timer < 0)
        timer = 0;

    min_speed = TESLA_MINSPEED + 15 * ent->client->resp.pstats.weapons[WEAPON_TESLA].mods[2].current_level;
    max_speed = TESLA_MAXSPEED + 15 * ent->client->resp.pstats.weapons[WEAPON_TESLA].mods[2].current_level;

    if (ent->health <= 0)
        speed = min_speed;
    else
        speed = min_speed + (TESLA_TIMER - timer) * ((max_speed - min_speed) / TESLA_TIMER);

    if (speed > max_speed)
        speed = max_speed;

    fire_tesla(ent, start, forward, damage, speed, radius);

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(ent - g_edicts);
    gi.WriteByte(MZ_GRENADE | is_silenced);
    gi.multicast(ent->s.origin, MULTICAST_PVS);
    PlayerNoise(ent, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
}

//
// ===== Weapon Think =====
//

void Weapon_Tesla(edict_t *ent)
{
    if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
    {
        ChangeWeapon(ent);
        return;
    }

    if (ent->client->weaponstate == WEAPON_ACTIVATING)
    {
        ent->client->weaponstate = WEAPON_READY;
        ent->client->ps.gunframe = 9;
        return;
    }

    if (ent->client->weaponstate == WEAPON_READY)
    {
        if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK))
        {
            ent->client->latched_buttons &= ~BUTTON_ATTACK;
            if (ent->client->pers.inventory[ent->client->ammo_index])
            {
                ent->client->ps.gunframe = 1;
                ent->client->weaponstate = WEAPON_FIRING;
                ent->client->grenade_time = 0;
            }
            else
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

        if (ent->client->ps.gunframe == 21)
        {
            if (randomMT() & 15)
                return;
        }

        if (++ent->client->ps.gunframe > 32)
            ent->client->ps.gunframe = 9;
        return;
    }

    if (ent->client->weaponstate == WEAPON_FIRING)
    {
        if (ent->client->ps.gunframe == 1)
        {
            if (!ent->client->grenade_time)
                ent->client->grenade_time = level.time + TESLA_TIMER + 0.2;

            if (ent->client->buttons & BUTTON_ATTACK)
                return;
        }

        if (ent->client->ps.gunframe == 2)
        {
            weapon_tesla_fire(ent, false);
            if (ent->client->pers.inventory[ent->client->ammo_index] == 0)
                NoAmmoWeaponChange(ent);
        }

        ent->client->ps.gunframe++;

        if (ent->client->ps.gunframe == 9)
        {
            ent->client->grenade_time = 0;
            ent->client->weaponstate = WEAPON_READY;
        }
    }
}
