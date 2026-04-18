#include "g_local.h"

// from g_weapon.c
extern void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, float damage, int kick, int hspread, int vspread, int mod);
extern void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, float damage, int kick, int hspread, int vspread, int count, int mod);

//CW++
// Awakening weapons.
/*
 * ======================================================================
 * DESERT EAGLE
 * ======================================================================
 */
void Weapon_DesertEagle_Fire(edict_t *self)
{
    trace_t	tr;
    vec3_t	start;
    vec3_t	forward;
    vec3_t	right;
    vec3_t	offset;
    vec3_t	t_start;
    int		damage;
    int		kick = 2;

    //	Set damage and kick values. There is a 20% chance of doing extra damage.

    damage = MACHINEGUN_INITIAL_DAMAGE + ((random()<0.20)?15:0);
    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    //	Set projectile start position and weapon kick info.

    AngleVectors(self->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0.0, 0.0, self->viewheight);
    P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

    VectorScale(forward, -2.0, self->client->kick_origin);
    self->client->kick_angles[0] = -1.0;

    //	Fire!

    fire_bullet(self, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_DEAGLE);
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(self-g_edicts);
    gi.WriteByte(MZ_BLASTER);
    gi.multicast(self->s.origin, MULTICAST_PVS);
    self->client->ps.gunframe++;
    self->client->machinegun_shots++;
    PlayerNoise(self, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        self->client->pers.inventory[self->client->ammo_index]--;

    //	Spawn a tracer sometimes (50% chance, if the path is clear).

    VectorMA(start, 200.0, forward, t_start);
    tr = gi.trace(start, NULL, NULL, t_start, self, MASK_SHOT);
    //if ((tr.fraction == 1.0) && (random() < 0.5))
        //Fire_Tracer(self, t_start, forward, 2000.0, 0.2);
}

void Weapon_DesertEagle(edict_t *self)
{
    static int	pause_frames[]	= {19, 32, 0};
    static int	fire_frames[]	= {6, 0};

    Weapon_Generic(self, 5, 8, 52, 55, pause_frames, fire_frames, Weapon_DesertEagle_Fire);
}

#define DEFAULT_JACKHAMMER_COUNT 6

/*
 * ======================================================================
 * JACKHAMMER
 * ======================================================================
 */
void Weapon_Jackhammer_Fire(edict_t *self)
{
    vec3_t	start;
    vec3_t	forward;
    vec3_t	right;
    vec3_t	offset;
    int		kick = 8;

    //	Set damage and kick values.

    float damage = SUPERSHOTGUN_INITIAL_DAMAGE +
    SUPERSHOTGUN_ADDON_DAMAGE * self->myskills.weapons[WEAPON_SUPERSHOTGUN].mods[0].current_level;

    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    //	Set projectile start position and weapon kick info.

    AngleVectors(self->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0.0, 8.0,  self->viewheight-8.0);
    P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

    VectorScale(forward, -2.0, self->client->kick_origin);
    self->client->kick_angles[0] = -2.0;

    //	Fire!

    fire_shotgun(self, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, DEFAULT_JACKHAMMER_COUNT, MOD_JACKHAMMER);
    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(self-g_edicts);
    gi.WriteByte(MZ_SSHOTGUN);
    gi.multicast(self->s.origin, MULTICAST_PVS);
    self->client->ps.gunframe++;
    self->client->machinegun_shots++;
    PlayerNoise(self, start, PNOISE_WEAPON);

    if (!((int)dmflags->value & DF_INFINITE_AMMO))
        self->client->pers.inventory[self->client->ammo_index]--;
}

void Weapon_Jackhammer(edict_t *self)
{
    static int	pause_frames[]	= {45, 0};
    static int	fire_frames[]	= {4, 0};

    Weapon_Generic(self, 3, 5, 45, 49, pause_frames, fire_frames, Weapon_Jackhammer_Fire);
}


/*
 * ======================================================================
 * MAC-10
 * ======================================================================
 */
void Weapon_Mac10_Fire(edict_t *self)
{
    vec3_t	start;
    vec3_t	forward;
    vec3_t	right;
    vec3_t	offset;
    vec3_t	t_start;
    trace_t	tr;
    int		kick = 2;
    int		i;

    //	Set damage and kick values.

    float damage = MACHINEGUN_INITIAL_DAMAGE +
    MACHINEGUN_ADDON_DAMAGE * self->myskills.weapons[WEAPON_MACHINEGUN].mods[0].current_level;

    if (is_quad)
    {
        damage *= 4;
        kick *= 4;
    }

    //	Set projectile start position and weapon kick info.

    AngleVectors(self->client->v_angle, forward, right, NULL);
    VectorSet(offset, 0.0, 0.0, self->viewheight);
    P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);

    VectorScale(forward, -2.0, self->client->kick_origin);
    self->client->kick_angles[0] = -1.0;
    for (i = 1; i < 3; ++i)
        self->client->kick_angles[i] = (random() < 0.5)?-1.0:1.0;

    //	Fire! (two bullets per trigger press).

    for (i = 0; i < 2; ++i)
    {
        fire_bullet(self, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MAC10);

        if (!((int)dmflags->value & DF_INFINITE_AMMO))
            self->client->pers.inventory[self->client->ammo_index]--;

        //		Check if there's enough ammo; swap weapons if we're dry.

        if (self->client->pers.inventory[self->client->ammo_index] < 1)
        {
            self->client->ps.gunframe = 5;
            if (level.time >= self->pain_debounce_time)
            {
                gi.sound(self, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
                self->pain_debounce_time = level.time + 1.0;
            }
            self->client->machinegun_shots = 0;
            NoAmmoWeaponChange(self);
            return;
        }
    }

    self->client->ps.gunframe++;
    self->client->machinegun_shots++;

    //	Spawn a tracer every 4 shots.

    if ((self->client->machinegun_shots % 4) == 0)
    {
        VectorSet(offset, 24.0, 7.0, self->viewheight-6.0);
        P_ProjectSource(self->client, self->s.origin, offset, forward, right, start);
        VectorMA(start, 300.0, forward, t_start);
        tr = gi.trace(start, NULL, NULL, t_start, self, MASK_SHOT);
        //if (tr.fraction == 1.0)
        //    Fire_Tracer(self, t_start, forward, 2000.0, 0.3);
    }

    //	Loop the animation sequence if the firing button is still being pressed.

    if ((self->client->ps.gunframe == 5) && (self->client->buttons & BUTTON_ATTACK))
        self->client->ps.gunframe = 3;

    //	Do the muzzleflash and sound.

    gi.WriteByte(svc_muzzleflash);
    gi.WriteShort(self-g_edicts);
    gi.WriteByte(MZ_MACHINEGUN);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    PlayerNoise(self, start, PNOISE_WEAPON);
}

void Weapon_Mac10(edict_t *self)
{
    static int	pause_frames[]	= {29, 45, 0};
    static int	fire_frames[]	= {3, 4, 0};

    Weapon_Generic(self, 2, 4, 45, 49, pause_frames, fire_frames, Weapon_Mac10_Fire);
}
