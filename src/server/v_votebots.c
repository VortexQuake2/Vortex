#include "g_local.h"
#include "vote.h"
#include "ai/ai_local.h"

void vrx_apply_bot_vote() {
    auto currentVote = vrx_vote_get_current();

    if (currentVote->votetype != VT_BOTS)
        return;

    gi.cvar_set("bot_enable", currentVote->botvote.enabled ? "1" : "0");
    gi.cvar_set("bot_autospawn", va("%d", currentVote->botvote.num_bots));

    if (currentVote->botvote.enabled) {
        gi.bprintf(PRINT_HIGH, "Bot vote enabled: Next map will have %d bots", currentVote->botvote.num_bots);
    } else {
        BOT_RemoveBot("all");
    }

    vrx_vote_reset();
}

void vrx_bot_vote_success(void * p) {
    vrx_apply_bot_vote();
}

void vrx_start_bot_vote(edict_t *ent, bool enable, int num_bots) {
    if (vrx_vote_is_in_progress())
        return;

    //check for valid choice
    auto currentVote = vrx_vote_get_current();

    //Add the vote
    _vrx_start_vote(ent, VT_BOTS, vrx_bot_vote_success, nullptr);
    currentVote->botvote.enabled = enable;
    currentVote->botvote.num_bots = num_bots;

    char text[1024];
    if (currentVote->botvote.enabled) {
        Com_sprintf(text, 1024, "%d bots", currentVote->botvote.num_bots);
    } else {
        Com_sprintf(text, 1024, "no bots");
    }

    vrx_vote_announce(ent, text);
}

#define BOT_ENABLE_BIT U32BIT(23)
#define BOT_DISABLE_BIT U32BIT(22)

// octets are: numbots, enable/disable, unused,   start/cancel
//        MSB  00000000 00000000        00000000  00000000  LSB
bool _vrx_show_bot_vote_menu(edict_t* self, int currentOptions);

void bot_vote_menu_handler(edict_t * ent, int option) {
    if (option == 11) {
        menu_close(ent, false);
    }

    if ((option & 0xFF) == 10) {
        bool enable = option & BOT_ENABLE_BIT;
        int num_bots = (option >> 24) & 0xFF;

        vrx_start_bot_vote(ent, enable, num_bots);
        return;
    }

    _vrx_show_bot_vote_menu(ent, option);
}

bool _vrx_show_bot_vote_menu(edict_t* self, int currentOptions) {
    int enable = currentOptions & BOT_ENABLE_BIT;
    int num_bots = (currentOptions >> 24) & 0xFF;
    int lastline = 2;

    if (!menu_can_show(self))
        return true;

    menu_clear(self);

    // header
    menu_add_line(self, "Vote for bot settings:", MENU_GREEN_CENTERED);
    menu_add_line(self, " ", 0);

    int new_bot_option = (num_bots << 24);
    if (enable) {
        new_bot_option &= ~BOT_ENABLE_BIT;
        new_bot_option |= BOT_DISABLE_BIT;
        lastline = menu_add_line(self, "bots enabled", new_bot_option);

        menu_add_line(self, "", 0);
        menu_add_line(self, va("%d bots", num_bots), MENU_GREEN_LEFT);

        if (num_bots < min(16, maxclients->value)) {
            int incbots_option = (currentOptions & 0x00FFFFFF) | ((num_bots + 1) << 24);
            menu_add_line(self, va("increase bots", incbots_option), incbots_option);
        }

        if (num_bots > 0) {
            int decbots_option = (currentOptions & 0x00FFFFFF) | ((num_bots - 1) << 24);
            menu_add_line(self, va("decrease bots", decbots_option), decbots_option);
        }

    } else {
        new_bot_option &= ~BOT_DISABLE_BIT;
        new_bot_option |= BOT_ENABLE_BIT;
        lastline = menu_add_line(self, "bots disabled", new_bot_option);
    }

    menu_add_line(self, "", 0);
    menu_add_line(self, "start vote", 10 + currentOptions);
    menu_add_line(self, "cancel", 11);

    menu_set_handler(self, bot_vote_menu_handler);

    self->client->menustorage.currentline = lastline;

    menu_show(self);
    return true;
}

void vrx_show_bot_vote_menu(edict_t* self) {
    int startOptions = 0;
    if (bot_enable->value)
        startOptions |= BOT_ENABLE_BIT;
    else
        startOptions |= BOT_DISABLE_BIT;

    if (bot_autospawn->value) {
        int cnt = min(bot_autospawn->value, 16);
        startOptions |= cnt << 24;
    }

    _vrx_show_bot_vote_menu(self, startOptions);
}

void vrx_vote_bots(edict_t *ent) {
    if (!vrx_can_vote(ent))
        return;

    vrx_show_bot_vote_menu(ent);
}
