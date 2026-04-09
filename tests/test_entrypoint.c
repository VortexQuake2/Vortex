#include "mock.h"
#include "../src/g_local.h"
#include "munit.h"

#ifndef WIN32
#include <execinfo.h>
#include <signal.h>
void sigsegv_handler(int sig) {
    void *array[10];


    size_t size = backtrace(array, 10);
    backtrace_symbols_fd(array, size, 2);
    exit(1);
}
#endif

void tests_init() {
    mock_init();
    vrx_init_ability_list();

#ifndef WIN32
    signal(SIGSEGV, sigsegv_handler);
#endif
}

void* setup_cvars(const MunitParameter params[],  void* userdata) {
    start_level = mock_cvar("start_level", "1");
    generalabmode = mock_cvar("generalabmmode", "1");
    ctf = mock_cvar("ctf", "0");
    ctf_enable_balanced_fc = mock_cvar("ctf_enable_balanced_fc", "0");
    vwep = mock_cvar("vwep", "0");
    // mock_cvar("levelup_threshold", "50");

    return NULL;
}

void cleanup_cvars(void* fixture) {
    mock_free_cvar(start_level);
    mock_free_cvar(generalabmode);
    mock_free_cvar(ctf);
    mock_free_cvar(vwep);
    mock_free_cvar(ctf_enable_balanced_fc);
}

MunitResult test_level50_levelup(const MunitParameter params[], void* user_data_or_fixture) {
    edict_t* ent1 = munit_malloc(sizeof(edict_t));
    ent1->client = munit_malloc(sizeof(gclient_t));

    edict_t* ent2 = munit_malloc(sizeof(edict_t));
    ent2->client = munit_malloc(sizeof(gclient_t));

    vrx_initialize_player_class(ent1, 1);
    vrx_initialize_player_class(ent2, 1);

    munit_assert(ent1->myskills.level == 1);
    while (ent1->myskills.level < 48) {
        vrx_apply_experience(ent1, ent1->myskills.next_level - ent1->myskills.experience);
        vrx_apply_experience(ent2, ent2->myskills.next_level - ent2->myskills.experience);
    }

    munit_assert(ent1->myskills.level == 48);

    // tomek's funny case
    vrx_apply_experience(ent1, 6128663 - ent1->myskills.experience);
    munit_assert(ent1->myskills.level != 48);

    while (ent2->myskills.level < 50) {
        vrx_apply_experience(ent2, ent2->myskills.next_level - ent2->myskills.experience);
    }

    munit_assert(vrx_prestige_get_upgrade_points(ent1->myskills.experience) == vrx_prestige_get_upgrade_points(ent2->myskills.experience));

    auto limit = vrx_get_prestige_max_xp();
    vrx_apply_experience(ent1, limit - ent1->myskills.experience);
    vrx_apply_experience(ent2, limit - ent2->myskills.experience);

    munit_assert(vrx_prestige_get_upgrade_points(ent1->myskills.experience) == vrx_prestige_get_upgrade_points(ent2->myskills.experience));
    munit_assert(ent1->myskills.experience == limit);
    munit_assert(ent2->myskills.experience == limit);
    munit_assert(vrx_prestige_get_upgrade_points(ent1->myskills.experience) == PRESTIGE_MAX_POINTS);

    vrx_apply_experience(ent1, 100);
    munit_assert(ent1->myskills.experience == limit);

    free(ent1->client);
    free(ent1);
    return MUNIT_OK;
}

MunitTest points_tests[] = {
    {"/level50_levelup", test_level50_levelup, setup_cvars, cleanup_cvars, MUNIT_TEST_OPTION_NONE, 0},
    { nullptr, nullptr, nullptr, nullptr, MUNIT_TEST_OPTION_NONE, nullptr }
};

MunitSuite tests_suite = { "levelup", points_tests, nullptr, 0, MUNIT_SUITE_OPTION_NONE };

int main(int argc, char** argv) {
    tests_init();
    return munit_suite_main(&tests_suite, nullptr, argc, argv);
}