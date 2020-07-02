is_pvm = ((cvar_get("pvm", "0") ~= "0") or (cvar_get("invasion", "0") ~= "0"))
is_invasion = (cvar_get("invasion", "0") ~= "0")

UsePathfinding = 1

useMysqlTablesOnSQLite = 0

if is_pvm then
    cvar_set("nolag", "1")
	q2dofile("variables_pvm")
else
	cvar_set("nolag", "0")
	q2dofile("variables_pvp")
end

q2print("INFO: nolag is set to " .. cvar_get("nolag", "0") .. ".\n")

if UsePathfinding == 0 then
	q2print("INFO: Grid pathfinding is disabled.\n")
else
	q2print("INFO: Grid pathfinding is enabled.\n")
end

reloadvars()