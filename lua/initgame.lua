is_pvm = ((q2.cvar_get("pvm", "0") ~= "0") or (q2.cvar_get("invasion", "0") ~= "0"))
is_invasion = (q2.cvar_get("invasion", "0") ~= "0")

useMysqlTablesOnSQLite = 0
UseLuaMaplists = 0

if is_pvm then
    q2.cvar_set("nolag", "1")
	q2.dofile("variables_pvm")
else
	q2.cvar_set("nolag", "0")
	q2.dofile("variables_pvp")
end

if is_invasion then 
	q2.print("Lua: Invasion - lowering xp to 18/monster\n")
	EXP_WORLD_MONSTER = 18
	PVB_BOSS_EXPERIENCE = 1600
	PVB_BOSS_CREDITS = 100
else
	q2.print("Lua: Non-invasion - Setting xp to 22/monster\n")
	EXP_WORLD_MONSTER = 22
	PVB_BOSS_EXPERIENCE = 1000
	PVB_BOSS_CREDITS = 3000
end

q2.print("INFO: nolag is set to " .. q2.cvar_get("nolag", "0") .. ".\n")
vrx.reloadvars()

function on_map_change()
	if vrx.get_joined_player_count() >= 6 then
		pvp_fraglimit = 100
	else
		pvp_fraglimit = 50
	end
end