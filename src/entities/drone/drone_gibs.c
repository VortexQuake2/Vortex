#include "g_local.h"

extern mmove_t infantry_move_death3;

typedef struct drone_gib_s {
	int count;
	const char *model;
	int type;
	float scale;
	int frame;
} drone_gib_t;

typedef struct drone_gib_list_s {
	const drone_gib_t *gibs;
	int count;
} drone_gib_list_t;

#define HGIB(name, type) { 1, name, type, 1.0f, 0 }
#define HGIB_N(count, name, type) { count, name, type, 1.0f, 0 }
#define HGIB_SCALE(name, scale, type) { 1, name, type, scale, 0 }
#define HGIB_FRAME(name, frame, type) { 1, name, type, 1.0f, frame }
#define HGIB_COUNT(list) ((int)(sizeof(list) / sizeof((list)[0])))
#define HGIB_LIST(list) { list, HGIB_COUNT(list) }

static const drone_gib_t gibs_soldier[] = {
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/bone2/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/soldier/gibs/arm.md2", GIB_SKINNED),
	HGIB("models/monsters/soldier/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/soldier/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/soldier/gibs/head.md2", GIB_HEAD | GIB_SKINNED),
};

static const drone_gib_t gibs_gunner[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/gunner/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/gunner/gibs/garm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gunner/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gunner/gibs/foot.md2", GIB_SKINNED),
	HGIB("models/monsters/gunner/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_guncmdr[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/gear/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/gunner/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/gunner/gibs/garm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gunner/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gunner/gibs/foot.md2", GIB_SKINNED),
	HGIB("models/monsters/gunner/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_chick[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/bitch/gibs/arm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/bitch/gibs/foot.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/bitch/gibs/tube.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/bitch/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/bitch/gibs/head.md2", GIB_HEAD | GIB_SKINNED),
};

static const drone_gib_t gibs_brain[] = {
	HGIB("models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/monsters/brain/gibs/arm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/brain/gibs/boot.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/brain/gibs/pelvis.md2", GIB_SKINNED),
	HGIB("models/monsters/brain/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/brain/gibs/door.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/brain/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_medic[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/medic/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/medic/gibs/leg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/medic/gibs/hook.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/medic/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/medic/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_mutant[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(4, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/monsters/mutant/gibs/hand.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/mutant/gibs/foot.md2", GIB_SKINNED),
	HGIB("models/monsters/mutant/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/mutant/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_parasite[] = {
	HGIB("models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/parasite/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/parasite/gibs/bleg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/parasite/gibs/fleg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/parasite/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_berserk[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/gear/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/berserk/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/berserk/gibs/hammer.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/berserk/gibs/thigh.md2", GIB_SKINNED),
	HGIB("models/monsters/berserk/gibs/head.md2", GIB_HEAD | GIB_SKINNED),
};

static const drone_gib_t gibs_gladiator[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/monsters/gladiatr/gibs/thigh.md2", GIB_SKINNED),
	HGIB("models/monsters/gladiatr/gibs/larm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gladiatr/gibs/rarm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/gladiatr/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/gladiatr/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_infantry[] = {
	HGIB("models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/infantry/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/infantry/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/infantry/gibs/foot.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/infantry/gibs/arm.md2", GIB_SKINNED),
	HGIB("models/monsters/infantry/gibs/head.md2", GIB_HEAD | GIB_SKINNED),
};

static const drone_gib_t gibs_tank[] = {
	HGIB("models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/objects/gibs/gear/tris.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/tank/gibs/foot.md2", GIB_SKINNED | GIB_METALLIC),
	HGIB_N(2, "models/monsters/tank/gibs/thigh.md2", GIB_SKINNED | GIB_METALLIC),
	HGIB("models/monsters/tank/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/tank/gibs/head.md2", GIB_HEAD | GIB_SKINNED),
};

static const drone_gib_t gibs_flyer[] = {
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/flyer/gibs/base.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/flyer/gibs/gun.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/flyer/gibs/wing.md2", GIB_SKINNED),
	HGIB("models/monsters/flyer/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_floater[] = {
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/float/gibs/piece.md2", GIB_SKINNED),
	HGIB("models/monsters/float/gibs/gun.md2", GIB_SKINNED),
	HGIB("models/monsters/float/gibs/base.md2", GIB_SKINNED),
	HGIB("models/monsters/float/gibs/jar.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_hover[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/hover/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/hover/gibs/ring.md2", GIB_SKINNED | GIB_METALLIC),
	HGIB_N(2, "models/monsters/hover/gibs/foot.md2", GIB_SKINNED),
	HGIB("models/monsters/hover/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_shambler[] = {
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/chest/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/head2/tris.md2", GIB_HEAD),
};

static const drone_gib_t gibs_arachnid[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB("models/monsters/gunner/gibs/chest.md2", GIB_METALLIC),
	HGIB("models/monsters/gunner/gibs/garm.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/gladiatr/gibs/rarm.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/gunner/gibs/foot.md2", GIB_METALLIC),
	HGIB("models/monsters/gunner/gibs/head.md2", GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_gekk[] = {
	HGIB("models/objects/gekkgib/pelvis/tris.md2", GIB_ACID),
	HGIB_N(2, "models/objects/gekkgib/arm/tris.md2", GIB_ACID),
	HGIB("models/objects/gekkgib/torso/tris.md2", GIB_ACID),
	HGIB("models/objects/gekkgib/claw/tris.md2", GIB_ACID),
	HGIB_N(2, "models/objects/gekkgib/leg/tris.md2", GIB_ACID),
	HGIB("models/objects/gekkgib/head/tris.md2", GIB_ACID | GIB_HEAD),
};

static const drone_gib_t gibs_stalker[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/stalker/gibs/bodya.md2", GIB_SKINNED),
	HGIB("models/monsters/stalker/gibs/bodyb.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/stalker/gibs/claw.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/stalker/gibs/leg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/stalker/gibs/foot.md2", GIB_SKINNED),
	HGIB("models/monsters/stalker/gibs/head.md2", GIB_SKINNED | GIB_HEAD),
};

static const drone_gib_t gibs_fixbot[] = {
	HGIB("models/objects/gibs/sm_metal/tris.md2", GIB_ACID),
	HGIB("models/objects/gibs/gear/tris.md2", GIB_METALLIC),
};

static const drone_gib_t gibs_supertank[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/boss1/gibs/cgun.md2", GIB_SKINNED | GIB_METALLIC),
	HGIB("models/monsters/boss1/gibs/chest.md2", GIB_SKINNED),
	HGIB("models/monsters/boss1/gibs/core.md2", GIB_SKINNED),
	HGIB("models/monsters/boss1/gibs/ltread.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss1/gibs/rgun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss1/gibs/rtread.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss1/gibs/tube.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss1/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_boss2[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/boss2/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/boss2/gibs/chaingun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/cpu.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/engine.md2", GIB_SKINNED),
	HGIB("models/monsters/boss2/gibs/rocket.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/spine.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/boss2/gibs/wing.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/larm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/rarm.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_SCALE("models/monsters/boss2/gibs/larm.md2", 2.0f, GIB_SKINNED | GIB_UPRIGHT),
	HGIB_SCALE("models/monsters/boss2/gibs/rarm.md2", 2.0f, GIB_SKINNED | GIB_UPRIGHT),
	HGIB_SCALE("models/monsters/boss2/gibs/larm.md2", 1.35f, GIB_SKINNED | GIB_UPRIGHT),
	HGIB_SCALE("models/monsters/boss2/gibs/rarm.md2", 1.35f, GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss2/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_jorg[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/boss3/jorg/gibs/chest.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/boss3/jorg/gibs/foot.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/boss3/jorg/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/boss3/jorg/gibs/thigh.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/boss3/jorg/gibs/spine.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(4, "models/monsters/boss3/jorg/gibs/tube.md2", GIB_SKINNED),
	HGIB_N(6, "models/monsters/boss3/jorg/gibs/spike.md2", GIB_SKINNED),
	HGIB("models/monsters/boss3/jorg/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_makron[] = {
	HGIB("models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(4, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/objects/gibs/gear/tris.md2", GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_carrier[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/carrier/gibs/base.md2", GIB_SKINNED),
	HGIB("models/monsters/carrier/gibs/chest.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/carrier/gibs/gl.md2", GIB_SKINNED),
	HGIB("models/monsters/carrier/gibs/lcg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/carrier/gibs/lwing.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/carrier/gibs/rcg.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB("models/monsters/carrier/gibs/rwing.md2", GIB_SKINNED | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/carrier/gibs/spawner.md2", GIB_SKINNED),
	HGIB_N(2, "models/monsters/carrier/gibs/thigh.md2", GIB_SKINNED),
	HGIB("models/monsters/carrier/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_guardian[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(4, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib1.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib2.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib3.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib4.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib5.md2", GIB_METALLIC),
	HGIB_N(2, "models/monsters/guardian/gib6.md2", GIB_METALLIC),
	HGIB("models/monsters/guardian/gib7.md2", GIB_METALLIC | GIB_HEAD),
};

static const drone_gib_t gibs_widow[] = {
	HGIB_N(2, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB("models/monsters/blackwidow/gib1/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/blackwidow/gib2/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/blackwidow/gib3/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/blackwidow/gib4/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
};

static const drone_gib_t gibs_widow2[] = {
	HGIB_N(2, "models/objects/gibs/bone/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_meat/tris.md2", GIB_ORGANIC),
	HGIB_N(3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC),
	HGIB_N(3, "models/monsters/blackwidow2/gib1/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB_N(3, "models/monsters/blackwidow2/gib2/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/blackwidow2/gib3/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/monsters/blackwidow2/gib4/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB_N(2, "models/monsters/blackwidow/gib3/tris.md2", GIB_METALLIC | GIB_UPRIGHT),
	HGIB("models/objects/gibs/chest/tris.md2", GIB_ORGANIC),
	HGIB("models/objects/gibs/head2/tris.md2", GIB_HEAD),
};

static const drone_gib_t gibs_rogue_turret[] = {
	HGIB_N(4, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS),
	HGIB_FRAME("models/monsters/turret/tris.md2", 14, GIB_SKINNED | GIB_METALLIC | GIB_HEAD | GIB_DEBRIS),
};

static const drone_gib_list_t list_soldier = HGIB_LIST(gibs_soldier);
static const drone_gib_list_t list_gunner = HGIB_LIST(gibs_gunner);
static const drone_gib_list_t list_guncmdr = HGIB_LIST(gibs_guncmdr);
static const drone_gib_list_t list_chick = HGIB_LIST(gibs_chick);
static const drone_gib_list_t list_brain = HGIB_LIST(gibs_brain);
static const drone_gib_list_t list_medic = HGIB_LIST(gibs_medic);
static const drone_gib_list_t list_mutant = HGIB_LIST(gibs_mutant);
static const drone_gib_list_t list_parasite = HGIB_LIST(gibs_parasite);
static const drone_gib_list_t list_berserk = HGIB_LIST(gibs_berserk);
static const drone_gib_list_t list_gladiator = HGIB_LIST(gibs_gladiator);
static const drone_gib_list_t list_infantry = HGIB_LIST(gibs_infantry);
static const drone_gib_list_t list_tank = HGIB_LIST(gibs_tank);
static const drone_gib_list_t list_flyer = HGIB_LIST(gibs_flyer);
static const drone_gib_list_t list_floater = HGIB_LIST(gibs_floater);
static const drone_gib_list_t list_hover = HGIB_LIST(gibs_hover);
static const drone_gib_list_t list_shambler = HGIB_LIST(gibs_shambler);
static const drone_gib_list_t list_arachnid = HGIB_LIST(gibs_arachnid);
static const drone_gib_list_t list_gekk = HGIB_LIST(gibs_gekk);
static const drone_gib_list_t list_stalker = HGIB_LIST(gibs_stalker);
static const drone_gib_list_t list_fixbot = HGIB_LIST(gibs_fixbot);
static const drone_gib_list_t list_supertank = HGIB_LIST(gibs_supertank);
static const drone_gib_list_t list_boss2 = HGIB_LIST(gibs_boss2);
static const drone_gib_list_t list_jorg = HGIB_LIST(gibs_jorg);
static const drone_gib_list_t list_makron = HGIB_LIST(gibs_makron);
static const drone_gib_list_t list_carrier = HGIB_LIST(gibs_carrier);
static const drone_gib_list_t list_guardian = HGIB_LIST(gibs_guardian);
static const drone_gib_list_t list_widow = HGIB_LIST(gibs_widow);
static const drone_gib_list_t list_widow2 = HGIB_LIST(gibs_widow2);
static const drone_gib_list_t list_rogue_turret = HGIB_LIST(gibs_rogue_turret);

static const drone_gib_list_t all_drone_gib_lists[] = {
	HGIB_LIST(gibs_soldier),
	HGIB_LIST(gibs_gunner),
	HGIB_LIST(gibs_guncmdr),
	HGIB_LIST(gibs_chick),
	HGIB_LIST(gibs_brain),
	HGIB_LIST(gibs_medic),
	HGIB_LIST(gibs_mutant),
	HGIB_LIST(gibs_parasite),
	HGIB_LIST(gibs_berserk),
	HGIB_LIST(gibs_gladiator),
	HGIB_LIST(gibs_infantry),
	HGIB_LIST(gibs_tank),
	HGIB_LIST(gibs_flyer),
	HGIB_LIST(gibs_floater),
	HGIB_LIST(gibs_hover),
	HGIB_LIST(gibs_shambler),
	HGIB_LIST(gibs_arachnid),
	HGIB_LIST(gibs_gekk),
	HGIB_LIST(gibs_stalker),
	HGIB_LIST(gibs_fixbot),
	HGIB_LIST(gibs_supertank),
	HGIB_LIST(gibs_boss2),
	HGIB_LIST(gibs_jorg),
	HGIB_LIST(gibs_makron),
	HGIB_LIST(gibs_carrier),
	HGIB_LIST(gibs_guardian),
	HGIB_LIST(gibs_widow),
	HGIB_LIST(gibs_widow2),
	HGIB_LIST(gibs_rogue_turret),
};

static const drone_gib_list_t *vrx_get_drone_gibs(edict_t *self)
{
	switch (self->mtype)
	{
	case M_SOLDIERLT:
	case M_SOLDIER:
	case M_SOLDIERSS:
	case M_SOLDIER_RIPPER:
	case M_SOLDIER_BLUEBLASTER:
	case M_SOLDIER_LASER:
		return &list_soldier;
	case M_GUNNER:
	case M_HEAVY_GUNNER:
		return &list_gunner;
	case M_GUNCMDR:
		return &list_guncmdr;
	case M_CHICK:
	case M_CHICK_HEAT:
		return &list_chick;
	case M_BRAIN:
		return &list_brain;
	case M_MEDIC:
	case M_MEDIC_COMMANDER:
		return &list_medic;
	case M_MUTANT:
	case M_REDMUTANT:
		return &list_mutant;
	case M_PARASITE:
		return &list_parasite;
	case M_BERSERK:
		return &list_berserk;
	case M_GLADIATOR:
	case M_GLADB:
	case M_GLADC:
		return &list_gladiator;
	case M_INFANTRY:
	case M_ENFORCER:
		return &list_infantry;
	case M_TANK:
	case M_TANK_N64:
	case M_COMMANDER:
	case M_RUNNERTANK:
		return &list_tank;
	case M_FLYER:
		return &list_flyer;
	case M_FLOATER:
		return &list_floater;
	case M_HOVER:
	case M_DAEDALUS:
		return &list_hover;
	case M_SHAMBLER:
		return &list_shambler;
	case M_ARACHNID_PLASMA:
	case M_ARACHNID_HEAT:
	case M_ARACHNID:
		return &list_arachnid;
	case M_GEKK:
		return &list_gekk;
	case M_STALKER:
		return &list_stalker;
	case M_FIXBOT:
	case M_FIXBOT_BOSS:
		return &list_fixbot;
	case M_SUPERTANK:
	case M_JANITOR:
	case M_BOSS5:
		return &list_supertank;
	case M_BOSS2:
	case M_BOSS2_SMALL:
		return &list_boss2;
	case M_JORG:
		return &list_jorg;
	case M_MAKRON:
		return &list_makron;
	case M_CARRIER:
		return &list_carrier;
	case M_GUARDIAN:
	case M_MINIGUARDIAN:
		return &list_guardian;
	case M_WIDOW:
		return &list_widow;
	case M_WIDOW2:
		return &list_widow2;
	case M_ROGUE_TURRET:
		return &list_rogue_turret;
	default:
		return NULL;
	}
}

static int vrx_get_drone_gib_skinnum(edict_t *self)
{
	if (!self)
		return 0;

	if (self->s.renderfx & RF_CUSTOMSKIN)
		return 0;

	if (self->mtype == M_ROGUE_TURRET)
		return self->s.skinnum;

	return self->s.skinnum / 2;
}

static const char *vrx_get_drone_gib_model(edict_t *self, const drone_gib_t *gib)
{
	if ((self->mtype == M_INFANTRY || self->mtype == M_ENFORCER) && (gib->type & GIB_HEAD) &&
		!strcmp(gib->model, "models/monsters/infantry/gibs/head.md2") &&
		self->monsterinfo.currentmove != &infantry_move_death3)
		return "models/objects/gibs/sm_meat/tris.md2";

	return gib->model;
}

static void vrx_throw_drone_gib_list(edict_t *self, int damage, const drone_gib_list_t *list)
{
	edict_t *gib;
	int gib_skinnum = vrx_get_drone_gib_skinnum(self);
	float self_scale = self->s.scale ? self->s.scale : 1.0f;
	const char *model;
	int i;
	int n;

	for (i = 0; i < list->count; i++)
	{
		model = vrx_get_drone_gib_model(self, &list->gibs[i]);

		for (n = 0; n < list->gibs[i].count; n++)
		{
			gib = ThrowGibEx(self, (char *)model, damage, list->gibs[i].type,
				list->gibs[i].scale * self_scale);
			if (gib)
			{
				if (list->gibs[i].type & GIB_SKINNED)
					gib->s.skinnum = gib_skinnum;
				if (list->gibs[i].frame)
					gib->s.frame = list->gibs[i].frame;
			}
		}
	}
}

void vrx_precache_drone_gibs(void)
{
	int i;
	int j;

	for (i = 0; i < HGIB_COUNT(all_drone_gib_lists); i++)
	{
		for (j = 0; j < all_drone_gib_lists[i].count; j++)
			gi.modelindex((char *)all_drone_gib_lists[i].gibs[j].model);
	}
	gi.modelindex("models/monsters/tank/gibs/barm.md2");
}

qboolean vrx_throw_drone_gibs(edict_t *self, int damage)
{
	const drone_gib_list_t *list;

	if (!self || !self->inuse || nolag->value)
		return false;
	if (!vrx_spawn_nonessential_ent(self->s.origin))
		return false;

	list = vrx_get_drone_gibs(self);
	if (!list)
		return false;

	vrx_throw_drone_gib_list(self, damage, list);

	if ((self->mtype == M_TANK || self->mtype == M_TANK_N64 || self->mtype == M_COMMANDER || self->mtype == M_RUNNERTANK) && !self->style)
	{
		edict_t *arm = ThrowGibEx(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT,
			self->s.scale ? self->s.scale : 1.0f);
		if (arm)
			arm->s.skinnum = vrx_get_drone_gib_skinnum(self);
	}

	return true;
}

void vrx_drop_tank_death_arm(edict_t *self, int damage)
{
	edict_t *arm;
	vec3_t forward;
	vec3_t right;
	vec3_t up;

	if (!self || self->style)
		return;

	self->style = 1;

	if (nolag->value || !vrx_spawn_nonessential_ent(self->s.origin))
		return;

	AngleVectors(self->s.angles, forward, right, up);
	arm = ThrowGibEx(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT,
		self->s.scale ? self->s.scale : 1.0f);
	if (!arm)
		return;

	arm->s.skinnum = vrx_get_drone_gib_skinnum(self);
	VectorMA(self->s.origin, -16, right, arm->s.origin);
	VectorMA(arm->s.origin, 23, up, arm->s.origin);
	VectorCopy(arm->s.origin, arm->s.old_origin);
	VectorScale(up, 100, arm->velocity);
	VectorMA(arm->velocity, -120, right, arm->velocity);
	VectorCopy(self->s.angles, arm->s.angles);
	arm->s.angles[ROLL] = -90;
	arm->avelocity[0] = crandom() * 15;
	arm->avelocity[1] = crandom() * 15;
	arm->avelocity[2] = 180;
	gi.linkentity(arm);
}

#undef HGIB
#undef HGIB_N
#undef HGIB_SCALE
#undef HGIB_FRAME
#undef HGIB_COUNT
#undef HGIB_LIST
