create table if not exists abilities
(
    char_idx      int not null,
    aindex        int not null,
    level         int not null,
    max_level     int not null,
    hard_max      int not null,
    modifier      int not null,
    disable       int not null,
    general_skill int not null,
    primary key (char_idx, aindex)
);

create table if not exists prestige
(
    char_idx      int not null,
    pindex        int not null,
    param         int not null,
    level         int not null,
    primary key (char_idx, pindex, param)
);

create table if not exists character_data
(
    char_idx   int not null,
    respawns   int not null default 0,
    health     int not null default 100,
    maxhealth  int not null default 100,
    armour     int not null default 0,
    maxarmour  int not null default 0,
    nerfme     int not null default 0,
    adminlevel int not null default 0,
    bosslevel  int not null default 0,
    prestigelevel int not null default 0,
    prestigepoints int not null default 0,
    primary key (char_idx)
);

create table if not exists ctf_stats
(
    char_idx      int not null,
    flag_pickups  int not null default 0,
    flag_captures int not null default 0,
    flag_returns  int not null default 0,
    flag_kills    int not null default 0,
    offense_kills int not null default 0,
    defense_kills int not null default 0,
    assists       int not null default 0,
    primary key (char_idx)
);

create table if not exists game_stats
(
    char_idx         int not null,
    shots            int not null default 0,
    shots_hit        int not null default 0,
    frags            int not null default 0,
    fragged          int not null default 0,
    num_sprees       int not null default 0,
    max_streak       int not null default 0,
    spree_wars       int not null default 0,
    broken_sprees    int not null default 0,
    broken_spreewars int not null default 0,
    suicides         int not null default 0,
    teleports        int not null default 0,
    num_2fers        int not null default 0,
    primary key (char_idx)
);

create table if not exists point_data
(
    char_idx    int not null,
    exp         int not null,
    exptnl      int not null,
    level       int not null default 0,
    classnum    int not null default 0,
    skillpoints int not null default 0,
    credits     int not null default 0,
    weap_points int not null default 0,
    resp_weapon int not null default 0,
    tpoints     int not null default 0,
    primary key (char_idx)
);

create table if not exists runes_meta
(
    char_idx    int      not null,
    rindex      int      not null,
    itemtype    int      null,
    itemlevel   int      null,
    quantity    int      null,
    untradeable int      null,
    id          char(16) null,
    name        char(24) null,
    nummods     int      null,
    setcode     int      null,
    classnum    int      null,
    primary key (char_idx, rindex)
);

create table if not exists runes_mods
(
    char_idx   int not null,
    rune_index int not null,
    mod_index  int not null,
    type       int null,
    mindex     int null,
    value      int null,
    rset       int null,
    primary key (char_idx, rune_index, mod_index)
);

create table if not exists stash
(
    char_idx     int not null,
    lock_char_id int null,
    primary key (char_idx)
);

create table if not exists stash_runes_meta
(
    char_idx    int      not null,
    stash_index int      not null,
    itemtype    int      null,
    itemlevel   int      null,
    quantity    int      null,
    untradeable int      null,
    id          char(16) null,
    name        char(24) null,
    nummods     int      null,
    setcode     int      null,
    classnum    int      null,
    primary key (char_idx, stash_index)
);

create table if not exists stash_runes_mods
(
    char_idx       int not null,
    stash_index    int not null,
    rune_mod_index int not null,
    type           int null,
    mod_index      int null,
    value          int null,
    rset           int null,
    primary key (char_idx, stash_index, rune_mod_index)
);

create table if not exists talents
(
    char_idx      int not null,
    id            int not null,
    upgrade_level int null,
    max_level     int null,
    primary key (char_idx, id)
);

create table if not exists userdata
(
    char_idx       int      not null,
    title          char(24) null,
    playername     char(64) null,
    password       char(24) null,
    email          char(64) null,
    owner          char(24) null,
    member_since   char(30) null,
    last_played    char(30) null,
    playtime_total int      null,
    playingtime    int      null,
    isplaying      int      null,
    primary key (char_idx)
);

create table if not exists weapon_meta
(
    char_idx int null,
    windex   int null,
    disable  int null
);

create table if not exists weapon_mods
(
    char_idx     int null,
    weapon_index int null,
    modindex     int null,
    level        int null,
    soft_max     int null,
    hard_max     int null
);

create
     procedure CharacterExists(IN pname varchar(64), OUT doesexist int)
BEGIN

    SELECT EXISTS (
                   SELECT (char_idx) FROM userdata
                   WHERE userdata.playername = pname
               ) INTO doesexist;

END;

create
     procedure FillNewChar(IN charname varchar(64))
BEGIN

    DECLARE chid INT DEFAULT 0;

    START TRANSACTION;

    -- Give us the character id so we generate this new character!
    SELECT COUNT(char_idx) INTO chid FROM character_data WHERE char_idx IS NOT NULL;

    INSERT INTO character_data (char_idx) VALUES(chid);

    INSERT INTO userdata (char_idx, playername) VALUES (chid, charname);
    INSERT INTO point_data (char_idx) VALUES (chid);
    INSERT INTO game_stats (char_idx) VALUES (chid);
    INSERT INTO ctf_stats (char_idx) VALUES (chid);
    INSERT INTO stash(char_idx) VALUES (chid);

    COMMIT;

END;

create
     procedure GetCharID(IN pname varchar(64), OUT chidx int)
BEGIN
    SELECT (char_idx) INTO chidx FROM userdata WHERE playername = pname;
END;

create
     procedure GetCharacterLock(IN chidx int, OUT canplay int)
BEGIN
    START TRANSACTION;

    if (select (isplaying) FROM userdata WHERE char_idx = chidx FOR UPDATE) THEN
        SET canplay = 0;
    else
        update userdata set isplaying = 1 where char_idx = chidx;
        SET canplay = 1;
    END IF;

    COMMIT;
END;

create
     procedure ResetTables(IN pname varchar(64))
BEGIN

    DECLARE chid INT DEFAULT 0;

    START TRANSACTION;

    SELECT (char_idx) INTO chid FROM userdata WHERE playername=pname;

    DELETE FROM abilities WHERE char_idx = chid;
    DELETE FROM talents WHERE char_idx = chid;
    DELETE FROM runes_meta WHERE char_idx = chid;
    DELETE FROM runes_mods WHERE char_idx = chid;
    DELETE FROM weapon_meta WHERE char_idx = chid;
    DELETE FROM weapon_mods WHERE char_idx = chid;

    COMMIT;

END;

