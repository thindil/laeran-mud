#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/log.h>
#include <phantasmal/exit.h>
#include <phantasmal/lpc_names.h>

inherit unq DTD_UNQABLE;
inherit ext EXIT;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  ext::create(clone);
  unq::create(clone);
  if(clone) {
    bdesc = nil;
    ldesc = nil;
    edesc = nil;
  }
}

void destructed(int clone, varargs int looped) {
  object exit, exit2, room;
  if(clone) {
    exit = this_object();
    room = exit->get_from_location();
    EXITD->remove_exit(room, exit);
    unq::destructed(clone);
    ext::destructed(clone);
  }
}

void upgraded(varargs int clone) {
  ext::upgraded(clone);
  unq::upgraded(clone);
}

string to_unq_text(void) {
  return "  ~newexit{\n" + to_unq_flags() + "}\n";
}

void from_dtd_unq(mixed* unq) {
  int ctr;

  if(unq[0] != "newexit")
    error("Nie wygląda jak dane dla wyjścia!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
  }
}
