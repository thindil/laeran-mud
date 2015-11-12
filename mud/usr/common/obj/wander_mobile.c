#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

inherit MOBILE;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
}

string get_type(void) 
{
    return "wander";
}

void random_move(void)
{
    int    num_ex, ctr;
    object exit, dest;

    if (TAGD->get_tag_value(this_object()->get_body(), "Combat"))
        return;

    num_ex = location->num_exits();
    if(num_ex > 0) {
        int dir;

        ctr = random(100);
        if (ctr >= 60)
            return;
        ctr = random(num_ex);
        exit = location->get_exit_num(ctr);
        if(!exit)
            error("Błąd wewnętrzny! Nie mogę pobrać wyjścia!");

        dir = exit->get_direction();
        dest = exit->get_destination();

        if (ZONED->get_zone_for_room(dest) != ZONED->get_zone_for_room(location))
            return;

        this_object()->move(dir);
    }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
