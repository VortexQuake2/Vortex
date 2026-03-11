// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include <stddef.h>

#include "cg_local.h"
#include "quake2/q_recompat.h"

#if defined(CHAR_WIDTH)
#undef CHAR_WIDTH
#endif

constexpr int32_t STAT_MINUS      = 10;  // num frame for '-' stats digit
const char *sb_nums[2][11] =
{
    {   "num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
        "num_6", "num_7", "num_8", "num_9", "num_minus"
    },
    {   "anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
        "anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"
    }
};

constexpr int32_t CHAR_WIDTH    = 16;
constexpr int32_t CONCHAR_WIDTH = 8;

static int32_t font_y_offset;

constexpr rgba_t alt_color = { .r=112, 255, 52, 255 };

static cvar_t *scr_usekfont;

static cvar_t *scr_centertime;
static cvar_t *scr_printspeed;
static cvar_t *cl_notifytime;
static cvar_t *scr_maxlines;
static cvar_t *ui_acc_contrast;
static cvar_t* ui_acc_alttypeface;

// static temp data used for hud
static struct
{
    struct {
        struct {
            char    text[24];
        } table_cells[6];
    } table_rows[11]; // just enough to store 8 levels + header + total (+ one slack)

    size_t column_widths[6];
    int32_t num_rows;
    int32_t num_columns;
} hud_temp = {0};

// max number of centerprints in the rotating buffer
constexpr size_t MAX_CENTER_PRINTS = 4;
constexpr size_t MAX_BIND_STR_LEN = 256;

struct cl_bind_t {
    char bind[MAX_BIND_STR_LEN];
    char purpose[MAX_BIND_STR_LEN];
};

#define MAX_CENTERPRINT_BINDS 16
#define MAX_CENTERPRINT_LINES 8

struct cl_centerprint_t {
    struct cl_bind_t binds[MAX_CENTERPRINT_BINDS]; // binds
    size_t bind_count; // current number of binds

    char  lines[MAX_CENTERPRINT_LINES][MAX_BIND_STR_LEN];
    size_t      row_count; // number of centerprint lines

    bool        instant; // don't type out

    size_t      current_line; // current line we're typing out
    size_t      line_count; // byte count to draw on current line
    bool        finished; // done typing it out
    uint64_t    time_tick, time_off; // time to remove at
};

void centerprint_clear_lines(struct cl_centerprint_t* self) {
    memset(self->lines, 0, sizeof(self->lines));
    self->row_count = 0;
}

void centerprint_add_line(struct cl_centerprint_t* self, const char* line) {
    if (!line)
        return;
    if (self->row_count >= MAX_CENTERPRINT_LINES)
        return;

    size_t len = strlen(line);
    if (len + 1 >= MAX_BIND_STR_LEN)
        return;

    memmove(&self->lines[self->row_count], line, len);
    self->lines[self->row_count][len] = '\0';
    self->row_count++;
}

void centerprint_add_bind(struct cl_centerprint_t* self, const struct cl_bind_t bind) {
    if (self->bind_count >= MAX_CENTERPRINT_BINDS)
        return;

    memcpy(&self->binds[self->bind_count], &bind, sizeof(struct cl_bind_t));
    self->bind_count++;
}

struct cl_bind_t bind_from_string(const char* bind_str, const char* purpose_str) {
    struct cl_bind_t bind;
    strncpy(bind.bind, bind_str, sizeof bind.bind);
    strncpy(bind.purpose, purpose_str, sizeof bind.purpose);
    bind.bind[sizeof bind.bind - 1] = '\0';
    bind.purpose[sizeof bind.purpose - 1] = '\0';
    return bind;
}

int32_t ps_instant_dmg_value(const player_state_t *ps) {
    return (int32_t) (((uint32_t)(uint16_t)ps->stats[STAT_ID_DAMAGE]) | (((uint32_t)(uint16_t)ps->stats[STAT_ID_DAMAGE2]) << 16));
}

int32_t ps_score_value(const player_state_t * ps) {
    return (int32_t) ((uint32_t)(uint16_t)ps->stats[STAT_SCORE] | (uint32_t)(uint16_t)ps->stats[STAT_SCORE2] << 16);
}

bool CG_ViewingLayout(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & (LAYOUTS_LAYOUT | LAYOUTS_INVENTORY);
}

bool CG_InIntermission(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & LAYOUTS_INTERMISSION;
}

inline bool CG_HudHidden(const player_state_t *ps)
{
    return ps->stats[STAT_LAYOUTS] & LAYOUTS_HIDE_HUD;
}

enum layout_flags_t CG_LayoutFlags(const player_state_t *ps)
{
    return (enum layout_flags_t) ps->stats[STAT_LAYOUTS];
}

constexpr size_t MAX_NOTIFY = 8;
constexpr size_t MAX_MESSAGE_LEN = 2048;

struct cl_notify_t {
    char            message[MAX_MESSAGE_LEN]; // utf8 message
    bool            is_active; // filled or not
    bool            is_chat; // green or not
    uint64_t        time; // rotate us when < CL_Time()
};

// per-splitscreen client hud storage
struct hud_data_t {
    struct cl_centerprint_t centers[MAX_CENTER_PRINTS]; // list of centers
    int32_t center_index; // current index we're drawing, or -1 if none left
    struct cl_notify_t notify[MAX_NOTIFY]; // list of notifies

    int32_t dmg_counter;
    int32_t dmg_instant;
    uint64_t last_dmg_time;
};


static struct hud_data_t hud_data[MAX_SPLIT_PLAYERS];

void CG_ClearCenterprint(const int32_t isplit)
{
    hud_data[isplit].center_index = -1;
}

void CG_ClearNotify(const int32_t isplit)
{
    for (size_t i = 0; i < MAX_NOTIFY; i++)
        hud_data[isplit].notify[i].is_active = false;
}

// if the top one is expired, cycle the ones ahead backwards (since
// the times are always increasing)
static void CG_Notify_CheckExpire(struct hud_data_t *data)
{
    while (data->notify[0].is_active && data->notify[0].time < cgi.CL_ClientTime())
    {
        data->notify[0].is_active = false;

        for (size_t i = 1; i < MAX_NOTIFY; i++)
            if (data->notify[i].is_active) {
                struct cl_notify_t cur = data->notify[i];
                data->notify[i] = data->notify[i - 1];
                data->notify[i - 1] = cur;
            }
    }
}

// add notify to list
static void CG_AddNotify(struct hud_data_t *data, const char *msg, const bool is_chat)
{
    size_t i = 0;

    if (scr_maxlines->integer <= 0)
        return;

    const int max = min(MAX_NOTIFY, (size_t)scr_maxlines->integer);

    for (; i < max; i++)
        if (!data->notify[i].is_active)
            break;

    // none left, so expire the topmost one
    if (i == max)
    {
        data->notify[0].time = 0;
        CG_Notify_CheckExpire(data);
        i = max - 1;
    }

    size_t len = min(strlen(msg), MAX_MESSAGE_LEN - 1);

    memmove(data->notify[i].message, msg, len);
    data->notify[i].message[len] = '\0';

    data->notify[i].is_active = true;
    data->notify[i].is_chat = is_chat;
    data->notify[i].time = cgi.CL_ClientTime() + cl_notifytime->value * 1000;
}

// draw notifies
static void CG_DrawNotify(const int32_t isplit, const struct vrect_t hud_vrect, const struct vrect_t hud_safe, const int32_t scale)
{
    const auto data = &hud_data[isplit];

    CG_Notify_CheckExpire(data);

    int y = hud_vrect.y * scale + hud_safe.y;

    cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

    if (ui_acc_contrast->integer)
    {
        for (size_t i = 0; i < MAX_NOTIFY; i++)
        {
            const auto msg = &data->notify[i];
            const auto len = strlen(msg->message);
            if (!msg->is_active || len == 0)
                break;

            vec2_t sz = cgi.SCR_MeasureFontString(msg->message, scale);
            sz.x += 10; // extra padding for black bars
            cgi.SCR_DrawColorPic(hud_vrect.x * scale + hud_safe.x - 5, y, sz.x, 15 * scale, "_white", &rgba_black);
            y += 10 * scale;
        }
    }

    y = hud_vrect.y * scale + hud_safe.y;
    for (size_t i = 0; i < MAX_NOTIFY; i++)
    {
        const auto msg = data->notify[i];
        if (!msg.is_active)
            break;

        cgi.SCR_DrawFontString(msg.message, hud_vrect.x * scale + hud_safe.x, y, scale, msg.is_chat ? &alt_color : &rgba_white, true, LEFT);
        y += 10 * scale;
    }

    cgi.SCR_SetAltTypeface(false);

    // draw text input (only the main player can really chat anyways...)
    if (isplit == 0)
    {
        const char *input_msg;
        bool input_team;

        if (cgi.CL_GetTextInput(&input_msg, &input_team))
            cgi.SCR_DrawFontString(
                va("%s: %s", input_team ? "say_team" : "say", input_msg),
                hud_vrect.x * scale + hud_safe.x,
                y,
                scale,
                &rgba_white,
                true,
                LEFT
            );
    }
}

/*
==============
CG_DrawHUDString
==============
*/
static int CG_DrawHUDString (const char *string, int x, int y, const int centerwidth, const int _xor, const int scale, const bool shadow /*= true*/)
{
    char    line[1024];

    const int margin = x;

    while (*string)
    {
        // scan out one line of text from the string
        int width = 0;
        while (*string && *string != '\n')
            line[width++] = *string++;
        line[width] = 0;

        vec2_t size;

        if (scr_usekfont->integer)
            size = cgi.SCR_MeasureFontString(line, scale);

        if (centerwidth)
        {
            if (!scr_usekfont->integer)
                x = margin + (centerwidth - width*CONCHAR_WIDTH*scale)/2;
            else
                x = margin + (centerwidth - size.x)/2;
        }
        else
            x = margin;

        if (!scr_usekfont->integer)
        {
            for (int i = 0 ; i<width ; i++)
            {
                cgi.SCR_DrawChar (x, y, scale, line[i]^_xor, shadow);
                x += CONCHAR_WIDTH * scale;
            }
        }
        else
        {
            cgi.SCR_DrawFontString(line, x, y - font_y_offset * scale, scale, _xor ? &alt_color : &rgba_white, true, LEFT);
            x += size.x;
        }

        if (*string)
        {
            string++;   // skip the \n
            x = margin;
            if (!scr_usekfont->integer)
                y += CONCHAR_WIDTH * scale;
            else
                // TODO
                y += 10 * scale;//size.y;
        }
    }

    return x;
}

// Shamefully stolen from Kex
size_t FindStartOfUTF8Codepoint(const char* str, const size_t pos)
{
    if(pos >= strlen(str))
    {
        return SIZE_MAX;
    }

    for(ptrdiff_t i = pos; i >= 0; i--)
    {
        const char ch = str[i];

        if((ch & 0x80) == 0)
        {
            // character is one byte
            return i;
        }
        if((ch & 0xC0) == 0x80)
        {
            // character is part of a multi-byte sequence, keep going
            continue;
        }
        // character is the start of a multi-byte sequence, so stop now
        return i;
    }

    return SIZE_MAX;
}

size_t FindEndOfUTF8Codepoint(const char* str, const size_t pos)
{
    if(pos >= strlen(str))
    {
        return SIZE_MAX;
    }

    for(size_t i = pos; i < strlen(str); i++)
    {
        const char ch = str[i];

        if((ch & 0x80) == 0)
        {
            // character is one byte
            return i;
        }

        if((ch & 0xC0) == 0x80)
        {
            // character is part of a multi-byte sequence, keep going
            continue;
        }

        // character is the start of a multi-byte sequence, so stop now
        return i;
    }

    return SIZE_MAX;
}

void CG_NotifyMessage(const int32_t isplit, const char *msg, const bool is_chat)
{
    CG_AddNotify(&hud_data[isplit], msg, is_chat);
}

// centerprint stuff
static struct cl_centerprint_t *CG_QueueCenterPrint(const int isplit, const bool instant)
{
    auto *icl = &hud_data[isplit];

    // just use first index
    if (icl->center_index == -1 || instant)
    {
        icl->center_index = 0;


        for (size_t i = 1; i < MAX_CENTER_PRINTS; i++)
            centerprint_clear_lines(&icl->centers[i]);

        return &icl->centers[0];
    }

    // pick the next free index if we can find one
    for (size_t i = 1; i < MAX_CENTER_PRINTS; i++)
    {
        const auto center = &icl->centers[(icl->center_index + i) % MAX_CENTER_PRINTS];

        if (center->row_count == 0)
            return center;
    }

    // none, so update the current one (the new end of buffer)
    // and skip ahead
    const auto center = &icl->centers[icl->center_index];
    icl->center_index = (icl->center_index + 1) % MAX_CENTER_PRINTS;
    return center;
}

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_ParseCenterPrint (const char *str, const int isplit, const bool instant) // [Sam-KEX] Made 1st param const
{
    char    line[64];
    int     i, l;

    // handle center queueing
    struct cl_centerprint_t *center = CG_QueueCenterPrint(isplit, instant);

    centerprint_clear_lines(center);

    // split the string into lines
    size_t line_start = 0;

    const size_t slen = strlen(str);
    const char string[slen + 1];
    strncpy(string, str, slen + 1);

    center->bind_count = 0;

    // [Paril-KEX] pull out bindings. they'll always be at the start
    const char* s = string;
    while (strncmp(s, "%bind:", 6) == 0)
    {
        const char* end_of_bind = strchr(s, '%');

        if (end_of_bind == nullptr)
            break;

        const char* bind = s + 6;

        const char* purpose_ptr = strchr(bind, ':');
        size_t bind_length = purpose_ptr ? purpose_ptr - bind : strlen(bind);
        size_t purpose_length = purpose_ptr ? end_of_bind - purpose_ptr : 0;

        char bind_string[bind_length + 1];
        char purpose_string[purpose_length + 1];
        strncpy(bind_string, bind, bind_length);
        strncpy(purpose_string, purpose_ptr + 1, purpose_length);
        bind_string[bind_length] = '\0';
        purpose_string[purpose_length] = '\0';

        if (purpose_ptr != nullptr) {
            centerprint_add_bind(center, bind_from_string(bind_string, purpose_string));
        } else
            centerprint_add_bind(center, bind_from_string(bind_string, ""));

        s = end_of_bind + 1;
        center->bind_count++;
    }

    const char* s_start = s;
    // echo it to the console
    cgi.Com_Print("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

    // const char *s = string.c_str();
    do
    {
        // scan the width of the line
        for (l=0 ; l<40 ; l++)
            if (s[l] == '\n' || !s[l])
                break;
        for (i=0 ; i<(40-l)/2 ; i++)
            line[i] = ' ';

        for (int j = 0 ; j<l ; j++)
        {
            line[i++] = s[j];
        }

        line[i] = '\n';
        line[i+1] = 0;

        cgi.Com_Print(line);

        while (*s && *s != '\n')
            s++;

        if (!*s)
            break;
        s++;        // skip the \n
    } while (1);
    cgi.Com_Print("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
    CG_ClearNotify (isplit);

    s = s_start;
    for (size_t line_end = 0; ; )
    {
        line_end = FindEndOfUTF8Codepoint(s, line_end);

        if (line_end == SIZE_MAX)
        {
            // final line
            if (line_start < slen)
                centerprint_add_line(center, s + line_start);
            break;
        }

        // char part of current line;
        // if newline, end line and cut off
        const char ch = s[line_end];

        if (ch == '\n')
        {
            if (line_end > line_start) {
                char _s[line_end - line_start + 1];
                strncpy(_s, s + line_start, line_end - line_start);
                _s[line_end - line_start] = '\0';
                centerprint_add_line(center, _s);
            }
            else
                centerprint_add_line(center, "");
            line_start = line_end + 1;
            line_end++;
            continue;
        }

         line_end++;
    }

    if (center->row_count == 0)
    {
        center->finished = true;
        return;
    }

    center->time_tick = cgi.CL_ClientRealTime() + scr_printspeed->value * 1000;
    center->instant = instant;
    center->finished = false;
    center->current_line = 0;
    center->line_count = 0;
}


static void CG_DrawCenterString( const player_state_t *ps, const struct vrect_t hud_vrect, const struct vrect_t hud_safe, const int isplit, const int scale, struct cl_centerprint_t *center)
{
    int32_t y = hud_vrect.y * scale;

    if (CG_ViewingLayout(ps))
        y += hud_safe.y;
    else if (center->row_count <= 4)
        y += hud_vrect.height * 0.2f * scale;
    else
        y += 48 * scale;

    int lineHeight = (scr_usekfont->integer ? 10 : 8) * scale;
    if (ui_acc_alttypeface->integer) lineHeight *= 1.5f;

    // easy!
    if (center->instant)
    {
        for (size_t i = 0; i < center->row_count; i++)
        {
            const auto *line = center->lines[i];

            cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

            if (ui_acc_contrast->integer && strlen(line))
            {
                vec2_t sz = cgi.SCR_MeasureFontString(line, scale);
                sz.x += 10; // extra padding for black bars
                const int barY = ui_acc_alttypeface->integer ? y - 8 : y;
                cgi.SCR_DrawColorPic((hud_vrect.x + hud_vrect.width / 2) * scale - sz.x / 2, barY, sz.x, lineHeight, "_white", &rgba_black);
            }
            CG_DrawHUDString(line, (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, 320 / 2 * 2 * scale, 0, scale, true);

            cgi.SCR_SetAltTypeface(false);

            y += lineHeight;
        }

        for (size_t i = 0; i < center->bind_count; i++)
        {
            const auto *bind = &center->binds[i];
            y += lineHeight * 2;
            cgi.SCR_DrawBind(isplit, bind->bind, bind->purpose, (hud_vrect.x + hud_vrect.width / 2) * scale, y, scale);
        }

        if (!center->finished)
        {
            center->finished = true;
            center->time_off = cgi.CL_ClientRealTime() + scr_centertime->value * 1000;
        }

        return;
    }

    // hard and annoying!
    // check if it's time to fetch a new char
    const uint64_t t = cgi.CL_ClientRealTime();

    if (!center->finished)
    {
        if (center->time_tick < t) {

            center->time_tick = t + scr_printspeed->value * 1000;
            center->line_count = FindEndOfUTF8Codepoint(center->lines[center->current_line], center->line_count + 1);

            if (center->line_count == SIZE_MAX) {
                center->current_line++;
                center->line_count = 0;

                if (center->current_line == center->row_count) {
                    center->current_line--;
                    center->finished = true;
                    center->time_off = t + scr_centertime->value * 1000;
                }
            }
        }
    }

    // smallish byte buffer for single line of data...
    char buffer[256];

    for (size_t i = 0; i < center->row_count; i++)
    {
        cgi.SCR_SetAltTypeface(ui_acc_alttypeface->integer && true);

        auto line = center->lines[i];
        auto len = strlen(line);

        buffer[0] = 0;

        if (center->finished || i != center->current_line)
            Q_strlcpy(buffer, line, sizeof(buffer));
        else
            Q_strlcpy(buffer, line, min(center->line_count + 1, sizeof(buffer)));

        int blinky_x;

        if (ui_acc_contrast->integer && len)
        {
            vec2_t sz = cgi.SCR_MeasureFontString(line, scale);
            sz.x += 10; // extra padding for black bars
            int barY = ui_acc_alttypeface->integer ? y - 8 : y;
            cgi.SCR_DrawColorPic((hud_vrect.x + hud_vrect.width / 2) * scale - sz.x / 2, barY, sz.x, lineHeight, "_white", &rgba_black);
        }

        if (buffer[0])
            blinky_x = CG_DrawHUDString(buffer, (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, 320 / 2 * 2 * scale, 0, scale, true);
        else
            blinky_x = hud_vrect.width / 2 * scale;

        cgi.SCR_SetAltTypeface(false);

        if (i == center->current_line && !ui_acc_alttypeface->integer)
            cgi.SCR_DrawChar(blinky_x, y, scale, 10 + (cgi.CL_ClientRealTime() >> 8 & 1), true);

        y += lineHeight;

        if (i == center->current_line)
            break;
    }
}

static void CG_CheckDrawCenterString( const player_state_t *ps, const struct vrect_t hud_vrect, const struct vrect_t hud_safe, const int isplit, const int scale )
{
    if (CG_InIntermission(ps))
        return;
    if (hud_data[isplit].center_index == -1)
        return;

    const auto data = &hud_data[isplit];
    const auto center = &data->centers[data->center_index];

    // ran out of center time
    if (center->finished && center->time_off < cgi.CL_ClientRealTime())
    {
        centerprint_clear_lines(center);

        const size_t next_index = (data->center_index + 1) % MAX_CENTER_PRINTS;
        const auto next_center = &data->centers[next_index];

        // no more
        if (next_center->row_count == 0)
        {
            data->center_index = -1;
            return;
        }

        // buffer rotated; start timer now
        data->center_index = next_index;
        next_center->current_line = next_center->line_count = 0;
    }

    if (data->center_index == -1)
        return;

    CG_DrawCenterString( ps, hud_vrect, hud_safe, isplit, scale, &data->centers[data->center_index] );
}

/*
==============
CG_DrawString
==============
*/
static void CG_DrawString (int x, const int y, const int scale, const char *s, const bool alt /* false */, const bool shadow /*= true*/)
{
    while (*s)
    {
        cgi.SCR_DrawChar (x, y, scale, *s ^ (alt ? 0x80 : 0), shadow);
        x+=8*scale;
        s++;
    }
}

/*
==============
CG_DrawField
==============
*/
static void CG_DrawField(int x, const int y, const int color, int width, int value, int scale) {
    char num[16];
    int frame;

    if (width < 1)
        return;

    // draw number string
    // az: from 5
    if (width > 7)
        width = 7;

    int l = snprintf(num, sizeof(num), "%d", value);
    if (l >= sizeof(num))
        l = sizeof(num) - 1;

    if (l > width)
        l = width;

    x += (2 + CHAR_WIDTH * (width - l)) * scale;

    char *ptr = num;
    while (*ptr && l) {
        if (*ptr == '-')
            frame = STAT_MINUS;
        else
            frame = *ptr - '0';
        int w, h;
        cgi.Draw_GetPicSize(&w, &h, sb_nums[color][frame]);
        cgi.SCR_DrawPic(x, y, w * scale, h * scale, sb_nums[color][frame]);
        x += CHAR_WIDTH * scale;
        ptr++;
        l--;
    }
}

// [Paril-KEX]
static void CG_DrawTable(int x, int y, const uint32_t width, const uint32_t height, const int32_t scale)
{
    // half left
    int32_t width_pixels = width;
    x -= width_pixels / 2;
    y += CONCHAR_WIDTH * scale;
    // use Y as top though

    int32_t height_pixels = height;

    // draw border
    // KEX_FIXME method that requires less chars
    cgi.SCR_DrawChar(x - CONCHAR_WIDTH * scale, y - CONCHAR_WIDTH * scale, scale, 18, false);
    cgi.SCR_DrawChar(x + width_pixels, y - CONCHAR_WIDTH * scale, scale, 20, false);
    cgi.SCR_DrawChar(x - CONCHAR_WIDTH * scale, y + height_pixels, scale, 24, false);
    cgi.SCR_DrawChar(x + width_pixels, y + height_pixels, scale, 26, false);

    for (int cx = x; cx < x + width_pixels; cx += CONCHAR_WIDTH * scale)
    {
        cgi.SCR_DrawChar(cx, y - CONCHAR_WIDTH * scale, scale, 19, false);
        cgi.SCR_DrawChar(cx, y + height_pixels, scale, 25, false);
    }

    for (int cy = y; cy < y + height_pixels; cy += CONCHAR_WIDTH * scale)
    {
        cgi.SCR_DrawChar(x - CONCHAR_WIDTH * scale, cy, scale, 21, false);
        cgi.SCR_DrawChar(x + width_pixels, cy, scale, 23, false);
    }

    cgi.SCR_DrawColorPic(x, y, width_pixels, height_pixels, "_white", &rgba_black);

    // draw in columns
    for (int i = 0; i < hud_temp.num_columns; i++)
    {
        for (int r = 0, ry = y; r < hud_temp.num_rows; r++, ry += (CONCHAR_WIDTH + font_y_offset) * scale)
        {
            int x_offset = 0;

            // center
            if (r == 0)
            {
                x_offset = hud_temp.column_widths[i] / 2 -
                    cgi.SCR_MeasureFontString(hud_temp.table_rows[r].table_cells[i].text, scale).x / 2;
            }
            // right align
            else if (i != 0)
            {
                x_offset = hud_temp.column_widths[i] - cgi.SCR_MeasureFontString(hud_temp.table_rows[r].table_cells[i].text, scale).x;
            }

            //CG_DrawString(x + x_offset, ry, scale, hud_temp.table_rows[r].table_cells[i].text, r == 0, true);
            cgi.SCR_DrawFontString(hud_temp.table_rows[r].table_cells[i].text, x + x_offset, ry - font_y_offset * scale, scale, r == 0 ? &alt_color : &rgba_white, true, LEFT);
        }

        x += hud_temp.column_widths[i] + cgi.SCR_MeasureFontString(" ", 1).x;
    }
}


static void vrx_apply_stat_hax(const player_state_t *ps, int *value, int stat) {
    if (stat == STAT_ID_DAMAGE) {
        *value = ps_instant_dmg_value(ps);
    }

    if (stat == STAT_SCORE) {
        *value = ps_score_value(ps);
    }
}

/*
================
CG_ExecuteLayoutString

================
*/
static void CG_ExecuteLayoutString (const char *s, struct vrect_t hud_vrect, struct vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps)
{
    int     x, y;
    int     w, h;
    int     hx, hy;
    int     value;
    const char *token;
    int     width;
    int     index;

    if (!s[0])
        return;

    x = hud_vrect.x;
    y = hud_vrect.y;
    width = 3;

    hx = 320 / 2;
    hy = 240 / 2;

    bool flash_frame = cgi.CL_ClientTime() % 1000 < 500;

    // if non-zero, parse but don't affect state
    int32_t if_depth = 0; // current if statement depth
    int32_t endif_depth = 0; // at this depth, toggle skip_depth
    bool skip_depth = false; // whether we're in a dead stmt or not

    while (s)
    {
        token = COM_Parse (&s);
        if (!strcmp(token, "xl"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + atoi(token)) * scale + hud_safe.x;
            continue;
        }
        if (!strcmp(token, "xr"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + hud_vrect.width + atoi(token)) * scale - hud_safe.x;
            continue;
        }
        if (!strcmp(token, "xv"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + hud_vrect.width/2 + (atoi(token) - hx)) * scale;
            continue;
        }

        if (!strcmp(token, "yt"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + atoi(token)) * scale + hud_safe.y;
            continue;
        }
        if (!strcmp(token, "yb"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + hud_vrect.height + atoi(token)) * scale - hud_safe.y;
            continue;
        }
        if (!strcmp(token, "yv"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + hud_vrect.height/2 + (atoi(token) - hy)) * scale;
            continue;
        }

        if (!strcmp(token, "pic"))
        {   // draw a pic from a stat number
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = ps->stats[atoi(token)];
                if (value >= MAX_IMAGES)
                    cgi.Com_Error("Pic >= MAX_IMAGES");

                const char *const pic = cgi.get_configstring(CS_IMAGES + value);

                if (pic && *pic)
                {
                    cgi.Draw_GetPicSize (&w, &h, pic);
                    cgi.SCR_DrawPic (x, y, w * scale, h * scale, pic);
                }
            }

            continue;
        }

        if (!strcmp(token, "client"))
        {   // draw a deathmatch client block
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                x = (hud_vrect.x + hud_vrect.width/2 + (atoi(token) - hx)) * scale;
                x += 8 * scale;
            }
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                y = (hud_vrect.y + hud_vrect.height/2 + (atoi(token) - hy)) * scale;
                y += 7 * scale;
            }

            token = COM_Parse (&s);

            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");
            }

            int score;

            token = COM_Parse (&s);
            if (!skip_depth)
                score = atoi(token);

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                int ping;
                ping = atoi(token);

                if (!scr_usekfont->integer)
                    CG_DrawString (x + 32 * scale, y, scale, cgi.CL_GetClientName(value), false, true);
                else
                    cgi.SCR_DrawFontString(cgi.CL_GetClientName(value), x + 32 * scale, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);

                if (!scr_usekfont->integer)
                    CG_DrawString (x + 32 * scale, y + 10 * scale, scale, va("%d", score), true, true);
                else
                    cgi.SCR_DrawFontString(va("%d", score), x + 32 * scale, y + (10 - font_y_offset) * scale, scale, &rgba_white, true, LEFT);

                cgi.SCR_DrawPic(x + 96 * scale, y + 10 * scale, 9 * scale, 9 * scale, "ping");

                if (!scr_usekfont->integer)
                    CG_DrawString (x + 73 * scale + 32 * scale, y + 10 * scale, scale, va("%d", ping), false, true);
                else
                    cgi.SCR_DrawFontString (va("%d", ping), x + 107 * scale, y + (10 - font_y_offset) * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "ctf"))
        {   // draw a ctf client block
            int     score, ping;

            token = COM_Parse (&s);
            if (!skip_depth)
                x = (hud_vrect.x + hud_vrect.width/2 - hx + atoi(token)) * scale;
            token = COM_Parse (&s);
            if (!skip_depth)
                y = (hud_vrect.y + hud_vrect.height/2 - hy + atoi(token)) * scale;

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");
            }

            token = COM_Parse (&s);
            if (!skip_depth)
                score = atoi(token);

            token = COM_Parse (&s);
            if (!skip_depth)
            {
                ping = atoi(token);
                if (ping > 999)
                    ping = 999;
            }

            token = COM_Parse (&s);

            if (!skip_depth)
            {

                cgi.SCR_DrawFontString (va("%d", score), x, y - font_y_offset * scale, scale, value == playernum ? &alt_color : &rgba_white, true, LEFT);
                x += 3 * 9 * scale;
                cgi.SCR_DrawFontString (va("%d", ping), x, y - font_y_offset * scale, scale, value == playernum ? &alt_color : &rgba_white, true, LEFT);
                x += 3 * 9 * scale;
                cgi.SCR_DrawFontString (cgi.CL_GetClientName(value), x, y - font_y_offset * scale, scale, value == playernum ? &alt_color : &rgba_white, true, LEFT);

                if (*token)
                {
                    cgi.Draw_GetPicSize(&w, &h, token);
                    cgi.SCR_DrawPic(x - (w + 2) * scale, y, w * scale, h * scale, token);
                }
            }
            continue;
        }

        if (!strcmp(token, "picn"))
        {   // draw a pic from a name
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                cgi.Draw_GetPicSize(&w, &h, token);
                cgi.SCR_DrawPic(x, y, w * scale, h * scale, token);
            }
            continue;
        }

        if (!strcmp(token, "num"))
        {   // draw a number
            token = COM_Parse (&s);
            if (!skip_depth)
                width = atoi(token);
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                int stat = atoi(token);
                value = ps->stats[stat];

                // az: the damage and game hacks for vortex
                vrx_apply_stat_hax(ps, &value, stat);


                CG_DrawField (x, y, 0, width, value, scale);
            }
            continue;
        }
        // [Paril-KEX] special handling for the lives number
        if (!strcmp(token, "lives_num"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                value = ps->stats[atoi(token)];
                CG_DrawField(x, y, value <= 2 ? flash_frame : 0, 1, max(0, value - 2), scale);
            }
        }

        if (!strcmp(token, "hnum"))
        {
            // health number
            if (!skip_depth)
            {
                int     color;

                width = 5;
                value = ps->stats[STAT_HEALTH];
                if (value > 25)
                    color = 0;  // green
                else if (value > 0)
                    color = flash_frame;      // flash
                else
                    color = 1;
                if (ps->stats[STAT_FLASHES] & 1)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "dmgnum")) {
                // damage number
                if (!skip_depth && hud_data[playernum].dmg_counter)
                {
                    width = 8;
                    value = hud_data[playernum].dmg_counter;

                    if (hud_data[playernum].last_dmg_time + 50 > cgi.CL_ClientTime())
                    {
                        cgi.SCR_DrawColorPic(x, y, 16 * (width - 1) * scale, 28 * scale, "_white", &rgba_orange);
                    }

                    const char* total_s = va("%d total", value);
                    auto len = strlen(total_s) + 2;
                    CG_DrawField (x, y, 0, width, hud_data[playernum].dmg_instant, scale);
                    CG_DrawString(x + (16 * width - len * 8) * scale, y + 32 * scale, scale, total_s, false, true);
                }
                continue;
        }

        if (!strcmp(token, "anum"))
        {
            // ammo number
            if (!skip_depth)
            {
                int     color;

                width = 5;
                value = ps->stats[STAT_AMMO];

                int32_t min_ammo = cgi.CL_GetWarnAmmoCount(ps->stats[STAT_ACTIVE_WEAPON]);

                if (!min_ammo)
                    min_ammo = 5; // back compat

                if (value > min_ammo)
                    color = 0;  // green
                else if (value >= 0)
                    color = flash_frame;      // flash
                else
                    continue;   // negative number = don't show
                if (ps->stats[STAT_FLASHES] & 4)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "rnum"))
        {
            // armor number
            if (!skip_depth)
            {
                int     color;

                width = 5;
                value = ps->stats[STAT_ARMOR];
                if (value < 0)
                    continue;

                color = 0;  // green
                if (ps->stats[STAT_FLASHES] & 2)
                {
                    cgi.Draw_GetPicSize(&w, &h, "field_3");
                    cgi.SCR_DrawPic(x, y, w * scale, h * scale, "field_3");
                }

                CG_DrawField (x, y, color, width, value, scale);
            }
            continue;
        }

        if (!strcmp(token, "stat_string"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.get_configstring(index), false, true);
                else
                    cgi.SCR_DrawFontString(cgi.get_configstring(index), x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "cstring"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                CG_DrawHUDString (token, x, y, hx*2*scale, 0, scale, true);
            continue;
        }

        if (!strcmp(token, "string"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, token, false, true);
                else
                    cgi.SCR_DrawFontString(token, x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "cstring2"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
                CG_DrawHUDString (token, x, y, hx*2*scale, 0x80, scale, true);
            continue;
        }

        if (!strcmp(token, "string2"))
        {
            token = COM_Parse (&s);
            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, token, true, true);
                else
                    cgi.SCR_DrawFontString(token, x, y - font_y_offset * scale, scale, &alt_color, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "if"))
        {
            // if stmt
            token = COM_Parse (&s);

            if_depth++;

            // skip to endif
            if (!skip_depth && !ps->stats[atoi(token)])
            {
                skip_depth = true;
                endif_depth = if_depth;
            }

            continue;
        }

        if (!strcmp(token, "ifeq")) {
            // stat index
            token = COM_Parse (&s);

            if_depth++;

            int stat_index = atoi(token);
            if (stat_index < 0 || stat_index >= MAX_STATS)
                cgi.Com_Error("Bad stat_index");

            token = COM_Parse (&s);
            int value = atoi(token);
            if (ps->stats[stat_index] != value) {
                skip_depth = true;
                endif_depth = if_depth;
            }
        }

        if (!strcmp(token, "ifgef"))
        {
            // if stmt
            token = COM_Parse (&s);

            if_depth++;

            // skip to endif
            if (!skip_depth && cgi.CL_ServerFrame() < atoi(token))
            {
                skip_depth = true;
                endif_depth = if_depth;
            }

            continue;
        }

        if (!strcmp(token, "endif"))
        {
            if (skip_depth && if_depth == endif_depth)
                skip_depth = false;

            if_depth--;

            if (if_depth < 0)
                cgi.Com_Error("endif without matching if");

            continue;
        }

        // localization stuff
        if (!strcmp(token, "loc_stat_string"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.Localize(cgi.get_configstring(index), nullptr, 0), false, true);
                else
                    cgi.SCR_DrawFontString(cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "loc_stat_rstring"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                const char *s = cgi.Localize(cgi.get_configstring(index), nullptr, 0);
                if (!scr_usekfont->integer)
                    CG_DrawString (x - strlen(s) * CONCHAR_WIDTH * scale, y, scale, s, false, true);
                else
                {
                    vec2_t size = cgi.SCR_MeasureFontString(s, scale);
                    cgi.SCR_DrawFontString(s, x - size.x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
                }
            }
            continue;
        }

        if (!strcmp(token, "loc_stat_cstring"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                CG_DrawHUDString (cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y, hx*2*scale, 0, scale, true);
            }
            continue;
        }

        if (!strcmp(token, "loc_stat_cstring2"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index];

                if (cgi.CL_ServerProtocol() <= PROTOCOL_VERSION_3XX)
                    index = CS_REMAP(index).start / CS_MAX_STRING_LENGTH;

                if (index < 0 || index >= MAX_CONFIGSTRINGS)
                    cgi.Com_Error("Bad stat_string index");
                CG_DrawHUDString (cgi.Localize(cgi.get_configstring(index), nullptr, 0), x, y, hx*2*scale, 0x80, scale, true);
            }
            continue;
        }

        static char arg_tokens[MAX_LOCALIZATION_ARGS + 1][MAX_TOKEN_CHARS];
        static const char *arg_buffers[MAX_LOCALIZATION_ARGS];

        if (!strcmp(token, "loc_cstring"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }

            if (!skip_depth)
                CG_DrawHUDString (cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y, hx*2*scale, 0, scale, true);
            continue;
        }

        if (!strcmp(token, "loc_string"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }

            if (!skip_depth)
            {
                if (!scr_usekfont->integer)
                    CG_DrawString (x, y, scale, cgi.Localize(arg_tokens[0], arg_buffers, num_args), false, true);
                else
                    cgi.SCR_DrawFontString(cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "loc_cstring2"))
        {
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }

            if (!skip_depth)
                CG_DrawHUDString (cgi.Localize(arg_tokens[0], arg_buffers, num_args), x, y, hx*2*scale, 0x80, scale, true);
            continue;
        }

        if (!strcmp(token, "loc_string2") || !strcmp(token, "loc_rstring2") ||
            !strcmp(token, "loc_string") || !strcmp(token, "loc_rstring"))
        {
            bool green = token[strlen(token) - 1] == '2';
            bool rightAlign = !Q_strncasecmp(token, "loc_rstring", strlen("loc_rstring"));
            int32_t num_args = atoi(COM_Parse (&s));

            if (num_args < 0 || num_args >= MAX_LOCALIZATION_ARGS)
                cgi.Com_Error("Bad loc string");

            // parse base
            token = COM_Parse (&s);
            Q_strlcpy(arg_tokens[0], token, sizeof(arg_tokens[0]));

            // parse args
            for (int32_t i = 0; i < num_args; i++)
            {
                token = COM_Parse (&s);
                Q_strlcpy(arg_tokens[1 + i], token, sizeof(arg_tokens[0]));
                arg_buffers[i] = arg_tokens[1 + i];
            }

            if (!skip_depth)
            {
                const char *locStr = cgi.Localize(arg_tokens[0], arg_buffers, num_args);
                int xOffs = 0;
                if (rightAlign)
                {
                    xOffs = scr_usekfont->integer ? cgi.SCR_MeasureFontString(locStr, scale).x : strlen(locStr) * CONCHAR_WIDTH * scale;
                }

                if (!scr_usekfont->integer)
                    CG_DrawString (x - xOffs, y, scale, locStr, green, true);
                else
                    cgi.SCR_DrawFontString(locStr, x - xOffs, y - font_y_offset * scale, scale, green ? &alt_color : &rgba_white, true, LEFT);
            }
            continue;
        }

        // draw time remaining
        if (!strcmp(token, "time_limit"))
        {
            // end frame
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                int32_t end_frame = atoi(token);

                if (end_frame < cgi.CL_ServerFrame())
                    continue;

                uint64_t remaining_ms = (end_frame - cgi.CL_ServerFrame()) * cgi.frame_time_ms;

                const bool green = true;
                arg_buffers[0] = va("%02d:%02d", remaining_ms / 1000 / 60, remaining_ms / 1000 % 60);

                const char *locStr = cgi.Localize("$g_score_time", arg_buffers, 1);
                int xOffs = scr_usekfont->integer ? cgi.SCR_MeasureFontString(locStr, scale).x : strlen(locStr) * CONCHAR_WIDTH * scale;
                if (!scr_usekfont->integer)
                    CG_DrawString (x - xOffs, y, scale, locStr, green, true);
                else
                    cgi.SCR_DrawFontString(locStr, x - xOffs, y - font_y_offset * scale, scale, green ? &alt_color : &rgba_white, true, LEFT);
            }
        }

        // draw client dogtag
        if (!strcmp(token, "dogtag"))
        {
            token = COM_Parse (&s);

            if (!skip_depth)
            {
                value = atoi(token);
                if (value >= MAX_CLIENTS || value < 0)
                    cgi.Com_Error("client >= MAX_CLIENTS");

                const char* path = va("/tags/%s", cgi.CL_GetClientDogtag(value));
                cgi.SCR_DrawPic(x, y, 198 * scale, 32 * scale, path);
            }
        }

        if (!strcmp(token, "start_table"))
        {
            token = COM_Parse (&s);
            value = atoi(token);

            if (!skip_depth)
            {
                if (value >= q_countof(hud_temp.table_rows[0].table_cells))
                    cgi.Com_Error("table too big");

                hud_temp.num_columns = value;
                hud_temp.num_rows = 1;

                for (int i = 0; i < value; i++)
                    hud_temp.column_widths[i] = 0;
            }

            for (int i = 0; i < value; i++)
            {
                token = COM_Parse (&s);
                if (!skip_depth)
                {
                    token = cgi.Localize(token, nullptr, 0);
                    Q_strlcpy(hud_temp.table_rows[0].table_cells[i].text, token, sizeof(hud_temp.table_rows[0].table_cells[i].text));
                    hud_temp.column_widths[i] = max(hud_temp.column_widths[i], (size_t) cgi.SCR_MeasureFontString(hud_temp.table_rows[0].table_cells[i].text, scale).x);
                }
            }
        }

        if (!strcmp(token, "table_row"))
        {
            token = COM_Parse (&s);
            value = atoi(token);

            if (!skip_depth)
            {
                if (hud_temp.num_rows >= q_countof(hud_temp.table_rows))
                {
                    cgi.Com_Error("table too big");
                    return;
                }
            }

            auto *row = &hud_temp.table_rows[hud_temp.num_rows];

            for (int i = 0; i < value; i++)
            {
                token = COM_Parse (&s);
                if (!skip_depth)
                {
                    Q_strlcpy(row->table_cells[i].text, token, sizeof(row->table_cells[i].text));
                    hud_temp.column_widths[i] = max(hud_temp.column_widths[i], (size_t) cgi.SCR_MeasureFontString(row->table_cells[i].text, scale).x);
                }
            }

            if (!skip_depth)
            {
                for (int i = value; i < hud_temp.num_columns; i++)
                    row->table_cells[i].text[0] = '\0';

                hud_temp.num_rows++;
            }
        }

        if (!strcmp(token, "draw_table"))
        {
            if (!skip_depth)
            {
                // in scaled pixels, incl padding between elements
                uint32_t total_inner_table_width = 0;

                for (int i = 0; i < hud_temp.num_columns; i++)
                {
                    if (i != 0)
                        total_inner_table_width += cgi.SCR_MeasureFontString(" ", scale).x;

                    total_inner_table_width += hud_temp.column_widths[i];
                }

                // in scaled pixels
                uint32_t total_table_height = hud_temp.num_rows * (CONCHAR_WIDTH + font_y_offset) * scale;

                CG_DrawTable(x, y, total_inner_table_width, total_table_height, scale);
            }
        }

        if (!strcmp(token, "stat_pname"))
        {
            token = COM_Parse(&s);

            if (!skip_depth)
            {
                index = atoi(token);
                if (index < 0 || index >= MAX_STATS)
                    cgi.Com_Error("Bad stat_string index");
                index = ps->stats[index] - 1;

                if (!scr_usekfont->integer)
                    CG_DrawString(x, y, scale, cgi.CL_GetClientName(index), false, true);
                else
                    cgi.SCR_DrawFontString(cgi.CL_GetClientName(index), x, y - font_y_offset * scale, scale, &rgba_white, true, LEFT);
            }
            continue;
        }

        if (!strcmp(token, "health_bars"))
        {
            if (skip_depth)
                continue;

            const byte *stat = (const byte *)&ps->stats[STAT_HEALTH_BARS];
            const char *name = cgi.Localize(cgi.get_configstring(CONFIG_HEALTH_BAR_NAME), nullptr, 0);

            CG_DrawHUDString(name, (hud_vrect.x + hud_vrect.width/2 + -160) * scale, y, 320 / 2 * 2 * scale, 0, scale, true);

            float bar_width = (hud_vrect.width * scale - hud_safe.x * 2) * 0.50f;
            float bar_height = 4 * scale;

            y += cgi.SCR_FontLineHeight(scale);

            float x = (hud_vrect.x + hud_vrect.width * 0.5f) * scale - bar_width * 0.5f;

            // 2 health bars, hardcoded
            for (size_t i = 0; i < 2; i++, stat++)
            {
                if (!(*stat & 0b10000000))
                    continue;

                float percent = (*stat & 0b01111111) / 127.f;

                cgi.SCR_DrawColorPic(x, y, bar_width + scale, bar_height + scale, "_white", &rgba_black);

                if (percent > 0)
                    cgi.SCR_DrawColorPic(x, y, bar_width * percent, bar_height, "_white", &rgba_red);

                auto col = (rgba_t){ .r=80, .g=80, .b=80, .a=255 };
                if (percent < 1)
                    cgi.SCR_DrawColorPic(x + bar_width * percent, y, bar_width * (1.f - percent), bar_height, "_white", &col);

                y += bar_height * 3;
            }
        }

        if (!strcmp(token, "story"))
        {
            const char *story_str = cgi.get_configstring(CONFIG_STORY);

            if (!*story_str)
                continue;

            const char *localized = cgi.Localize(story_str, nullptr, 0);
            vec2_t size = cgi.SCR_MeasureFontString(localized, scale);
            float centerx = (hud_vrect.x + hud_vrect.width * 0.5f) * scale;
            float centery = (hud_vrect.y + hud_vrect.height * 0.5f) * scale - size.y * 0.5f;

            cgi.SCR_DrawFontString(localized, centerx, centery, scale, &rgba_white, true, CENTER);
        }
    }

    if (skip_depth)
        cgi.Com_Error("if with no matching endif");
}

static cvar_t *cl_skipHud;
static cvar_t *cl_paused;

/*
================
CL_DrawInventory
================
*/
constexpr size_t DISPLAY_ITEMS   = 19;

static void CG_DrawInventory(const player_state_t *ps, const int16_t inventory[MAX_ITEMS], const struct vrect_t hud_vrect, const int32_t scale)
{
    int     i;
    int     index[MAX_ITEMS];

    int selected = ps->stats[STAT_SELECTED_ITEM];

    int num = 0;
    int selected_num = 0;
    for (i=0 ; i<MAX_ITEMS ; i++) {
        if ( i == selected ) {
            selected_num = num;
        }
        if ( inventory[i] ) {
            index[num] = i;
            num++;
        }
    }

    // determine scroll point
    int top = selected_num - DISPLAY_ITEMS / 2;
    if (num - top < DISPLAY_ITEMS)
        top = num - DISPLAY_ITEMS;
    if (top < 0)
        top = 0;

    int x = hud_vrect.x * scale;
    int y = hud_vrect.y * scale;
    int width = hud_vrect.width;
    int height = hud_vrect.height;

    x += (width / 2 - 256 / 2) * scale;
    y += (height / 2 - 216 / 2) * scale;

    int pich, picw;
    cgi.Draw_GetPicSize(&picw, &pich, "inventory");
    cgi.SCR_DrawPic(x, y+8*scale, picw * scale, pich * scale, "inventory");

    y += 27 * scale;
    x += 22 * scale;

    for (i=top ; i<num && i < top+DISPLAY_ITEMS ; i++)
    {
        int item = index[i];
        if (item == selected) // draw a blinky cursor by the selected item
        {
            if ( cgi.CL_ClientRealTime() * 10 & 1)
                cgi.SCR_DrawChar(x-8, y, scale, 15, false);
        }

        if (!scr_usekfont->integer)
        {
            CG_DrawString(x, y, scale,
                va("%03d %s", inventory[item],
                    cgi.Localize(cgi.get_configstring(CS_ITEMS + item), nullptr, 0)),
                item == selected, false);
        }
        else
        {
            const char *string = va("%03d", inventory[item]);
            cgi.SCR_DrawFontString(string, x + 216 * scale - 16 * scale, y - font_y_offset * scale, scale, item == selected ? &alt_color : &rgba_white, true, RIGHT);

            string = cgi.Localize(cgi.get_configstring(CS_ITEMS + item), nullptr, 0);
            cgi.SCR_DrawFontString(string, x + 16 * scale, y - font_y_offset * scale, scale, item == selected ? &alt_color : &rgba_white, true, LEFT);
        }

        y += 8 * scale;
    }
}

extern uint64_t cgame_init_time;

void CG_DrawHUD (
    const int32_t isplit,
    const struct cg_server_data_t *data,
    const struct vrect_t hud_vrect,
    const struct vrect_t hud_safe,
    const int32_t scale,
    const int32_t playernum,
    const player_state_t *ps)
{
    if (cgi.CL_InAutoDemoLoop())
    {
        if (cl_paused->integer) return; // demo is paused, menu is open

        const uint64_t time = cgi.CL_ClientRealTime() - cgame_init_time;
        if (time < 20000 &&
            time % 4000 < 2000)
            cgi.SCR_DrawFontString(cgi.Localize("$m_eou_press_button", nullptr, 0), hud_vrect.width * 0.5f * scale, (hud_vrect.height - 64.f) * scale, scale, &rgba_green, true, CENTER);
        return;
    }

    int instant_dmg = ps_instant_dmg_value(ps);
    if (instant_dmg) {
        hud_data[playernum].dmg_instant = instant_dmg;
        hud_data[playernum].dmg_counter += hud_data[playernum].dmg_instant;
        hud_data[playernum].last_dmg_time = cgi.CL_ClientTime();
    }

    if (cgi.CL_ClientTime() > hud_data[playernum].last_dmg_time + 2000 && hud_data[playernum].dmg_counter) {
        hud_data[playernum].dmg_counter = 0;
        hud_data[playernum].dmg_instant = 0;
    }

    // draw HUD
    if (!cl_skipHud->integer && !(ps->stats[STAT_LAYOUTS] & LAYOUTS_HIDE_HUD)) {
        auto sbar = cgi.get_configstring(CS_STATUSBAR);
        CG_ExecuteLayoutString(sbar, hud_vrect, hud_safe, scale, playernum, ps);
    }

    // draw centerprint string
    CG_CheckDrawCenterString(ps, hud_vrect, hud_safe, isplit, scale);

    // draw notify
    CG_DrawNotify(isplit, hud_vrect, hud_safe, scale);

    // svc_layout still drawn with hud off
    if (ps->stats[STAT_LAYOUTS] & LAYOUTS_LAYOUT)
        CG_ExecuteLayoutString(data->layout, hud_vrect, hud_safe, scale, playernum, ps);

    // inventory too
    if (ps->stats[STAT_LAYOUTS] & LAYOUTS_INVENTORY)
        CG_DrawInventory(ps, data->inventory, hud_vrect, scale);
}

/*
================
CG_TouchPics

================
*/
void CG_TouchPics()
{
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 11; j++)
            cgi.Draw_RegisterPic(sb_nums[i][j]);

    cgi.Draw_RegisterPic("inventory");

    font_y_offset = (cgi.SCR_FontLineHeight(1) - CONCHAR_WIDTH) / 2;
}

void CG_InitScreen()
{
    cl_paused = cgi.cvar("paused", "0", CVAR_NOFLAGS);
    cl_skipHud = cgi.cvar("cl_skipHud", "0", CVAR_ARCHIVE);
    scr_usekfont = cgi.cvar("scr_usekfont", "1", CVAR_NOFLAGS);

    scr_centertime  = cgi.cvar ("scr_centertime", "5.0",  CVAR_ARCHIVE); // [Sam-KEX] Changed from 2.5
    scr_printspeed  = cgi.cvar ("scr_printspeed", "0.04", CVAR_NOFLAGS); // [Sam-KEX] Changed from 8
    cl_notifytime   = cgi.cvar ("cl_notifytime", "5.0",   CVAR_ARCHIVE);
    scr_maxlines    = cgi.cvar ("scr_maxlines", "4",      CVAR_ARCHIVE);
    ui_acc_contrast = cgi.cvar ("ui_acc_contrast", "0",   CVAR_NOFLAGS);
    ui_acc_alttypeface = cgi.cvar("ui_acc_alttypeface", "0", CVAR_NOFLAGS);

    memset(&hud_data, 0, sizeof(hud_data));
    for (int i = 0; i < MAX_SPLIT_PLAYERS; i++)
        hud_data[i].center_index = -1;
}