classnames = ["CLERIC", "SHAMAN", "NECROMANCER"]
vtypes = ["ARMOR", "POWERCUBES", "HEALTH"]
prefix = ["INITIAL", "LEVELUP"]

sx = []
ss = []
si = []
for vclass in classnames:
    for var in vtypes:
        for p in prefix:
            s = "{}_{}_{}".format(p, var, vclass)
            sx.append('{} = Lua_GetVariable("{}", 0);'.format(s, s))
            ss.append("extern double {};".format(s))
            si.append("double {};".format(s))

for x in sx:
    print (x)

print("")

for x in ss:
    print(x)

print("")

for x in si:
    print(x)