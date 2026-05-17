#pragma once

enum upgradehelp_paramtype_s {
    PT_DOUBLE,
    PT_FLOAT,
    PT_INT,
    PT_INT_VALUE,
    PT_FLOAT_VALUE,
    PT_DOUBLE_VALUE
};

struct upgradehelp_param_s {
    enum upgradehelp_paramtype_s type;
    const char* description;
    union {
        const double* paramdptr;
        const float* paramfptr;
        const int* paramiptr;
        const int param;
        const float paramf;
        const double paramd;
    };
};

struct upgradehelp_s {
    int abilityIndex;
    const char * descriptionLines[16];
    const char* command;
    const struct upgradehelp_param_s* params;
};

// struct classhelp_s {
//     int classIndex;
//
// };

int vrx_upgradehelp_add_menu_lines(edict_t *ent, const struct upgradehelp_s *help);
const struct upgradehelp_s* vrx_upgradehelp_get(int abilityIndex);
const struct upgradehelp_s* vrx_talenthelp_get(int abilityIndex);