#ifndef V_MAPLIST_H
#define V_MAPLIST_H

#define MAX_VOTERS			(MAX_CLIENTS * 2)
#define MAX_MAPS			128
#define MAX_MAPNAME_LEN		16

#define MAPMODE_PVP			1
#define MAPMODE_PVM			2
#define MAPMODE_DOM			3
#define MAPMODE_CTF			4
#define MAPMODE_FFA			5
#define MAPMODE_INV			6
#define MAPMODE_TRA			7 // vrx chile 2.5 trading mode
#define MAPMODE_INH			8 // vrxchile 2.6 invasion hard mode
#define MAPMODE_VHW			9 // vrxchile 3.0 vortex holywars
#define MAPMODE_TBI			10 // vrxchile 3.4 destroy the spawns
#define MAPMODE_INHE		11 // extended invasion hard mode

#define ML_ROTATE_SEQ          0
#define ML_ROTATE_RANDOM       1
#define ML_ROTATE_NUM_CHOICES  2

//***************************************************************************************
//***************************************************************************************

typedef struct
{
	char name[MAX_MAPNAME_LEN];
	int monsters;
	int min_players, max_players;
} mapdata_t;

typedef struct v_maplist_s
{ 
   int  nummaps;          // number of maps in list
   mapdata_t maps[MAX_MAPS];//4.5
   //char mapnames[MAX_MAPS][MAX_MAPNAME_LEN];
} v_maplist_t;

//***************************************************************************************
//***************************************************************************************

typedef struct votes_s
{
	qboolean	used; // Paril: in "new" system, this means in progress.
	int			mode;
	int			mapindex;
	char		name[24];
	char		ip[16];
} votes_t;

//***************************************************************************************
//***************************************************************************************

#endif