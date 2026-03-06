//
// Created by zardoru on 01-03-26.
//

#include "game.h"
#include "q_recompat.h"

#include <stdarg.h>


void vrx_repro_shim(game_import_t *gi);

repro_import_t gire;

void vrx_repro_getgameapi(repro_import_t *pr, game_import_t *gi) {
    memcpy(pr, &gire, sizeof(repro_import_t));
    vrx_repro_shim(gi);
}

#define VA_PRELUDE(x) va_list argptr;\
    va_start(argptr, fmt);\
    int len = vsnprintf(NULL, 0, fmt, argptr);\
    char _##x[len];\
    vsnprintf(_##x, len, fmt, argptr);\
    va_end(argptr);

void	shim_bprintf (int printlevel, const char *fmt, ...) {
    VA_PRELUDE(msg)
    gire.Broadcast_Print(printlevel, _msg);
}

void	shim_dprintf (const char *fmt, ...) {
    VA_PRELUDE(msg)
    gire.Com_Print(_msg);
}
void	shim_cprintf (const edict_t *ent, int printlevel, const char *fmt, ...) {
    VA_PRELUDE(msg)
    gire.Client_Print(ent, printlevel, _msg);
}
void	shim_centerprintf (const edict_t *ent,  const char *fmt, ...) {
    VA_PRELUDE(msg)
    gire.Center_Print(ent, _msg);
}

void shim_error(const char* fmt, ...) {
    VA_PRELUDE(msg)
    gire.Com_Error(_msg);
}

void vrx_repro_shim(game_import_t *gi) {
    gi->bprintf = shim_bprintf;
    gi->dprintf = shim_dprintf;
    gi->cprintf = shim_cprintf;
    gi->centerprintf = shim_centerprintf;
    gi->sound = gire.sound;
    gi->positioned_sound = gire.positioned_sound;

    gi->configstring = gire.configstring;

    gi->error = shim_error;

    gi->modelindex = gire.modelindex;
    gi->soundindex = gire.soundindex;
    gi->imageindex = gire.imageindex;

    gi->setmodel = gire.setmodel;

    gi->trace = gire.trace;
    gi->pointcontents = gire.pointcontents;
    gi->inPVS = gire.inPVS;
    gi->inPHS = gire.inPHS;
    gi->SetAreaPortalState = gire.SetAreaPortalState;
    gi->AreasConnected = gire.AreasConnected;

    gi->linkentity = gire.linkentity;
    gi->unlinkentity = gire.unlinkentity;
    gi->BoxEdicts = gire.BoxEdicts;

    // TODO
    // gi->Pmove = gire.PMo;


    gi->multicast = gire.multicast;
    gi->unicast = gire.unicast;

    gi->WriteChar = gire.WriteChar;
    gi->WriteByte = gire.WriteByte;
    gi->WriteShort = gire.WriteShort;
    gi->WriteLong = gire.WriteLong;
    gi->WriteFloat = gire.WriteFloat;
    gi->WriteString = gire.WriteString;
    gi->WritePosition = gire.WritePosition;
    gi->WriteDir = gire.WriteDir;
    gi->WriteAngle = gire.WriteAngle;

    gi->TagMalloc = gire.TagMalloc;
    gi->TagFree = gire.TagFree;
    gi->FreeTags = gire.FreeTags;

    gi->cvar = gire.cvar;
    gi->cvar_set = gire.cvar_set;
    gi->cvar_forceset = gire.cvar_forceset;

    gi->argc = gire.argc;
    gi->argv = gire.argv;
    gi->args = gire.args;

    gi->AddCommandString = gire.AddCommandString;
    gi->DebugGraph = gire.DebugGraph;
}

#ifdef VRX_REPRO
void pm_set_viewheight(pmove_t *pm, int viewheight) {
    VectorSet(pm->viewoffset, 0, 0, viewheight);
}

int pm_get_viewheight(pmove_t *pm) {
    return pm->viewoffset[2];
}
#else
void pm_set_viewheight(pmove_t *pm, int viewheight) {
    pm->viewheight = viewheight;
}

void pm_set_viewheight(pmove_t *pm) {
    return pm->viewheight;
}
#endif