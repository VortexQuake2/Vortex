#define MAX_LINES				21
#define LINE_SPACING			8
#define MENU_DELAY_TIME			0.1 // 3.65 delay time when cycling thru menu (in seconds)

#define MENU_WHITE_CENTERED		20001
#define	MENU_GREEN_CENTERED		20002
#define MENU_GREEN_RIGHT		20003
#define MENU_GREEN_LEFT			20004

// 3.45 menu indexes, used to tell which menu we're on
// note that this isn't necessary unless there are 2 menus
// that are exactly the same (because the compiler treats
// them as the same function)
#define MENU_MASTER_PASSWORD	1
#define MENU_SPECIAL_UPGRADES	2
#define MENU_MULTI_UPGRADE		3
#define MENU_COMBAT_PREFERENCES	4

void menu_add_line (edict_t *ent,char *line,int option);
void menu_clear (edict_t *ent);
void menu_set_handler (edict_t *ent,void (*optionselected)(edict_t *ent,int option));
void menu_set_close_handler(edict_t* ent, void (*onclose)(edict_t* ent));
void menu_close_all (void);

void menu_up (edict_t *ent);
void menu_down (edict_t *ent);
void menu_select (edict_t *ent);

void menu_init (edict_t *ent);
void menu_show (edict_t *ent);
void menu_close (edict_t *ent, qboolean do_close_event);

qboolean menu_can_show (edict_t *ent);
qboolean menu_active (edict_t *ent,int index, void (*optionselected)(edict_t *ent,int option));

typedef struct menumsg_s
{
	char	*msg;
	int		option;
} menumsg_t;

typedef struct menusystem_s
{
	void 		(*optionselected)(edict_t *ent,int option);
	void		(*oldmenuhandler)(edict_t *ent,int option);
	void 		(*onclose)(edict_t* ent);
	qboolean cancel_close_event;
	qboolean 	menu_active;
	qboolean	displaymsg;
	int			oldline;
	menumsg_t	messages[MAX_LINES];
	int 		currentline;
	int			num_of_lines;
	int			menu_index;
} menusystem_t;
