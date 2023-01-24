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

	switch(mode)
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

int v_LoadMapList(int mode)
{
	FILE *fptr;
	v_maplist_t *maplist;
	char filename[256];
	int iterator = 0;
	
	//determine path

	DoMaplistFilename(mode, &filename[0]);

	switch(mode)
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
		gi.dprintf("ERROR in v_LoadMapList(). Incorrect map mode. (%d)\n", mode);
		return 0;
	}

	//gi.dprintf("mode = %d\n", mode);


		if ((fptr = fopen(filename, "r")) != NULL)
		{
			char buf[MAX_INFO_STRING], *s;

			while (fgets(buf, MAX_INFO_STRING, fptr) != NULL)
			{
			    if (iterator >= MAX_MAPS)
                {
			        gi.dprintf("Maplist for mode %d is too big - skipping extra entries. \n", mode);
			        break;
                }

				// tokenize string using comma as separator
				if ((s = strtok(buf, ",")) != NULL)
				{
					// copy map name to list
					strcpy(maplist->maps[iterator].name, s);

					// terminate
					int i = strcspn(s, " \r\n");
					maplist->maps[iterator].name[i] = 0;
				}
				else
				{
					// couldn't find first token, fail
					gi.dprintf("Error loading map file: %s\n", filename);
					maplist->nummaps = 0;
					fclose(fptr);
					return 0;
				}

				// find next token
				if ((s = strtok(NULL, ",")) != NULL)
				{
				    // terminate for atoi
                    s[strcspn(s, " \r\n")] = 0;

					// copy monster value to list
					maplist->maps[iterator].monsters = atoi(s);
				} else {
					maplist->maps[iterator].monsters = 0; // use dm_monsters
				}

				++iterator;
			}
			fclose(fptr);
			maplist->nummaps = iterator;
		}
		else
		{
			gi.dprintf("Error loading map file: %s\n", filename);
			maplist->nummaps = 0;
		}


	return maplist->nummaps;
}