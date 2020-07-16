is_pvm = ((cvar_get("pvm", "0") ~= "0") or (cvar_get("invasion", "0") ~= "0"))
is_invasion = (cvar_get("invasion", "0") ~= "0")

useMysqlTablesOnSQLite = 0
UseLuaMaplists = 0

if is_pvm then
    cvar_set("nolag", "1")
	q2dofile("variables_pvm")
else
	cvar_set("nolag", "0")
	q2dofile("variables_pvp")
end

if is_invasion then 
	q2print("Lua: Invasion - lowering xp to 14/monster\n")
	EXP_WORLD_MONSTER = 14
	PVB_BOSS_EXPERIENCE = 400
	PVB_BOSS_CREDITS = 100
else
	q2print("Lua: Non-invasion - Setting xp to 22/monster\n")
	EXP_WORLD_MONSTER = 22
	PVB_BOSS_EXPERIENCE = 1000
	PVB_BOSS_CREDITS = 3000
end

q2print("INFO: nolag is set to " .. cvar_get("nolag", "0") .. ".\n")

--[[ if UsePathfinding == 0 then
	q2print("INFO: Grid pathfinding is disabled.\n")
else
	q2print("INFO: Grid pathfinding is enabled.\n")
end
]]

reloadvars()