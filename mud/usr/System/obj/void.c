#include <phantasmal/lpc_names.h>

/* void.c:
 *
 * The Void is the first room created, and holds space for creators to
 * play around.
 */

inherit ROOM;

#define NEW_PHRASE(a) PHRASED->new_simple_english_phrase(a)

static void create(int clone) {
  ::create(clone);
  if(clone) {
    bdesc = NEW_PHRASE("Pustka");
    ldesc = NEW_PHRASE("Dziki wiatr świszcze w wirze pustki...");
    edesc = NEW_PHRASE("O, coś jest w oddali! Nie, to tylko złudzenie.");

    MAPD->add_room_object(this_object());
  }

}
