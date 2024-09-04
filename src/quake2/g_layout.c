#include "g_local.h"

lva_result_t lva(const char* format, ...)
{
	va_list		argptr;
	lva_result_t ret;

	va_start(argptr, format);
	ret.len = vsnprintf(ret.str, MAX_LVA_SIZE, format, argptr);
	va_end(argptr);

	return ret;
}

#define MAX_SIDEBAR_ROWS 32
#define STYLE_WHITE	0
#define STYLE_GREEN	1
#define STYLE_BLINK 2

typedef struct sidebar_result_s
{
	lva_result_t name;
	lva_result_t data;
	lva_result_t pic;
	layout_pos_t pos;
	int name_style; // 0 = white text, 1 = green text
	int data_style; // 0 = white text, 1 = green text
} sidebar_entry_t;

typedef struct sidebar_s
{
	sidebar_entry_t entry[MAX_SIDEBAR_ROWS];
	int entry_count;
	int line;
	int y_offset;
} sidebar_t;

void sidebar_add_entry(sidebar_t* sidebar, sidebar_entry_t entry)
{
	if (sidebar->entry_count < MAX_SIDEBAR_ROWS - 1)
	{
		sidebar->entry[sidebar->entry_count++] = entry;
	}
}


qboolean layout_add_lva(layout_t* layout, lva_result_t* s)
{
	if (s->len < 0)
	{
		gi.dprintf("g_layout.c: invalid encoding\n");
		return false;
	}

	if (s->len >= MAX_LVA_SIZE)
	{
		gi.dprintf("g_layout.c: formatted data overflow");
		return false;
	}

	if (layout->current_len + s->len < MAX_LAYOUT_LEN)
	{
		strcat(layout->layout + layout->current_len, s->str);
		layout->current_len += s->len;
	}
	else
	{
		gi.dprintf("g_layout.c: layout overflow\n");
		return false;
	}

	return true;
}

qboolean layout_add_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("string \"%s\" ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_highlight_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("string2 \"%s\" ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_centerstring(layout_t* layout, char* str)
{
	lva_result_t text = lva("cstring \"%s\" ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_centerstring_green(layout_t* layout, char* str)
{
	lva_result_t text = lva("cstring2 \"%s\" ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_raw_string(layout_t* layout, char* str)
{
	lva_result_t text = lva("%s ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_pic(layout_t* layout, int i)
{
	lva_result_t text = lva("pic %d ", i );
	return layout_add_lva(layout, &text);
}

qboolean layout_add_pic_name(layout_t* layout, char* str)
{
	lva_result_t text = lva("picn \"%s\" ", str);
	return layout_add_lva(layout, &text);
}

qboolean layout_add_num(layout_t* layout, int width, int num)
{
	lva_result_t text = lva("num %d %d ", width, num);
	return layout_add_lva(layout, &text);
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

qboolean layout_apply_pos(layout_t* layout, layout_pos_t pos)
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

			if (!layout_add_lva(layout, &text))
				return false;
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

			if (!layout_add_lva(layout, &text))
				return false;
		}
	}

	return true;
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

layout_pos_t sidebar_get_next_line_pos(sidebar_t* sidebar)
{
	layout_pos_t ret = layout_set_cursor_xy(
		5, XM_LEFT,
		(sidebar->line + 2) * 8 + sidebar->y_offset, YM_CENTER
	);

	sidebar->line++;
	return ret;
}

sidebar_entry_t layout_add_entity_info(sidebar_t* sidebar, edict_t* ent)
{
	sidebar_entry_t res = {0};
	layout_pos_t pos = sidebar_get_next_line_pos(sidebar);

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
	case M_BARON_FIRE:
	case M_SHAMBLER:
		name = lva("%s", V_GetMonsterName(ent));
		data = lva("+%d/%d", ent->health, ent->monsterinfo.power_armor_power);
		break;
	case M_SENTRY:
		name = lva("sentry");
		data = lva("+%d/%d %d/%da", ent->health, ent->monsterinfo.power_armor_power, ent->light_level, ent->style);
		break;
	case M_MINISENTRY:
	case M_BEAMSENTRY:
		name = lva("sentry");
		data = lva("+%d/%d %da", ent->health, ent->monsterinfo.power_armor_power, ent->light_level);
		break;
	case M_LASER:
		name = lva("laser");
		data = lva("+%d", ent->activator->health);
		break;
	case M_MAGMINE:
		name = lva("magmine");
		data = lva("%dc", ent->health);
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
	case M_BARREL:
		name = lva("barrel");
		data = lva("+%d %dd %dr", ent->health, ent->dmg, (int)ceil(ent->dmg_radius));
		break;
	case M_ARMOR:
		name = lva("armor");
		data = lva("%dd %dr %ds", ent->dmg, (int)ceil(ent->dmg_radius), (int)ceil(ent->delay-level.time));
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
		name = lva("cannon");
		data = lva("+%d %da", ent->health, ent->light_level);
		break;
	case M_DETECTOR:
	case M_ALARM:
		name = lva("detector");
		data = lva("+%d %dr %ds", ent->health, (int)ceil(ent->dmg_radius), (int)ceil(ent->delay - level.time));
		break;
	case M_SUPPLYSTATION:
		name = lva("SS");
		if (ent->s.effects & EF_PLASMA)
			data = lva("+%d", ent->health);
		else
			data = lva("+%d %ds %d%%", ent->health, (int)ceil(0.1*(ent->count-level.framenum)), (int)ceil(ent->wait));
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

sidebar_entry_t layout_add_curse_info(sidebar_t* sidebar, que_t* curse)
{
	sidebar_entry_t res = {0};

	res.pos = sidebar_get_next_line_pos(sidebar);
	res.name = lva("%s", GetCurseName(curse->ent->atype));
	res.data = lva("%.1fs", curse->time - level.time);

	return res;
}

void layout_generate_entities(const layout_t* layout, sidebar_t* sidebar)
{
	for (int i = 0; i < layout->tracked_count; i++)
	{
		const sidebar_entry_t res = layout_add_entity_info(sidebar, layout->tracked_list[i]);
		sidebar_add_entry(sidebar, res);
	}
}

sidebar_entry_t layout_add_aura_info(sidebar_t* sidebar, que_t* que)
{
	sidebar_entry_t res = {0};

	res.pos = sidebar_get_next_line_pos(sidebar);

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

void layout_generate_misc(edict_t* ent, sidebar_t* sidebar)
{
	if (ent->client->ability_delay > level.time)
	{
		sidebar_entry_t entry = { 0 };
		entry.pos = sidebar_get_next_line_pos(sidebar);
		entry.name = lva("cd");
		entry.data = lva("%.1f", ent->client->ability_delay - level.time);
		sidebar_add_entry(sidebar, entry);
	}

	if (ent->cocoon_time >= level.time) 
	{
		float dt = ent->cocoon_time - level.time;
		sidebar_entry_t res = { 0 };
		res.pos = sidebar_get_next_line_pos(sidebar);
		res.name = lva("cocooned");
		res.data = lva("%.1fs +%.1f%%", dt, (ent->cocoon_factor - 1) * 100.0f);
		sidebar_add_entry(sidebar, res);
	}

	if (ent->fury_time >= level.time)
	{
		float dt = ent->fury_time - level.time;
		sidebar_entry_t res = { 0 };
		res.pos = sidebar_get_next_line_pos(sidebar);
		res.name = lva("furied");
		res.data = lva("%.2fs", dt);
		sidebar_add_entry(sidebar, res);
	}

	if (pregame_time->value > level.time && !trading->value)
	{
		float timeleft = pregame_time->value - level.time;
		sidebar_entry_t entry = { 0 };

		if (timeleft < 10)
		{
			entry.data_style = STYLE_BLINK;
		}
		//else
			//entry.data_style = STYLE_WHITE;
		//gi.dprintf("time left %.1f, set style to %d\n", timeleft, entry.data_style);
		entry.pos = sidebar_get_next_line_pos(sidebar);
		entry.name = lva("pregame");
		entry.data = lva("%.0fs", pregame_time->value - level.time);
		sidebar_add_entry(sidebar, entry);
	}

	if (ent->client->tele_timeout > level.framenum)
	{
		sidebar_entry_t entry = { 0 };
		entry.pos = sidebar_get_next_line_pos(sidebar);
		entry.name = lva("blinkStrike");
		entry.data = lva("%ds", (int)(ent->client->tele_timeout - level.framenum) / 10);
		sidebar_add_entry(sidebar, entry);
	}
}

void layout_generate_curses(edict_t* ent, sidebar_t* sidebar)
{
	for (int i = 0; i < QUE_MAXSIZE; i++)
	{
		que_t* curse = &ent->curses[i];
		sidebar_entry_t res = {0};
		if (!que_valident(curse)) continue;

		res = layout_add_curse_info(sidebar, curse);
		sidebar_add_entry(sidebar, res);
	}

	for (int i = 0; i < QUE_MAXSIZE; i++)
	{
		que_t* aura = &ent->auras[i];
		sidebar_entry_t res = {0};
		if (!que_valident(aura)) continue;

		res = layout_add_aura_info(sidebar, aura);
		sidebar_add_entry(sidebar, res);
	}
}

void layout_reset(layout_t* layout)
{
	memset(layout->layout, 0, MAX_LAYOUT_LEN);
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

	// emit everything except pics. assumption is that entry.pos will update both x and y.
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		sidebar_entry_t* entry = &sidebar->entry[i];

		if (entry->pic.len) continue;

		// first, emit the name string
		layout_apply_pos(layout, entry->pos);
		if (entry->name_style == STYLE_GREEN)
			layout_add_highlight_string(layout, entry->name.str);
		else if (entry->data_style == STYLE_BLINK)
		{
			if (sf2qf(level.framenum) & 2)
				layout_add_string(layout, entry->name.str);
			else
				layout_add_highlight_string(layout, entry->name.str);
		}
		else
			layout_add_string(layout, entry->name.str);
	}

	// doing it this way we save cursor position changes from the 3rd row.
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		sidebar_entry_t* entry = &sidebar->entry[i];
		layout_pos_t npos = entry->pos;

		if (entry->pic.len) continue;

		// now emit the data string
		npos.x += namelen * 8;
		layout_apply_pos(layout, npos);
		if (entry->data_style == STYLE_GREEN)
			layout_add_highlight_string(layout, entry->data.str);
		else if (entry->data_style == STYLE_BLINK)
		{
			if (sf2qf(level.framenum) & 2)
				layout_add_string(layout, entry->data.str);
			else
				layout_add_highlight_string(layout, entry->data.str);
		}
		else
			layout_add_string(layout, entry->data.str);
	}

	// emit pics
	for (int i = 0; i < sidebar->entry_count; i++)
	{
		sidebar_entry_t* entry = &sidebar->entry[i];

		// not a pic? skip
		if (!entry->pic.len) continue;

		layout_apply_pos(layout, entry->pos);
		layout_add_pic_name(layout, entry->pic.str);
	}
}

void layout_generate_tech(edict_t* ent, sidebar_t* sidebar)
{
	sidebar_entry_t res = {0};
	qboolean has_tech = false;

	if (ent->client->pers.inventory[resistance_index])
	{
		res.pic = lva("tech1");
		has_tech = true;
	}

	if (ent->client->pers.inventory[strength_index])
	{
		res.pic = lva("tech2");
		has_tech = true;
	}

	if (ent->client->pers.inventory[regeneration_index])
	{
		res.pic = lva("tech4");
		has_tech = true;
	}

	if (ent->client->pers.inventory[haste_index])
	{
		res.pic = lva("tech3");
		has_tech = true;
	}

	if (has_tech)
	{
		sidebar->y_offset += 16; // skip a little bit
		res.pos = sidebar_get_next_line_pos(sidebar);
		sidebar->y_offset += 16; // skip a little bit

		sidebar_add_entry(sidebar, res);
	}
}

void layout_generate_all(edict_t* ent)
{
	sidebar_t sidebar = {0};

	layout_clean_tracked_entity_list(&ent->client->layout);
	layout_reset(&ent->client->layout);
	layout_generate_misc(ent, &sidebar);
	layout_generate_entities(&ent->client->layout, &sidebar);
	layout_generate_curses(ent, &sidebar);

	layout_generate_tech(ent, &sidebar);

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