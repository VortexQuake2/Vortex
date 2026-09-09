#pragma once

typedef bool (*buycallback)(edict_t *ent, const void *data);

typedef int (*pricecallback)(edict_t *ent, const void *data);

// due to limitations of the armory menu layout, it's 255 items or categories max.

struct armoryitem_s {
    const char *name;
    const void *buydata;
    const buycallback callback;
    const pricecallback pricecallback;
    const void *pricecallbackdata;
    bool needconfirmation;
    const char *confirmation;
};

struct armorycategory_s {
    const char *name;
    const struct armoryitem_s *items;
    const buycallback default_buy_callback;
    const pricecallback default_price_callback;
    const size_t numitems;
};

struct armory_s {
    struct armorycategory_s *categories;
    size_t numcategories;
};

struct armory_s* vrx_armory_get();