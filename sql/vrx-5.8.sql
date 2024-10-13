create table if not exists servers
(
    id         int      not null auto_increment,
    name       char(64) not null unique,
    ip         char(64) not null,
    port       int      not null,
    authkey    char(64) not null,
    running    int      not null default 0,
    created_at timestamp         default CURRENT_TIMESTAMP,
    updated_at timestamp         default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
    primary key (id)
);

insert into servers (name, ip, port, authkey)
values ('localhost', 'localhost', 27910, 'password');

create table if not exists abilities
(
    char_idx      int not null references userdata (char_idx) on delete cascade,
    aindex        int not null,
    level         int not null,
    max_level     int not null,
    hard_max      int not null,
    modifier      int not null,
    disable       int not null,
    general_skill int not null,
    primary key (char_idx, aindex)
);

create index ix_abil_char on abilities (char_idx);

create table if not exists prestige
(
    char_idx int not null references userdata (char_idx) on delete cascade,
    pindex   int not null,
    param    int not null,
    level    int not null,
    primary key (char_idx, pindex, param)
);

create index ix_prestige_char on prestige (char_idx);

create table if not exists character_data
(
    char_idx       int not null references userdata (char_idx) on delete cascade,
    respawns       int not null default 0,
    health         int not null default 100,
    maxhealth      int not null default 100,
    armour         int not null default 0,
    maxarmour      int not null default 0,
    nerfme         int not null default 0,
    adminlevel     int not null default 0,
    bosslevel      int not null default 0,
    prestigelevel  int not null default 0,
    prestigepoints int not null default 0,
    primary key (char_idx)
);

create index ix_data_char on character_data (char_idx);

create table if not exists ctf_stats
(
    char_idx      int not null references userdata (char_idx) on delete cascade,
    flag_pickups  int not null default 0,
    flag_captures int not null default 0,
    flag_returns  int not null default 0,
    flag_kills    int not null default 0,
    offense_kills int not null default 0,
    defense_kills int not null default 0,
    assists       int not null default 0,
    primary key (char_idx)
);

create index ix_ctf_char on ctf_stats (char_idx);

create table if not exists game_stats
(
    char_idx         int not null references userdata (char_idx) on delete cascade,
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

create index ix_game_char on game_stats (char_idx);

create table if not exists point_data
(
    char_idx    int not null references userdata (char_idx) on delete cascade,
    exp         int not null default 0,
    exptnl      int not null default 1500,
    level       int not null default 0,
    classnum    int not null default 0,
    skillpoints int not null default 0,
    credits     int not null default 0,
    weap_points int not null default 0,
    resp_weapon int not null default 0,
    tpoints     int not null default 0,
    primary key (char_idx)
);

create index ix_point_char on point_data (char_idx);

create table if not exists runes_meta
(
    char_idx    int      not null references userdata (char_idx) on delete cascade,
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

create index ix_runes_char on runes_meta (char_idx);

create table if not exists runes_mods
(
    char_idx   int not null references userdata (char_idx) on delete cascade,
    rune_index int not null,
    mod_index  int not null,
    type       int null,
    mindex     int null,
    value      int null,
    rset       int null,
    primary key (char_idx, rune_index, mod_index),
    foreign key (char_idx, rune_index)
        references runes_meta (char_idx, rindex)
        on delete cascade
);

create index ix_runes_mods_char on runes_mods (char_idx, rune_index);

create table if not exists stash
(
    char_idx     int not null references userdata (char_idx) on delete cascade,
    lock_char_id int null references userdata (char_idx) on delete set null,
    primary key (char_idx)
);

create index ix_stash_char on stash (char_idx);

create table if not exists stash_runes_meta
(
    char_idx    int      not null references userdata (char_idx) on delete cascade,
    stash_index int      not null check (stash_index >= 0),
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

create index ix_stash_meta_char on stash_runes_meta (char_idx);

create table if not exists stash_runes_mods
(
    char_idx       int not null,
    stash_index    int not null,
    rune_mod_index int not null,
    type           int null,
    mod_index      int null,
    value          int null,
    rset           int null,
    primary key (char_idx, stash_index, rune_mod_index),
    foreign key (char_idx, stash_index)
        references stash_runes_meta (char_idx, stash_index)
        on delete cascade
);

create index ix_stash_mods_char on stash_runes_mods (char_idx, stash_index);

create table if not exists talents
(
    char_idx      int not null references userdata (char_idx) on delete cascade,
    id            int not null,
    upgrade_level int null,
    max_level     int null,
    primary key (char_idx, id)
);

create index ix_talents_char on talents (char_idx);

create table if not exists userdata
(
    char_idx          int      not null auto_increment,
    title             char(24) null,
    playername        char(64) not null unique,
    password          char(24) null,
    email             char(64) null,
    owner             char(24) null,
    member_since      char(30) null,
    last_played       char(30) null,
    playtime_total    int      null,
    playingtime       int      null,
    creator_server_id int      null references servers (id) on delete set null,
    isplaying         int      not null default 0,
    server_id         int      null references servers (id) on delete set null,
    created_at        timestamp         default CURRENT_TIMESTAMP,
    updated_at        timestamp         default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
    primary key (char_idx)
);

create index ix_userdata_char on userdata (char_idx);
create index ix_userdata_player on userdata (playername, char_idx);
create index ix_userdata_server on userdata (server_id);

create table if not exists weapon_meta
(
    char_idx int not null references userdata (char_idx) on delete cascade,
    windex   int not null,
    disable  int not null,
    primary key (char_idx, windex)
);

create index ix_weapon_meta_char on weapon_meta (char_idx);

create table if not exists weapon_mods
(
    char_idx     int not null references userdata (char_idx) on delete cascade,
    weapon_index int not null,
    modindex     int not null,
    level        int not null,
    soft_max     int not null,
    hard_max     int not null,
    primary key (char_idx, weapon_index, modindex)
);

create procedure CharacterExists(IN pname varchar(64), OUT doesexist int)
BEGIN

    SELECT EXISTS (SELECT (char_idx)
                   FROM userdata
                   WHERE userdata.playername = pname)
    INTO doesexist;

END;

create procedure FillNewChar(IN charname varchar(64))
BEGIN

    DECLARE chid INT DEFAULT 0;

    START TRANSACTION;

    -- Give us the character id so we generate this new character!
    INSERT INTO userdata (playername) VALUES (charname);
    SET chid = LAST_INSERT_ID();

    INSERT INTO character_data (char_idx) VALUES (chid);


    INSERT INTO point_data (char_idx) VALUES (chid);
    INSERT INTO game_stats (char_idx) VALUES (chid);
    INSERT INTO ctf_stats (char_idx) VALUES (chid);
    INSERT INTO stash(char_idx) VALUES (chid);
    COMMIT;

END;

create procedure GetCharID(IN pname varchar(64), OUT chidx int)
BEGIN
    SELECT (char_idx) INTO chidx FROM userdata WHERE playername = pname;
END;

create procedure NotifyServerStatus(IN server_key char(64), IN isrunning int)
BEGIN
    START TRANSACTION;
    UPDATE servers SET running = isrunning WHERE authkey = server_key;
    SET @sid = (select s.id
                from servers s
                where s.authkey = server_key);

    update userdata
    set server_id = null,
        isplaying = 0
    where server_id = @sid;
    COMMIT;
end;

create procedure GetCharacterLock(IN chidx int, IN server_key char(64), OUT canplay int)
BEGIN
    proc:
    BEGIN
        START TRANSACTION;
        SET @sid = (select s.id
                    from servers s
                    where s.authkey = server_key);

        if @sid is null then
            SET canplay = 0;
            COMMIT;
            LEAVE proc;
        end if;

        if (select (isplaying) FROM userdata WHERE char_idx = chidx FOR UPDATE) THEN
            SET canplay = 0;
        else
            update userdata
            set isplaying = 1,
                server_id = @sid
            where char_idx = chidx;
            SET canplay = 1;
        END IF;
        COMMIT;
    END proc;
END;

create procedure ResetTables(IN pname varchar(64))
BEGIN

    DECLARE chid INT DEFAULT 0;

    START TRANSACTION;

    SELECT (char_idx) INTO chid FROM userdata WHERE playername = pname;

    DELETE FROM abilities WHERE char_idx = chid;
    DELETE FROM talents WHERE char_idx = chid;
    DELETE FROM runes_meta WHERE char_idx = chid;
    DELETE FROM runes_mods WHERE char_idx = chid;
    DELETE FROM weapon_meta WHERE char_idx = chid;
    DELETE FROM weapon_mods WHERE char_idx = chid;
    DELETE FROM prestige WHERE char_idx = chid;
    COMMIT;

END;

