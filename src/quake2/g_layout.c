#include "g_local.h"

// a copy of the "va" function but with its own memory.
#define MAX_LVA_SIZE 1024
#define MAX_SIDEBAR_ROWS 32
typedef struct lva_result_s
{
	char str[MAX_LVA_SIZE];
	int len;
} lva_result_t;

typedef struct sidebar_result_s
{
	lva_result_t name;
	lva_result_t data;
	layout_pos_t pos;
} sidebar_result_t;

typedef struct sidebar_s
{
	sidebar_result_t entry[MAX_SIDEBAR_ROWS];
	int entry_count;
} sidebar_t;

void sidebar_add_entry(sidebar_t* sidebar, sidebar_result_t entry)
{
	if (sidebar->entry_count < MAX_SIDEBAR_ROWS - 1)
	{
		sidebar->entry[sidebar->entry_count++] = entry;
	}
}

lva_result_t lva(const char* format, ...)
{
	va_list		argptr;
	lva_result_t ret;

	va_start(argptr, format);
	ret.len = vsnprintf(ret.str, MAX_LVA_SIZE, format, argptr);
	va_end(argptr);

	return ret;
}

size_t layout_remaining(layout_t* layout);
void layout_clear(layout_t* layout);

void layout_add_lva(layout_t* layout, lva_result_t* s)
{
	if (s->len < 0)
	{
		gi.dprintf("g_layout.c: invalid encoding\n");
		return;
	}

	if (s->len >= MAX_LVA_SIZE)
	{
		gi.dprintf("g_layout.c: formatted data overflow");
		return;
	}

	if (layout->current_len + s->len < MAX_LAYOUT_LEN)
	{
		strcat(layout->layout + layout->current_len, s->str);
		layout->current_len += s->len;
	} else
	{
		gi.dprintf("g_layout.c: layout overflow\n");
	}
}

void layout_add_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("string \"%s\" ", str);
	layout_add_lva(layout, &text);
}

void layout_add_highlight_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("string2 \"%s\" ", str);
	layout_add_lva(layout, &text);
}

void layout_add_centerstring(layout_t* layout, char* str)
{
	lva_result_t text = lva("cstring \"%s\" ", str);
	layout_add_lva(layout, &text);
}

void layout_add_centerstring_green(layout_t* layout, char* str)
{
	lva_result_t text = lva("cstring2 \"%s\" ", str);
	layout_add_lva(layout, &text);
}

void layout_add_raw_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("%s ", str);
	layout_add_lva(layout, &text);
}

void layout_add_pic(layout_t* layout, int i)
{
	lva_result_t text = lva("pic %d ", i );
	layout_add_lva(layout, &text);
}

void layout_add_pic_name(layout_t* layout, char* str)
{
	lva_result_t text = lva("picn \"%s\" ", str);
	layout_add_lva(layout, &text);
}

void layout_add_num(layout_t* layout, int width, int num)
{
	lva_result_t text = lva("num %d %d ", width, num);
	layout_add_lva(layout, &text);
}

layout_pos_t layout_set_cursor_x(int x, layout_pos_type_x posx_type)
{
	layout_pos_t ret = { 0 };
	ret.x = x;
	ret.x_mode = posx_type;
	ret.pos_flags = LPF_X;

	return ret;
}

layout_pos_t layout_set_cursor_y(int y, layout_pos_type_y posy_type)
{
	layout_pos_t ret = { 0 };
	ret.y = y;
	ret.y_mode = posy_type;
	ret.pos_flags = LPF_Y;

	return ret;
}

layout_pos_t layout_set_cursor_xy(
	int x, layout_pos_type_x posx_type,
	int y, layout_pos_type_y posy_type
)
{
	layout_pos_t ret = { 0 };
	ret.x = x;
	ret.x_mode = posx_type;
	ret.y = y;
	ret.y_mode = posy_type;
	ret.pos_flags = LPF_XY;

	return ret;
}

void layout_apply_pos(layout_t* layout, layout_pos_t pos)
{
	// only apply cursor change if x/y positions are different
	if (pos.pos_flags & LPF_X)
	{
		if (layout->last_pos.x != pos.x || layout->last_pos.x_mode != pos.x_mode)
		{
			layout->last_pos.x = pos.x;
			layout->last_pos.x_mode = pos.x_mode;

			lva_result_t text = {0};
			switch (pos.x_mode)
			{
			case XM_LEFT:
				text = lva("xl %d ", pos.x);
				break;
			case XM_RIGHT:
				text = lva("xr %d ", pos.x);
				break;
			case XM_CENTER:
				text = lva("xv %d ", pos.x);
				break;
			default:
				gi.dprintf("invalid x-mode %d\n", pos.x_mode);
			}

			layout_add_lva(layout, &text);
		}
	}

	if (pos.pos_flags & LPF_Y)
	{
		if (layout->last_pos.y != pos.y || layout->last_pos.y_mode != pos.y_mode)
		{
			layout->last_pos.y = pos.y;
			layout->last_pos.y_mode = pos.y_mode;

			lva_result_t text = { 0 };
			switch (pos.y_mode)
			{
			case YM_TOP:
				text = lva("yt %d ", pos.y);
				break;
			case YM_BOTTOM:
				text = lva("yb %d ", pos.y);
				break;
			case YM_CENTER:
				text = lva("yv %d ", pos.y);
				break;
			default:
				gi.dprintf("invalid y-mode %d\n", pos.y_mode);
			}

			layout_add_lva(layout, &text);
		}
	}
}

// -1 if it does not exist
int layout_has_tracked_entity(layout_t* layout, edict_t* ent)
{
	for (int i = 0; i < layout->tracked_count; i++)
	{
		if (layout->tracked_list[i] == ent) return i;
	}

	return -1;
}

qboolean layout_add_tracked_entity(layout_t* layout, edict_t* ent)
{
	if (layout_has_tracked_entity(layout, ent) != -1) return false;

	// too many tracked ents?
	if (layout->tracked_count >= MAX_LAYOUT_TRACKED_ENTS - 1)
	{
		return false;
	}

	layout->tracked_list[layout->tracked_count++] = ent;
	layout->dirty = true;
	return true;
}

qboolean layout_remove_tracked_entity(layout_t* layout, edict_t* ent)
{
	int index = layout_has_tracked_entity(layout, ent);

	if (index == -1) return false;
	
	layout->tracked_list[index] = layout->tracked_list[--layout->tracked_count];
	layout->dirty = true;
	return true;
}

void layout_clean_tracked_entity_list(layout_t* layout)
{
	qboolean cleaned = false;
	for (int i = 0; i < layout->tracked_count; i++)
	{
		if (!layout->tracked_list[i]->inuse || layout->tracked_list[i]->deadflag != DEAD_NO)
		{
			layout->tracked_list[i] = layout->tracked_list[--layout->tracked_count];
			i--; // continue with this entity
			cleaned = true;
		}
	}

	if (cleaned)
		layout->dirty = true;
}

layout_pos_t sidebar_get_next_line_pos(layout_t* layout)
{
	layout_pos_t ret = layout_set_cursor_xy(
		5, XM_LEFT,
		(layout->line + 2) * 8, YM_CENTER
	);

	layout->line++;
	return ret;
}

sidebar_result_t layout_add_entity_info(layout_t* layout, edict_t* ent)
{
	sidebar_result_t res;
	layout_pos_t pos = sidebar_get_next_line_pos(layout);

	lva_result_t name = { 0 };
	lva_result_t data = { 0 };
	switch (ent->mtype)
	{
	case M_SOLDIERLT:
	case M_SOLDIER:
	case M_SOLDIERSS:
	case M_FLIPPER:
	case M_FLYER:
	case M_INFANTRY:
	case M_INSANE:
	case M_GUNNER:
	case M_CHICK:
	case M_PARASITE:
	case M_FLOATER:
	case M_HOVER:
	case M_BERSERK:
	case M_MEDIC:
	case M_MUTANT:
	case M_BRAIN:
	case M_GLADIATOR:
	case M_TANK:
	case M_FORCEWALL:
	case M_MINISENTRY:
		name = lva("%s", V_GetMonsterName(ent));
		data = lva("+%d/%d", ent->health, ent->monsterinfo.power_armor_power);
		break;
	case M_SENTRY:
		name = lva("sentry");
		data = lva("+%d/%d %d/%da", ent->health, ent->monsterinfo.power_armor_power, ent->light_level, ent->style);
		break;
	case M_LASER:
		name = lva("laser");
		data = lva("+%d", ent->activator->health);
		break;
	case TOTEM_FIRE:
	case TOTEM_WATER:
	case TOTEM_AIR:
	case TOTEM_EARTH:
	case TOTEM_NATURE:
	case TOTEM_DARKNESS:
	case M_SKULL:
	case M_SPIKEBALL:
	case M_SPIKER:
	case M_OBSTACLE:
	case M_HEALER:
	case M_GASSER:
	case M_DECOY:
		name = lva("%s", V_GetMonsterName(ent));
		data = lva("+%d", ent->health);
		break;
	case M_COCOON:
		name = lva("cocoon");
		if (ent->enemy) {
			if (ent->enemy->client) {
				data = lva("+%d (%s)", ent->health, ent->enemy->client->pers.netname);
			} else
			{
				if (PM_MonsterHasPilot(ent->enemy))
					data = lva("+%d (%s)", ent->health, ent->enemy->owner->client->pers.netname);
				else
					data = lva("+%d (%s)", ent->health, V_GetMonsterName(ent->enemy));
			}
		} else
			data = lva("+%d", ent->health);
		break;
	case M_AUTOCANNON:
		name = lva("cannon", V_GetMonsterName(ent));
		data = lva("+%d %da", ent->health, ent->light_level);
		break;
	default:
		gi.dprintf("g_layout.c: unsupported mtype %d\n");
	}

	res.name = name;
	res.data = data;
	res.pos = pos;
	return res;
}

// curses.c
char* GetCurseName(int type);

sidebar_result_t layout_add_curse_info(layout_t* layout, que_t* curse)
{
	sidebar_result_t res;

	res.pos = sidebar_get_next_line_pos(layout);
	res.name = lva("%s", GetCurseName(curse->ent->atype));
	res.data = lva("%.1fs", curse->time - level.time);

	return res;
}

void layout_generate_entities(layout_t* layout, sidebar_t* sidebar)
{
	for (int i = 0; i < layout->tracked_count; i++)
	{
		const sidebar_result_t res = layout_add_entity_info(layout, layout->tracked_list[i]);
		sidebar_add_entry(sidebar, res);
	}
}

sidebar_result_t layout_add_aura_info(layout_t* layout, que_t* que)
{
	sidebar_result_t res;

	res.pos = sidebar_get_next_line_pos(layout);

	switch (que->ent->mtype)
	{
	case AURA_SALVATION:
		res.name = lva("salvation");
		break;
	case AURA_HOLYFREEZE:
		res.name = lva("holy freeze");
		break;
	case AURA_HOLYSHOCK:
		res.name = lva("holy shock");
		break;
	case AURA_MANASHIELD:
		res.name = lva("manashield");
		break;
	default:
		res.name = lva("%s", que->ent->classname);
		break;
	}

	if (que->ent->owner && que->ent->owner->client)
		res.data = lva(
			"%.1fs (%s)", 
			que->time - level.time, 
			que->ent->owner->client->pers.netname
		);
	else
		res.data = lva("%.1fs", que->time - level.time);

	return res;
}

void layout_generate_curses(edict_t* ent, sidebar_t* sidebar)
{
	if (ent->cocoon_time >= level.time) {
		float dt = ent->cocoon_time - level.time;
		sidebar_result_t res;
		res.pos = sidebar_get_next_line_pos(&ent->client->layout);
		res.name = lva("cocooned");
		res.data = lva("%.1fs +%.1f%%", dt, (ent->cocoon_factor - 1) * 100.0f);
		sidebar_add_entry(sidebar, res);
	}

	if (ent->fury_time >= level.time)
	{
		float dt = ent->fury_time - level.time;
		sidebar_result_t res;
		res.pos = sidebar_get_next_line_pos(&ent->client->layout);
		res.name = lva("furied");
		res.data = lva("%.2fs", dt);
		sidebar_add_entry(sidebar, res);
	}

	for (int i = 0; i < QUE_MAXSIZE; i++)
	{
		que_t* curse = &ent->curses[i];
		sidebar_result_t res;
		if (!que_valident(curse)) continue;

		res = layout_add_curse_info(&ent->client->layout, curse);
		sidebar_add_entry(sidebar, res);
	}

	for (int i = 0; i < QUE_MAXSIZE; i++)
	{
		que_t* aura = &ent->auras[i];
		sidebar_result_t res;
		if (!que_valident(aura)) continue;

		res = layout_add_aura_info(&ent->client->layout, aura);
		sidebar_add_entry(sidebar, res);
	}
}

void layout_reset(layout_t* layout)
{
	memset(layout->layout, 0, MAX_LAYOUT_LEN);
	layout->line = 0;
	layout->current_len = 0;
	memset(&layout->last_pos, 0, sizeof layout->last_pos);
}

void sidebar_emit_layout(layout_t* layout, sidebar_t* sidebar)
{
	int namelen = 0;
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		namelen = max(namelen, sidebar->entry[i].name.len);
	}

	namelen += 1;

	// emit everything. assumption is that entry.pos will update both x and y.
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		// first, emit the name string
		layout_apply_pos(layout, sidebar->entry[i].pos);
		layout_add_string(layout, sidebar->entry[i].name.str);
	}

	// doing it this way we save cursor position changes from the 3rd row.
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		layout_pos_t npos = sidebar->entry[i].pos;

		// now emit the data string
		npos.x += namelen * 8;
		layout_apply_pos(layout, npos);
		layout_add_string(layout, sidebar->entry[i].data.str);
	}
}

void layout_generate_all(edict_t* ent)
{
	sidebar_t sidebar = {0};

	layout_clean_tracked_entity_list(&ent->client->layout);
	layout_reset(&ent->client->layout);

	if (ent->client->ability_delay > level.time)
	{
		sidebar_result_t entry;
		entry.pos = sidebar_get_next_line_pos(&ent->client->layout);
		entry.name = lva("cd");
		entry.data = lva("%.1f", ent->client->ability_delay - level.time);

		sidebar_add_entry(&sidebar, entry);
	}

	layout_generate_entities(&ent->client->layout, &sidebar);
	layout_generate_curses(ent, &sidebar);

	sidebar_emit_layout(&ent->client->layout, &sidebar);

	ent->client->layout.dirty = false;
}

void layout_send(edict_t* ent)
{
	if (!ent || !ent->inuse)
		return;

	if (!ent->client)
		return;

	gi.WriteByte(svc_layout);
	gi.WriteString(ent->client->layout.layout);
	gi.unicast(ent, false);
}