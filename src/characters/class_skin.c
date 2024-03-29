#include "g_local.h"
#include "../gamemodes/ctf.h"

char *V_GetClassSkin(edict_t *ent) {
    char *c1, *c2;
    static char out[64];

    /* az 3.4a ctf skins support */
    switch (ent->myskills.class_num) {
        case CLASS_SOLDIER:
            c1 = class1_model->string;
            c2 = class1_skin->string;
            break;
        case CLASS_POLTERGEIST:
            c1 = class2_model->string;
            c2 = class2_skin->string;
            break;
        case CLASS_VAMPIRE:
            c1 = class3_model->string;
            c2 = class3_skin->string;
            break;
        case CLASS_MAGE:
            c1 = class4_model->string;
            c2 = class4_skin->string;
            break;
        case CLASS_ENGINEER:
            c1 = class5_model->string;
            c2 = class5_skin->string;
            break;
        case CLASS_KNIGHT:
            c1 = class6_model->string;
            c2 = class6_skin->string;
            break;
        case CLASS_ALIEN:
                c1 = class11_model->string;
                c2 = class11_skin->string;
                break;
        case CLASS_CLERIC:
            c1 = class7_model->string;
                c2 = class7_skin->string;
            break;
        case CLASS_WEAPONMASTER:
            c1 = class8_model->string;
            c2 = class8_skin->string;
            break;
        case CLASS_NECROMANCER:
            c1 = class9_model->string;
                c2 = class9_skin->string;
                break;

            case CLASS_SHAMAN:
                c1 = class10_model->string;
                c2 = class10_skin->string;
                break;
        default:
            //gi.dprintf("returned male/grunt: %d %d %s\n", ent->myskills.class_num, ent->myskills.level, ent->client->pers.netname);
            return "male/grunt";
    }

    if (ctf->value || domination->value || ptr->value || tbi->value) {
        if (ent->teamnum == RED_TEAM)
            c2 = "ctf_r";
        else
            c2 = "ctf_b";
    }

    sprintf(out, "%s/%s", c1, c2);
    return out;
}

qboolean vrx_assign_character_skin(edict_t *ent, char *s) {
    int playernum = ent - g_edicts - 1;
    char *p;
    char t[64];
    char *c_skin;

    if (!enforce_class_skins->value)
        return false;

    // don't assign class skins in teamplay modes
    // az 3.4a support ctf skins.
    /*if (ctf->value || domination->value || ptr->value || tbi->value)
        return false;*/

    Com_sprintf(t, sizeof(t), "%s", s);

    if ((p = strrchr(t, '/')) != NULL)
        p[1] = 0;
    else
        strcpy(t, "male/");

    c_skin = va("%s\\%s\0", ent->client->pers.netname, V_GetClassSkin(ent));
    gi.configstring(CS_PLAYERSKINS + playernum, c_skin);

    return true;
}
