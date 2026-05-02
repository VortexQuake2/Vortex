#include "g_local.h"

#ifdef VRX_REPRO

enum {
    SVC_FOG_BIT_DENSITY = 1 << 0,
    SVC_FOG_BIT_R = 1 << 1,
    SVC_FOG_BIT_G = 1 << 2,
    SVC_FOG_BIT_B = 1 << 3,
    SVC_FOG_BIT_TIME = 1 << 4,
    SVC_FOG_BIT_HEIGHTFOG_FALLOFF = 1 << 5,
    SVC_FOG_BIT_HEIGHTFOG_DENSITY = 1 << 6,
    SVC_FOG_BIT_MORE_BITS = 1 << 7,
    SVC_FOG_BIT_HEIGHTFOG_START_R = 1 << 8,
    SVC_FOG_BIT_HEIGHTFOG_START_G = 1 << 9,
    SVC_FOG_BIT_HEIGHTFOG_START_B = 1 << 10,
    SVC_FOG_BIT_HEIGHTFOG_START_DIST = 1 << 11,
    SVC_FOG_BIT_HEIGHTFOG_END_R = 1 << 12,
    SVC_FOG_BIT_HEIGHTFOG_END_G = 1 << 13,
    SVC_FOG_BIT_HEIGHTFOG_END_B = 1 << 14,
    SVC_FOG_BIT_HEIGHTFOG_END_DIST = 1 << 15
};

typedef struct svc_fog_data_s {
    int bits;
    float density;
    int skyfactor;
    int red;
    int green;
    int blue;
    int time;

    float hf_falloff;
    float hf_density;
    int hf_start_r;
    int hf_start_g;
    int hf_start_b;
    int hf_start_dist;
    int hf_end_r;
    int hf_end_g;
    int hf_end_b;
    int hf_end_dist;
} svc_fog_data_t;

static qboolean FogValuesEqual(const float a[5], const float b[5])
{
    for (int i = 0; i < 5; i++)
    {
        if (a[i] != b[i])
            return false;
    }

    return true;
}

static qboolean HeightFogValuesEqual(const height_fog_t *a, const height_fog_t *b)
{
    for (int i = 0; i < 4; i++)
    {
        if (a->start[i] != b->start[i] || a->end[i] != b->end[i])
            return false;
    }

    return a->falloff == b->falloff && a->density == b->density;
}

static int FogColorByte(float value)
{
    return (int)(value * 255.0f);
}

static int FogTransitionMilliseconds(float seconds)
{
    int ms = (int)(seconds * 1000.0f);

    if (ms < 0)
        return 0;
    if (ms > UINT16_MAX)
        return UINT16_MAX;

    return ms;
}

// [Paril-KEX] force the fog transition on the given player,
// optionally instantaneously (ignore any transition time)
void P_ForceFogTransition(edict_t *ent, qboolean instant)
{
    if (!ent || !ent->client || ent->ai.is_bot || !ent->client->pers.connected)
        return;

    // Sanity check; if we're not changing the values, don't bother.
    if (FogValuesEqual(ent->client->fog, ent->client->pers.wanted_fog) &&
        HeightFogValuesEqual(&ent->client->heightfog, &ent->client->pers.wanted_heightfog))
        return;

    svc_fog_data_t fog = { 0 };

    // Check regular fog.
    if (ent->client->pers.wanted_fog[0] != ent->client->fog[0] ||
        ent->client->pers.wanted_fog[4] != ent->client->fog[4])
    {
        fog.bits |= SVC_FOG_BIT_DENSITY;
        fog.density = ent->client->pers.wanted_fog[0];
        fog.skyfactor = FogColorByte(ent->client->pers.wanted_fog[4]);
    }
    if (ent->client->pers.wanted_fog[1] != ent->client->fog[1])
    {
        fog.bits |= SVC_FOG_BIT_R;
        fog.red = FogColorByte(ent->client->pers.wanted_fog[1]);
    }
    if (ent->client->pers.wanted_fog[2] != ent->client->fog[2])
    {
        fog.bits |= SVC_FOG_BIT_G;
        fog.green = FogColorByte(ent->client->pers.wanted_fog[2]);
    }
    if (ent->client->pers.wanted_fog[3] != ent->client->fog[3])
    {
        fog.bits |= SVC_FOG_BIT_B;
        fog.blue = FogColorByte(ent->client->pers.wanted_fog[3]);
    }

    if (!instant && ent->client->pers.fog_transition_time)
    {
        fog.bits |= SVC_FOG_BIT_TIME;
        fog.time = FogTransitionMilliseconds(ent->client->pers.fog_transition_time);
    }

    // Check height fog.
    height_fog_t *hf = &ent->client->heightfog;
    const height_fog_t *wanted_hf = &ent->client->pers.wanted_heightfog;

    if (hf->falloff != wanted_hf->falloff)
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_FALLOFF;
        fog.hf_falloff = wanted_hf->falloff ? wanted_hf->falloff : 0.0f;
    }
    if (hf->density != wanted_hf->density)
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_DENSITY;
        fog.hf_density = wanted_hf->falloff ? wanted_hf->density : 0.0f;
    }

    if (hf->start[0] != wanted_hf->start[0])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_START_R;
        fog.hf_start_r = FogColorByte(wanted_hf->start[0]);
    }
    if (hf->start[1] != wanted_hf->start[1])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_START_G;
        fog.hf_start_g = FogColorByte(wanted_hf->start[1]);
    }
    if (hf->start[2] != wanted_hf->start[2])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_START_B;
        fog.hf_start_b = FogColorByte(wanted_hf->start[2]);
    }
    if (hf->start[3] != wanted_hf->start[3])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_START_DIST;
        fog.hf_start_dist = (int)wanted_hf->start[3];
    }

    if (hf->end[0] != wanted_hf->end[0])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_END_R;
        fog.hf_end_r = FogColorByte(wanted_hf->end[0]);
    }
    if (hf->end[1] != wanted_hf->end[1])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_END_G;
        fog.hf_end_g = FogColorByte(wanted_hf->end[1]);
    }
    if (hf->end[2] != wanted_hf->end[2])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_END_B;
        fog.hf_end_b = FogColorByte(wanted_hf->end[2]);
    }
    if (hf->end[3] != wanted_hf->end[3])
    {
        fog.bits |= SVC_FOG_BIT_HEIGHTFOG_END_DIST;
        fog.hf_end_dist = (int)wanted_hf->end[3];
    }

    if (fog.bits & 0xFF00)
        fog.bits |= SVC_FOG_BIT_MORE_BITS;

    gi.WriteByte(svc_fog);

    if (fog.bits & SVC_FOG_BIT_MORE_BITS)
        gi.WriteShort(fog.bits);
    else
        gi.WriteByte(fog.bits);

    if (fog.bits & SVC_FOG_BIT_DENSITY)
    {
        gi.WriteFloat(fog.density);
        gi.WriteByte(fog.skyfactor);
    }
    if (fog.bits & SVC_FOG_BIT_R)
        gi.WriteByte(fog.red);
    if (fog.bits & SVC_FOG_BIT_G)
        gi.WriteByte(fog.green);
    if (fog.bits & SVC_FOG_BIT_B)
        gi.WriteByte(fog.blue);
    if (fog.bits & SVC_FOG_BIT_TIME)
        gi.WriteShort(fog.time);

    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_FALLOFF)
        gi.WriteFloat(fog.hf_falloff);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_DENSITY)
        gi.WriteFloat(fog.hf_density);

    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_START_R)
        gi.WriteByte(fog.hf_start_r);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_START_G)
        gi.WriteByte(fog.hf_start_g);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_START_B)
        gi.WriteByte(fog.hf_start_b);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_START_DIST)
        gi.WriteLong(fog.hf_start_dist);

    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_END_R)
        gi.WriteByte(fog.hf_end_r);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_END_G)
        gi.WriteByte(fog.hf_end_g);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_END_B)
        gi.WriteByte(fog.hf_end_b);
    if (fog.bits & SVC_FOG_BIT_HEIGHTFOG_END_DIST)
        gi.WriteLong(fog.hf_end_dist);

    gi.unicast(ent, true);

    memcpy(ent->client->fog, ent->client->pers.wanted_fog, sizeof(ent->client->fog));
    ent->client->heightfog = ent->client->pers.wanted_heightfog;
}

#endif
