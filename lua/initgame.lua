is_pvm = ((q2.cvar_get("pvm", "0") ~= "0") or (q2.cvar_get("invasion", "0") ~= "0"))
is_invasion = tonumber(q2.cvar_get("invasion", "0"))

useMysqlTablesOnSQLite = 0
UseLuaMaplists = 0

if is_pvm then
    q2.cvar_set("nolag", "1")
	q2.dofile("variables_pvm")
else
	q2.cvar_set("nolag", "0")
	q2.dofile("variables_pvp")
end

if is_invasion == 1 then
	EXP_WORLD_MONSTER = 25
	CREDITS_OTHER_BASE = 5
	PVB_BOSS_EXPERIENCE = 600
	PVB_BOSS_CREDITS = 100
elseif is_invasion == 2 then
	EXP_WORLD_MONSTER = 35
	CREDITS_OTHER_BASE = 5
	PVB_BOSS_EXPERIENCE = 2000
	PVB_BOSS_CREDITS = 200
else
	EXP_WORLD_MONSTER = 50
	CREDITS_OTHER_BASE = 10
	PVB_BOSS_EXPERIENCE = 1200
	PVB_BOSS_CREDITS = 3000
end

q2.print("INFO: nolag is set to " .. q2.cvar_get("nolag", "0") .. ".\n")
vrx.reloadvars()

pvp_fraglimit = 75

function on_map_change()
	if vrx.get_joined_player_count() >= 6 then
		pvp_fraglimit = 100
	else
		pvp_fraglimit = 50
	end
end
