
#define MAX_LAYOUT_LEN 1024

typedef enum
{
	XM_LEFT,
	XM_RIGHT,
	XM_CENTER
} layout_pos_type_x;

typedef enum
{
	YM_TOP,
	YM_BOTTOM,
	YM_CENTER
} layout_pos_type_y;

#define LPF_X 1
#define LPF_Y 2
#define LPF_XY (LPF_X | LPF_Y)

typedef struct layout_pos_s
{
	int x;
	int y;
	layout_pos_type_x x_mode;
	layout_pos_type_y y_mode;
	int pos_flags; // LPF_*
} layout_pos_t;

#define MAX_LAYOUT_TRACKED_ENTS 32

typedef struct layout_s
{
	char layout[MAX_LAYOUT_LEN];
	int current_len;
	layout_pos_t last_pos;
	edict_t* tracked_list[MAX_LAYOUT_TRACKED_ENTS];
	int tracked_count;
	qboolean dirty; // force an update this frame?
} layout_t;

// basic string mgmt
size_t layout_remaining(layout_t* layout);
void layout_clear(layout_t* layout);


// primitives
void layout_add_string(layout_t* layout, char* str);
void layout_add_highlight_string(layout_t* layout, char* str);
void layout_add_centerstring(layout_t* layout, char* str);
void layout_add_centerstring_green(layout_t* layout, char* str);

void layout_add_raw_string(layout_t* layout, char* str);
void layout_add_pic(layout_t* layout, int i);
void layout_add_num(layout_t* layout, int width, int num);

// position mgmt
layout_pos_t layout_set_cursor_x(int x, layout_pos_type_x posx_type);
layout_pos_t layout_set_cursor_y(int y, layout_pos_type_y posy_type);
layout_pos_t layout_set_cursor_xy(
	int x, layout_pos_type_x posx_type, 
	int y, layout_pos_type_y posy_type
);

void layout_apply_pos(layout_t* layout, layout_pos_t pos);

// tracked entities
qboolean layout_add_tracked_entity(layout_t *layout, edict_t* ent);
qboolean layout_remove_tracked_entity(layout_t* layout, edict_t* ent);
void layout_clean_tracked_entity_list(layout_t* layout); // removes ents not in use etc..

// go through all tracked entities, curses, etc... and add them to the layout
void layout_generate_all(edict_t* client);
void layout_send(edict_t* client);