#include "g_local.h"

/* newvrx newbie tips. */

const char *messages[] = {
//   decino: max 40 chars
//   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n
    "Open the Vortex menu with the following \nkey: 'TAB', or use the 'inven' command. \n",
    "You can set your respawn weapon with the\nfollowing command: 'vrxrespawn'         \n",
    "Damage and kill as many things possible,\nyou will gain EXP fast and level up!    \n",
    "Use your credits at the armory to buy   \nstuff with the command: 'vrxarmory'     \n",
    "Want to change name or class?           \nChange your name and reconnect.         \n",
    "Leveled up? Spend your points to become \nstronger and unique!                    \n",
    "Weapon points will upgrade your weapons,\nresulting more damage and other effects.\n",
    "You can teleport away during combat with\nthe command: 'use tball self'           \n",
    "Make sure you've set a password, use    \n'set vrx_password <your_password> u'    \n",
    "To vote for modes and maps use the      \nfollowing command: 'vote'               \n",
    "You earn talent points every 2 levels,  \nuse those to become even more unique!   \n",
    "Seen those colored discs? Those are     \ncalled runes and enhance your skills    \nwithout the need of upgrading it! \n",
    "To get a full list of commands, use     \n'vrxhelp'                               \n"
};

static const size_t msg_count = sizeof(messages) / sizeof(char*);

void vrx_print_newbie_tip(edict_t *ent) {
    if (ent->myskills.level < 2) {
        const char* msg = messages[GetRandom(0, msg_count - 1)];
        gi.centerprintf(ent, msg);
    }
}