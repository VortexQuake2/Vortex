#include "g_local.h"
#include "vote.h"

void vrx_vote_show_map_menu(edict_t *ent, int pagenum, int mapmode);

struct vote_s currentVote;

struct vote_s *vrx_vote_get_current() {
    return &currentVote;
}

qboolean vrx_vote_is_in_progress() {
    return currentVote.running;
}

void vrx_vote_reset() {
    int i = 0;
    edict_t *e;
    memset(&currentVote, 0, sizeof(currentVote));

    for_each_player(e, i) {
        e->client->resp.HasVoted = false;
    }
}

enum votestate_t vrx_vote_is_done() {
    const int players = vrx_get_joined_players(false);

    if (players < 1)
        return VS_UNFINISHED;

    if (currentVote.numVotes >= 0.75 * players)
        return VS_CHANGE_NOW;

    if (currentVote.numVotes > 0.5 * players)
        return VS_CHANGE_LATER;

    return VS_UNFINISHED;
}


void vrx_votes_run() {
    if (!currentVote.running)
        return;

    if (currentVote.endframe == level.framenum + qf2sf(600))
        gi.bprintf(PRINT_CHAT, "One minute left to place your vote.\n");
    else if (currentVote.endframe == level.framenum + qf2sf(300))
        gi.bprintf(PRINT_CHAT, "Thirty seconds left to place your vote.\n");
    else if (currentVote.endframe == level.framenum + qf2sf(100))
        gi.bprintf(PRINT_CHAT, "Ten seconds left to place your vote.\n");
    else if (currentVote.endframe <= level.framenum) {
        // Finish vote
        // Did we reach a majority?
        if (vrx_vote_is_done()) {
            // Tell everyone
            G_PrintGreenText("A majority was reached! Vote passed!\n");
            currentVote.running = false;

            //Change the map
            // az note: internally, V_ChangeMap will reset the votes!
            currentVote.success(currentVote.udata);
        } else {
            gi.bprintf(PRINT_CHAT, "Vote failed.\n");
            vrx_vote_reset();

            currentVote.voter->client->resp.VoteTimeout = level.time + 20;
            currentVote.voter = NULL;
        }
    } else {
        if (vrx_vote_is_done() == VS_CHANGE_NOW) {
            // Tell everyone
            gi.sound(&g_edicts[0], CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NONE, 0);
            G_PrintGreenText("A majority was reached! Vote passed!\n");

            currentVote.success(currentVote.udata);
        }
    }
}

//************************************************************************************************

void vrx_vote_kill(edict_t *ent) {
    if (ent->client->resp.HasVoted) // voted?
    {
        ent->client->resp.HasVoted = false;
        if (ent->client->resp.voteType == 1) // voted yes?
            currentVote.numVotes--;
        else
            currentVote.numVoteNo--; // voted no..
    }
}

void _vrx_start_vote(edict_t *ent, enum votetype_t vt, void (*success)(void *), void *udata) {
    currentVote.votetype = vt;
    currentVote.voter = ent;
    currentVote.numVotes = 1;
    currentVote.numVoteNo = 0;
    ent->client->resp.HasVoted = true;
    strcpy(currentVote.ip, ent->client->pers.current_ip);
    strcpy(currentVote.name, ent->myskills.player_name);
    currentVote.endframe = level.framenum + qf2sf(900);
    currentVote.running = true;

    currentVote.success = success;
    currentVote.udata = udata;
}


void vrx_vote_yes(edict_t *ent) {
    if (!currentVote.running)
        safe_cprintf(ent, PRINT_HIGH, "There is no vote taking place at this time.\n");
    else {
        // Did we already vote?
        if (ent->client->resp.HasVoted) {
            safe_cprintf(ent, PRINT_HIGH, "You already placed your vote.\n");
            return; // GTFO PLZ
        }
        ent->client->resp.HasVoted = true;
        currentVote.numVotes++;

        G_PrintGreenText(va("%s voted Yes.\n", ent->client->pers.netname));
    }
}

void vrx_vote_no(edict_t *ent) {
    if (!currentVote.running)
        safe_cprintf(ent, PRINT_HIGH, "There is no vote taking place at this time.\n");
    else {
        // Did we already vote?
        if (ent->client->resp.HasVoted) {
            safe_cprintf(ent, PRINT_HIGH, "You already placed your vote.\n");
            return; // GTFO PLZ
        }
        currentVote.numVoteNo++;
        ent->client->resp.HasVoted = true;
        gi.bprintf(PRINT_CHAT, "%s voted No.\n", ent->client->pers.netname);
    }
    return;
}

bool vrx_can_vote(edict_t *ent) {
    if (!voting->value)
        return false;

    //Voting enabled?
    if (!voting->value)
        return false;

    // don't allow non-admin voting during pre-game to allow players time to connect
    if (!ent->myskills.administrator && (level.time < 5.0)) // allow 5 seconds for players to connect
    {
        safe_cprintf(ent, PRINT_HIGH, "Please allow time for other players to connect.\n");
        return false;
    }
    if (ent->client->resp.VoteTimeout > level.time)
        return false;

    return true;
}

void vrx_vote_cmd(edict_t *ent) {
    const char *cmd2 = gi.argv(1);
    if (!vrx_can_vote(ent))
        return;

    if (cmd2[0] == 'y' || cmd2[0] == 'Y') {
        vrx_vote_yes(ent);
        return;
    } else if (cmd2[0] == 'n' || cmd2[0] == 'N') {
        vrx_vote_no(ent);
        return;
    } else if (cmd2[0] == 'b' || cmd2[0] == 'B') {
        if (gi.argc() > 2) {
            char *end = nullptr;
            const int n = strtol(gi.argv(2), &end, 10);
            if (end == NULL || *end != '\0' || n < 0 || n > maxclients->value) {
                safe_cprintf(ent, PRINT_HIGH, "Invalid number format for bot vote.\n");
                return;
            }
            vrx_start_bot_vote(ent, n != 0, n);
        } else
            vrx_show_bot_vote_menu(ent);
        return;
    }
    // Bring open the menu
    else if (currentVote.running) {
        // TODO
        return;
    }

    vrx_vote_map(ent);
}


void vrx_vote_announce(edict_t *ent, const char *msg) {
    char hudText[1024];
    char announceText[1024];

    Com_sprintf(announceText, 1024, "%s started a vote for %s", ent->myskills.player_name, msg);
    Com_sprintf(hudText, 1024, "vote in progress: %s\n", msg);

    gi.configstring(CS_GENERAL + MAX_CLIENTS + 1, hudText);

    G_PrintGreenText(announceText);
    gi.sound(&g_edicts[0], CHAN_VOICE, gi.soundindex("misc/comp_up.wav"), 1, ATTN_NONE, 0);

    const uint64_t timeRem = (currentVote.endframe - level.framenum) / (uint64_t) sv_fps->value;
    gi.bprintf(PRINT_HIGH, "Please place your vote by typing 'vote yes' or 'vote no' within the next %u seconds.\n",
               timeRem);
}
