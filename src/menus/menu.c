#include "g_local.h"

// v3.12
qboolean menu_active(const edict_t *ent, int index, void (*optionselected)(edict_t *ent, int option)) {
    // don't need to be here if there's no menu open!
    if (!ent->client->menustorage.menu_active)
        return false;
    // check for index, only necessary if 2 funcs are exactly the same
    if (index && (ent->client->menustorage.menu_index != index))
        return false;
    // check if we're using the right handler
    return (ent->client->menustorage.optionselected == optionselected);
}

int menu_add_line(const edict_t *ent, const char *line, int option) {
    if (ent->client->menustorage.menu_active) // checks to see if the menu is showing
        return 0;
    // Menu lines are 1-based in storage; index 0 is unused.
    if (ent->client->menustorage.num_of_lines >= MAX_LINES - 1) // checks to see if there is space
        return MAX_LINES;

    ent->client->menustorage.num_of_lines++; // adds to the number of lines that can be seen

    const size_t size = strlen(line) + 1;
    ent->client->menustorage.messages[ent->client->menustorage.num_of_lines].msg = vrx_malloc(size, TAG_GAME);
    strcpy(ent->client->menustorage.messages[ent->client->menustorage.num_of_lines].msg, line);
    ent->client->menustorage.messages[ent->client->menustorage.num_of_lines].option = option;

    return ent->client->menustorage.num_of_lines;
}

void menu_clear(const edict_t *ent) {
    int i = 0;

    if (ent->client->menustorage.menu_active) // checks to see if the menu is showing
        return;

    for (i = 0; i < MAX_LINES; i++) {
        ent->client->menustorage.messages[i].option = 0;
        if (ent->client->menustorage.messages[i].msg != nullptr) {
            vrx_free(ent->client->menustorage.messages[i].msg);
            //GHz START
            ent->client->menustorage.messages[i].msg = nullptr;
            //GHz END
        }
    }
    //GHz START
    // keep record of last known menu for multi-purpose menus that have more than one handler
    if (ent->client->menustorage.optionselected)
        ent->client->menustorage.oldmenuhandler = ent->client->menustorage.optionselected;
    if (ent->client->menustorage.currentline)
        ent->client->menustorage.oldline = ent->client->menustorage.currentline;
    //GHz END
    ent->client->menustorage.optionselected = nullptr;
    // az begin
    ent->client->menustorage.onclose = nullptr;
    ent->client->menustorage.cancel_close_event = false;
    // az end
    ent->client->menustorage.currentline = 0;
    ent->client->menustorage.num_of_lines = 0;
    ent->client->menustorage.menu_index = 0; // 3.45
    ent->client->menustorage.usercontext = nullptr;
}

void tradeconfirmation_handler(edict_t *ent, int option);

void itemmenu_handler(edict_t *ent, int option);

void menu_set_handler(const edict_t *ent, void (*optionselected)(edict_t *ent, int option)) {
    ent->client->menustorage.optionselected = optionselected;
}

void menu_set_close_handler(const edict_t *ent, void (*onclose)(edict_t *ent)) {
    ent->client->menustorage.onclose = onclose;
}

//GHz START
int menu_top(const edict_t *ent) {
    int i;

    for (i = 0; i < MAX_LINES; i++) {
        int option = ent->client->menustorage.messages[i].option;
        if (option != 0 && option != MENU_GREEN_CENTERED && option != MENU_WHITE_CENTERED && option != MENU_GREEN_LEFT)
            break;
    }
    return i;
}

int menu_bottom(const edict_t *ent) {
    int i;

    for (i = MAX_LINES - 1; i > 0; i--) {
        int option = ent->client->menustorage.messages[i].option;
        if (option != 0 && option != MENU_GREEN_CENTERED && option != MENU_WHITE_CENTERED && option != MENU_GREEN_LEFT)
            break;
    }
    return i;
}

//GHz END

void menu_down(edict_t *ent) {
    int option;

    // 3.65 avoid menu overflows
    if (ent->client->menu_delay > level.time)
        return;
    ent->client->menu_delay = level.time + MENU_DELAY_TIME;

    do {
        if (ent->client->menustorage.currentline < ent->client->menustorage.num_of_lines)
            ent->client->menustorage.currentline++;
        else
            ent->client->menustorage.currentline = menu_top(ent);
        option = ent->client->menustorage.messages[ent->client->menustorage.currentline].option;
    } while (option == 0 || option == MENU_WHITE_CENTERED || option == MENU_GREEN_CENTERED
             || option == MENU_GREEN_RIGHT || option == MENU_GREEN_LEFT);
    menu_show(ent);
}

void menu_up(edict_t *ent) {
    int option;

    // 3.65 avoid menu overflows
    if (ent->client->menu_delay > level.time)
        return;
    ent->client->menu_delay = level.time + MENU_DELAY_TIME;

    do {
        if (ent->client->menustorage.currentline > menu_top(ent))
            ent->client->menustorage.currentline--;
        else
            ent->client->menustorage.currentline = menu_bottom(ent);
        option = ent->client->menustorage.messages[ent->client->menustorage.currentline].option;
    } while (option == 0 || option == MENU_WHITE_CENTERED || option == MENU_GREEN_CENTERED
             || option == MENU_GREEN_RIGHT || option == MENU_GREEN_LEFT);
    menu_show(ent);
}

/*
=============
menu_select
calls the menu handler with the currently selected option
=============
*/
void menu_select(edict_t *ent) {
    //GHz START
    if (debuginfo->value)
        gi.dprintf("DEBUG: menu_select()\n");
    //GHz END

    const int i = ent->client->menustorage.messages[ent->client->menustorage.currentline].option;

    // close the menu as a selection has been made.
    // also cache "cancel" so it doesn't get ignored.
    menu_close(ent, false);

    // call the menu handler with the current option value
    ent->client->menustorage.optionselected(ent, i);

    if (ent->client->menustorage.onclose && !ent->client->menustorage.cancel_close_event)
        ent->client->menustorage.onclose(ent);

    /* consume the cancel close event flag */
    if (ent->client->menustorage.cancel_close_event)
        ent->client->menustorage.cancel_close_event = false;
}

/*
=============
menu_init
clears all menus for this client
=============
*/
void menu_init(const edict_t *ent) {
    for (int i = 0; i < MAX_LINES; i++) {
        ent->client->menustorage.messages[i].msg = nullptr;
        ent->client->menustorage.messages[i].option = 0;
    }


    ent->client->menustorage.menu_active = false;
    ent->client->menustorage.displaymsg = false;
    ent->client->menustorage.optionselected = nullptr;
    ent->client->menustorage.currentline = 0;
    ent->client->menustorage.num_of_lines = 0;
    ent->client->menustorage.menu_index = 0; // 3.45
    ent->client->menustorage.onclose = nullptr;
    ent->client->menustorage.cancel_close_event = false;
}

/*
=============
menu_can_show
used every frame to display a player's menu
=============
*/
void menu_show(edict_t *ent) {
    int j; // general purpose integer for temporary use :)
    char finalmenu[1024]; // where the control strings for the menu are assembled.
    char tmp[80]; // temporary storage strings
    int center;

    if (debuginfo->value)
        gi.dprintf("DEBUG: menu_show()\n");
    if ((ent->client->menustorage.num_of_lines < 1) || (ent->client->menustorage.num_of_lines > (MAX_LINES - 1))) {
        gi.dprintf("WARNING: menu_show() called with %d lines\n", ent->client->menustorage.num_of_lines);
        return;
    }

    // copy menu bg control strings to our final menu
#ifndef VRX_REPRO
    sprintf(finalmenu, "xv 32 yv 8 picn inventory ");
#else
    // force monospace font
    sprintf(finalmenu, "mono xv 32 yv 8 picn inventory ");
#endif
    // get y coord of text based on the number of lines we want to create
    // this keeps the text vertically centered on our screen
    j = 24 + LINE_SPACING * (ceilf((float) (20 - ent->client->menustorage.num_of_lines) / 2));
    // cycle through all lines and add their control codes to our final menu
    // nothing is actually displayed until the very end
    for (int i = 1; i < (ent->client->menustorage.num_of_lines + 1); i++) {
        // get x coord of screen based on the length of the string for
        // text that should be centered
        center = 216 / 2 - strlen(ent->client->menustorage.messages[i].msg) * 4 + 52;
        if (ent->client->menustorage.messages[i].option == 0) // print white text
        {
            Com_sprintf(tmp, sizeof(tmp), "xv 52 yv %i string \"%s\" ", j, ent->client->menustorage.messages[i].msg);
        } else if (ent->client->menustorage.messages[i].option == MENU_GREEN_LEFT) // print green text
        {
            Com_sprintf(tmp, sizeof(tmp), "xv 52 yv %i string2 \"%s\" ", j, ent->client->menustorage.messages[i].msg);
        } else if (ent->client->menustorage.messages[i].option == MENU_GREEN_CENTERED) // print centered green text
        {
            Com_sprintf(tmp, sizeof(tmp), "xv %d yv %i string2 \"%s\" ", center, j,
                        ent->client->menustorage.messages[i].msg);
        } else if (ent->client->menustorage.messages[i].option == MENU_WHITE_CENTERED) // print centered white text
        {
            Com_sprintf(tmp, sizeof(tmp), "xv %d yv %i string \"%s\" ", center, j,
                        ent->client->menustorage.messages[i].msg);
        } else if (ent->client->menustorage.messages[i].option == MENU_GREEN_RIGHT) // print right-aligned green text
        {
            center = 216 - strlen(ent->client->menustorage.messages[i].msg) * 8 + 52;
            Com_sprintf(tmp, sizeof(tmp), "xv %d yv %i string2 \"%s\" ", center, j,
                        ent->client->menustorage.messages[i].msg);
        } else if (i == ent->client->menustorage.currentline) {
            Com_sprintf(tmp, sizeof(tmp), "xv 52 yv %i string2 \">> %s\" ", j,
                        ent->client->menustorage.messages[i].msg);
        } else
            Com_sprintf(tmp, sizeof(tmp), "xv 52 yv %i string \"   %s\" ", j, ent->client->menustorage.messages[i].msg);
        // add the control string to our final menu space
        strncat(finalmenu, tmp, sizeof(finalmenu) - strlen(finalmenu) - 1);
        j += LINE_SPACING;
    }

    // actually display the final menu
    ent->client->menustorage.menu_active = true;
    ent->client->menustorage.displaymsg = false;
    ent->client->showinventory = false;
    ent->client->showscores = true;
    gi.WriteByte(svc_layout);
    gi.WriteString(finalmenu);
    gi.unicast(ent, true);

    //gi.dprintf("menu_show() at line %d\n", ent->client->menustorage.currentline);
}

void menu_close(edict_t *ent, qboolean do_close_event) {
    if (debuginfo->value)
        gi.dprintf("DEBUG: menu_close()\n");

    menu_clear(ent); // reset all menu variables and strings
    ent->client->showscores = false;
    ent->client->menustorage.menu_active = false;
    ent->client->menustorage.displaymsg = false;
    ent->client->showinventory = false;
    ent->client->trading = false; // done trading
    ent->client->layout.dirty = true; // update the layout asap

    if (ent->client->menustorage.onclose && do_close_event)
        ent->client->menustorage.onclose(ent);

    // maybe the onclose needs the chance to free the usercontext
    ent->client->menustorage.usercontext = nullptr;
}

/*
=============
menu_close_all
cycles thru all clients and resets their menus
=============
*/
void menu_close_all(void) {
    for (int i = 0; i < game.maxclients; i++) {
        edict_t *ent = g_edicts + 1 + i;
        menu_close(ent, true);
    }
}

/*
=============
menu_can_show
returns false if the client has another menu open
=============
*/
qboolean menu_can_show(const edict_t *ent) {
    if (ent->client->showscores || ent->client->showinventory
        || ent->client->menustorage.menu_active || ent->client->pers.scanner_active)
        return false;
    return true;
}
