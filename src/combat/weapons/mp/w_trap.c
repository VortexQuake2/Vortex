#include "g_local.h"

#define TRAP_TIMER              5.0
#define TRAP_MINSPEED           500
#define TRAP_MAXSPEED           900

//
// ===== Weapon Fire =====
//

void weapon_trap_fire(edict_t *ent, qboolean held)
{
    vec3_t offset, forward, right, start, angles;
    int damage = 125 + 4 * ent->myskills.weapons[WEAPON_TRAP].mods[0].current_level;
    float timer;
    int speed;
    int min_speed;
    int max_speed;
    float radius = damage + 40 + 2 * ent->myskills.weapons[WEAPON_TRAP].mods[1].current_level;

    if (is_quad)
        damage *= 4;

    VectorCopy(ent->client->v_angle, angles);
    if (angles[PITCH] < -62.5f)
        angles[PITCH] = -62.5f;

    VectorSet(offset, 8, 0, ent->viewheight - 8);
    AngleVectors(angles, forward, right, NULL);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

    timer = ent->client->grenade_time - level.time;
    if (timer < 0)
        timer = 0;

    min_speed = TRAP_MINSPEED + 10 * ent->myskills.weapons[WEAPON_TRAP].mods[2].current_level;
    max_speed = TRAP_MAXSPEED + 10 * ent->myskills.weapons[WEAPON_TRAP].mods[2].current_level;

    if (ent->health <= 0)
        speed = min_speed;
    else
        speed = min_speed + (TRAP_TIMER - timer) * ((max_speed - min_speed) / TRAP_TIMER);

    if (speed > max_speed)
        speed = max_speed;

    fire_trap(ent, start, forward, damage, speed, timer, radius, held);

    ent->client->pers.inventory[ent->client->ammo_index]--;
    ent->client->grenade_time = level.time + 1.0;
}

//
// ===== Weapon Think =====
//

void Weapon_Trap(edict_t *ent)
{
    if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
    {
        ChangeWeapon(ent);
        return;
    }

    if (ent->client->weaponstate == WEAPON_ACTIVATING)
    {
        ent->client->weaponstate = WEAPON_READY;
        ent->client->ps.gunframe = 16;
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

        if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) ||
            (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
        {
            if (rand() & 15)
                return;
        }

        if (++ent->client->ps.gunframe > 48)
            ent->client->ps.gunframe = 16;
        return;
    }

    if (ent->client->weaponstate == WEAPON_FIRING)
    {
        if (ent->client->ps.gunframe == 5)
            gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/trapcock.wav"), 1, ATTN_NORM, 0);

        if (ent->client->ps.gunframe == 11)
        {
            if (!ent->client->grenade_time)
            {
                ent->client->grenade_time = level.time + TRAP_TIMER + 0.2;
                ent->client->weapon_sound = gi.soundindex("weapons/traploop.wav");
            }

            if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
            {
                ent->client->weapon_sound = 0;
                weapon_trap_fire(ent, true);
                ent->client->grenade_blew_up = true;
            }

            if (ent->client->buttons & BUTTON_ATTACK)
                return;

            if (ent->client->grenade_blew_up)
            {
                if (level.time >= ent->client->grenade_time)
                {
                    ent->client->ps.gunframe = 15;
                    ent->client->grenade_blew_up = false;
                }
                else
                {
                    return;
                }
            }
        }

        if (ent->client->ps.gunframe == 12)
        {
            ent->client->weapon_sound = 0;
            weapon_trap_fire(ent, false);
            if (ent->client->pers.inventory[ent->client->ammo_index] == 0)
                NoAmmoWeaponChange(ent);
        }

        if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
            return;

        ent->client->ps.gunframe++;

        if (ent->client->ps.gunframe == 16)
        {
            ent->client->grenade_time = 0;
            ent->client->weaponstate = WEAPON_READY;
        }
    }
}
