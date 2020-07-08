

How to install/add uniques



1) The file "uniques.csv" must be in a subdirectory called "\uniques" in the player save directory
2) Open the Excel worksheet "Uniques.xls"
3) Edit the Excel file, making sure to stick to the format.
	- Set items will have the same set code
	- The "Set" property of each mod will control how many set items you need to get the bonus
		- 0 = Always there
		- 1 = You need > 1 items equipped
4) SAVE THE FILE
5) Save the file again, using "save as...". Save as COMMA DELIMITED TEXT (*.csv)
6) Done!


Format of the .csv file:

	Type (int)		Type of rune (see type list)
	Untradeable (int)	0 = tradeable item
	Name (str)		name of the unique
	numMods (int)		Number of mods the item has
	setCode (int)		Index of the set. Links all items of the same set together
	mod1.type (int)		Type of mod (ability mod, weapon mod, or none if set to zero)
	mod1.index (int)	Index of mod (ability index OR special weapon code) ex: rg damage = 1800, rg refire = 1801
	mod1.value (int)	Value of mod bonus
	mod1.set (int)		The number of equipped items in-game must be > than this # to get the bonus

** There are some samples of uniques and set items already up!

******************************************************************
List of defined unique item types
	0  - None (don't use 0, or else the rune won't spawn)
	1  - Weapon rune
	2  - Ability rune
	3  - Belt (combo) rune
	9  - Unique weapon rune
	10 - Unique ability rune
	40 - Unique belt (combo) rune
******************************************************************


New uniques will be picked randomly whenever a unique is told to spawn in the game. Enjoy!

- Doomie/Archer (Vortex 3.0)