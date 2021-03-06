#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <kernel/kernel.h>

inherit room ROOM;
inherit unq DTD_UNQABLE;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);
  if(clone) {
    bdesc = PHR("Pokój");
    ldesc = PHR("Widzisz pokój tutaj.");
    edesc = nil;

    MAPD->add_room_object(this_object());
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    room::destructed(clone);
    unq::destructed(clone);
    if(clone) {
      MAPD->remove_room_object(this_object());
    }
  }
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    room::upgraded(clone);
    unq::upgraded(clone);
  }
}

string to_unq_text(void) {
  if(SYSTEM() || COMMON() || GAME()) {
    return "~object{\n  ~obj_type{simple room}\n" + to_unq_flags() + "\n}\n\n";
  }
  return nil;
}

void from_dtd_unq(mixed* unq) {
  int    ctr;
  string dtd_tag_name;
  mixed  dtd_tag_val;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Nie możesz wywoływać from_dtd_unq bez uprawnień!");

  if(unq[0] != "object")
    error("Nie wygląda jak dane obiektu!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    dtd_tag_name = unq[1][ctr][0];
    dtd_tag_val  = unq[1][ctr][1];

    if(dtd_tag_name == "data") {
      error("Simple_room nie powinien posiadać pola data!"
	    + "  Czyżbyś zapomniał pola obj_type{} w jakimś "
	    + "innym typie pokoju?");
    } else {
      from_dtd_tag(dtd_tag_name, dtd_tag_val);
    }
  }
}
