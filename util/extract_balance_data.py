# -*- coding: utf-8 -*-
from glob import glob
from re import compile
from json import dumps

class Damage(object):
    def __init__(self):
        self.damage_start = 0
        self.damage_addon = 0
        self.speed_start = 0
        self.speed_addon = 0
        self.dmg_min_start = None
        self.dmg_min_addon = 0
        self.dmg_max_start = None
        self.dmg_max_addon = 0
        self.cap = None
        self.speed_cap = None
        self.pull_start = 0
        self.pull_addon = 0
        self.count_start = 0
        self.count_addon = 0

    @property
    def is_damage_range(self):
        return self.dmg_min_start is not None and\
               self.dmg_max_start is not None

    @property
    def is_damage_capped(self):
        return self.cap is not None

    def is_speed_capped(self):
        return self.speed_cap is not None

class Ent(object):
    def __init__(self):
        self.health_start = 0
        self.health_addon = 0
        self.power_start = 0
        self.power_addon = 0

# #define xxx number
re_define = compile(r'\#define\s+([a-zA-Z0-9_]+)\s+(-?\d+(\.\d+)?)')

# // xxx.yyy: aaa,bbb,ccc
re_dmgclass = compile(r'//\s*([a-zA-Z.]+)\s*:\s*([a-zA-Z_]+((,[a-zA-Z_]+)+)?)')

# VAR = num
re_luadefine = compile(r'([a-zA-Z0-9_]+)\s*=\s*(\d+(\.\d+)?)')

# (num|def) + (num|def) * whatever;
re_val = compile(r'=\s*((-?(\d+(\.\d+)?)|-?[a-zA-Z_]+)(\s*\+\s*)?)?(((\d+(\.\d+)?)|[a-zA-Z_]+)\s*\*\s*[->.a-zA-Z\[\]]+)?;')


def load_define(defs, line):
    match = re_define.search(line)
    if match:
        # print(match.group(0))
        defs[match.group(1)] = float(match.group(2))

def load_luavar(defs, line):
    match = re_luadefine.search(line)
    if match:
        # print(match.group(0))
        defs[match.group(1)] = float(match.group(2))

def load_dmgclass_value(defs, dmgs, ents, ltype, names, line, filename):
    match = re_val.search(line)
    if match is None:
        print("invalid match in file {}, line \"{}\"".format(filename, line.strip()))
        return

    # print(match.group(0), match.group(1), match.group(5))
    # group 1: base
    # group 2: scaling
    names = names.split(',')
    for ident in names:
        
        try:
            base = float(match.group(2) or 0)
        except ValueError: # not a number
            try:
                base = defs[match.group(2)]
            except KeyError:
                print(line, "errored while trying to find key", match.group(1))
                return

        try:
            scale = float(match.group(7) or 0)
        except ValueError:
            try:
                scale = defs[match.group(7)]
            except KeyError:
                print(line, "errored while trying to find key", match.group(5))
                return

        if ltype in ["dmg", "spd", "cap", "dmgcap", "spdcap", "pull", "dmg.min", "dmg.max", "count"]:
            if not ident in dmgs:
                dmgs[ident] = Damage()
            dmg = dmgs[ident]

        if ltype in ["hlt", "pow"]:
            if not ident in ents:
                ents[ident] = Ent()
            ent = ents[ident]

        if ltype == "dmg":
            dmg.damage_start = base
            dmg.damage_addon = scale
        if ltype == "spd":
            dmg.speed_start = base
            dmg.speed_addon = scale
        if ltype == "cap":
            dmg.cap = base
        if ltype == "dmgcap":
            dmg.cap = base
        if ltype == "spdcap":
            dmg.speed_cap = base
        if ltype == "pull":
            dmg.pull_start = base
            dmg.pull_addon = scale
        if ltype == "dmg.min":
            dmg.dmg_min_start = base
            dmg.dmg_min_addon = scale
        if ltype == "dmg.max":
            dmg.dmg_max_start = base
            dmg.dmg_max_addon = scale
        if ltype == "dmg.count":
            dmg.count_start = base
            dmg.count_addon = scale

        if ltype == "hlt":
            ent.health_start = base
            ent.health_addon = scale
        if ltype == "pow":
            ent.power_start = base
            ent.power_addon = scale


def load_dmgclass(defs, dmgs, ents, line, filename):
    match = re_dmgclass.search(line)
    if match:
        if match.group(1) in ["hlt", "dmg", "spd", "cap", "dmgcap", "spdcap", "pow", "pull", "dmg.min", "dmg.max", "count"]:
            load_dmgclass_value(defs, dmgs, ents, match.group(1), match.group(2), line, filename)

def read_all_c(defs, dmgs, ents):
    srcs = glob("src/**/*.c", recursive=True) + glob("src/**/*.h", recursive=True)

    # exclude library files
    srcs = [x for x in srcs if 'libraries' not in x]

    # read defines first
    print("Reading C defines...")
    for filename in srcs:
        with open(filename, 'r', encoding='utf-8') as file:
            print("CURRENT FILE:", filename, " " * 12)
            for line in file:
                load_define(defs, line)
    print()

    print("Reading tagged balance data...")
    # read damage classes
    for filename in srcs:
        with open(filename, 'r', encoding='utf-8') as file:
            print("CURRENT FILE:", filename, " " * 12)
            for line in file:
                load_dmgclass(defs, dmgs, ents, line, filename)

def read_all_lua(defs, dmgs):
    lua_srcs = glob("lua/variables.lua")
    for filename in lua_srcs:
        with open(filename, 'r') as file:
            for line in file:
                load_luavar(defs, line)

if __name__ == '__main__':
    defs = {}
    dmgs = {}
    ents = {}

    read_all_lua(defs, dmgs)
    read_all_c(defs, dmgs, ents)
    
    dmgstr = dumps(dmgs, indent=4, default=lambda o: o.__dict__)
    entstr = dumps(ents, indent=4, default=lambda o: o.__dict__)

    with open("dmgdata.json", "w") as f:
        f.write(dmgstr)
    with open("entstrdata.json", "w") as f:
        f.write(entstr)