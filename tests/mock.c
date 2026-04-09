#include "g_local.h"
#include "mock.h"

#include "munit.h"
#include <stdarg.h>

// #define TRUEPRINT

void nullprintf(const char* ch, ...) {
#ifdef TRUEPRINT
    va_list ap;
    va_start(ap, ch);
    auto size = vsnprintf(nullptr, 0, ch, ap);
    va_end(ap);

    if (size < 0) {
        return;
    }

    auto buffer = munit_malloc(size + 1);
    va_start(ap, ch);
    vsnprintf(buffer, size + 1, ch, ap);
    printf("%s", buffer);
    va_end(ap);
    free(buffer)
    #endif;
}

void nullbprintf(int printlevel, const char* ch, ...) {
#ifdef TRUEPRINT
    va_list ap;
    va_start(ap, ch);
    auto size = vsnprintf(nullptr, 0, ch, ap);
    va_end(ap);

    if (size < 0) {
        return;
    }

    auto buffer = munit_malloc(size + 1);
    va_start(ap, ch);
    vsnprintf(buffer, size + 1, ch, ap);
    printf("%s", buffer);
    va_end(ap);
    free(buffer);
#endif
}

int nullmodelindex(const char*) { return 0; }
void* nullmalloc(size_t size, int tag) { return munit_malloc(size); }
void initialize_item_references(void);
char* nullprint(const char* ch) {
    return strdup(ch);
}

void nullsound(const struct edict_s * edict_s, enum soundchan_t soundchan, int i, float x, float arg, float x1) {}

void mock_init() {
    gi.dprintf = nullprintf;
    gi.bprintf = nullbprintf;
    gi.modelindex = nullmodelindex;
    gi.TagMalloc = nullmalloc;
    gi.TagFree = free;
    gi.soundindex = nullmodelindex;
    gi.sound = nullsound;

    // HiPrint = nullprint;
    InitItems();
    initialize_item_references();
    vrx_prestige_global_init();
}

cvar_t* mock_cvar(const char* name, const char* value)
{
    cvar_t* cvar = munit_malloc(sizeof(cvar_t));
    cvar->name = strdup(name);
    cvar->string = strdup(value);

    cvar->flags = 0;

    cvar->integer = strtol(value, NULL, 10);

    sscanf(value, "%f", &cvar->value);
    return cvar;
}

void mock_free_cvar(cvar_t* cvar)
{
    free(cvar->name);
    free(cvar->string);
    free(cvar);
}