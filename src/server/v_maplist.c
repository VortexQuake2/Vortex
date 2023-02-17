#include "g_local.h"

v_maplist_t maplist_PVP;
v_maplist_t maplist_DOM;
v_maplist_t maplist_PVM;
v_maplist_t	maplist_CTF;
v_maplist_t maplist_FFA;
v_maplist_t maplist_INV;
v_maplist_t maplist_TRA;
v_maplist_t maplist_INH;
v_maplist_t maplist_VHW;
v_maplist_t maplist_TBI;

void DoMaplistFilename(int mode, char* filename)
{
	sprintf(filename, "%s/settings/maplists/", game_path->string);

	switch (mode)
	{
	case MAPMODE_PVP:
		strcat(filename, "maplist_PVP.txt");
		break;
	case MAPMODE_DOM:
		strcat(filename, "maplist_DOM.txt");
		break;
	case MAPMODE_PVM:
		strcat(filename, "maplist_PVM.txt");
		break;
	case MAPMODE_CTF:
		strcat(filename, "maplist_CTF.txt");
		break;
	case MAPMODE_FFA:
		strcat(filename, "maplist_FFA.txt");
		break;
	case MAPMODE_INV:
		strcat(filename, "maplist_INV.txt");
		break;
	case MAPMODE_TRA:
		strcat(filename, "maplist_TRA.txt");
		break;
	case MAPMODE_INH:
		strcat(filename, "maplist_INH.txt");
		break;
	case MAPMODE_VHW:
		strcat(filename, "maplist_VHW.txt");
		break;
	case MAPMODE_TBI:
		strcat(filename, "maplist_TBI.txt");
		break;
	}
}

int vrx_load_map_list(int mode)
{
	v_maplist_t* maplist;
	char filename[256];
	int iterator = 0;

	//determine path

	DoMaplistFilename(mode, &filename[0]);

	switch (mode)
	{
	case MAPMODE_PVP:
		maplist = &maplist_PVP;
		break;
	case MAPMODE_DOM:
		maplist = &maplist_DOM;
		break;
	case MAPMODE_PVM:
		maplist = &maplist_PVM;
		break;
	case MAPMODE_CTF:
		maplist = &maplist_CTF;
		break;
	case MAPMODE_FFA:
		maplist = &maplist_FFA;
		break;
	case MAPMODE_INV:
		maplist = &maplist_INV;
		break;
	case MAPMODE_TRA:
		maplist = &maplist_TRA;
		break;
	case MAPMODE_INH:
		maplist = &maplist_INH;
		break;
	case MAPMODE_VHW:
		maplist = &maplist_VHW;
		break;
	case MAPMODE_TBI:
		maplist = &maplist_TBI;
		break;
	default:
		gi.dprintf("ERROR in vrx_load_map_list(). Incorrect map mode. (%d)\n", mode);
		return 0;
	}

	//gi.dprintf("mode = %d\n", mode);


	FILE* fptr = fopen(filename, "r");
	if (fptr == NULL)
	{
		gi.dprintf("Error loading map file: %s\n", filename);
		maplist->nummaps = 0;
		return 0;
	}

	char buf[MAX_INFO_STRING];

	while (fgets(buf, MAX_INFO_STRING, fptr) != NULL)
	{
		if (iterator >= MAX_MAPS)
		{
			gi.dprintf("Maplist for mode %d is too big - skipping extra entries. \n", mode);
			break;
		}

		char* s = buf;

		// skip spaces
		s += strspn(s, " \r\n\t");

		// remove comments
		char* dismiss = strstr(s, "//");
		if (dismiss) // substring found!
			dismiss[0] = 0;

		// tokenize string using comma as separator
		const size_t len = strlen(s);
		int column = 0;

		if (len == 0) // empty line
			continue;

		const char* end = buf + len;
		do
		{
			char buf_item[MAX_INFO_STRING];

			// left trim
			const size_t ltrim_index = strspn(s, " \r\n\t");
			s += ltrim_index;

			// skip to comma
			const size_t comma_pos = strcspn(s, ",");
			strncpy(buf_item, s, comma_pos);
			buf_item[comma_pos] = 0;

			// right trim
			// no entry is supposed to have any spaces, so something like "q2dm1  20  "
			// will make buf_item "q2dm1" instead of "q2dm1  20". This is fine.
			const size_t rtrim_index = strcspn(buf_item, " \r\n\t");
			buf_item[rtrim_index] = 0; // null terminate

			// advance string
			// skip current column ',' marker.
			s += comma_pos;
			if (s[0] == ',') s++; 

			// sanity check and parse
			if (strlen(buf_item) > 0)
			{
				switch (column)
				{
				case 0:
					strcpy(maplist->maps[iterator].name, buf_item);
					break;
				case 1:
					// fixme: use strtol to report errors
					maplist->maps[iterator].monsters = atoi(buf_item);
					break;
				case 2:
					maplist->maps[iterator].min_players = atoi(buf_item);
					break;
				case 3:
					maplist->maps[iterator].max_players = atoi(buf_item);
					break;
				default:
					gi.dprintf("unrecognized column at line %d loading mode %d\n", iterator, mode);
					break;
				}
			}

			column++;
		} while (s < end);

		++iterator;
	}

	fclose(fptr);
	maplist->nummaps = iterator;


	return maplist->nummaps;
}