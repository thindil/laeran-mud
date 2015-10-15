#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

inherit MOBILE;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone)
{
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone)
{
}

string get_type(void) {
  return "shop";
}

/* Initiate communication */
void hook_social(object body, object target, string verb)
{
  whisper(body, "Witaj w moim sklepie. Jeżeli chcesz zobaczyć listę towarów, szepnij do mnie 'lista'. \n"
	  + " Jeżeli chcesz coś kupić szepnij do mnie 'kupuje <nazwa przedmiotu>.'\n"
	  + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>' \n");
}

/* Start trade */
void hook_whisper(object body, string message)
{
  whisper(body, "|"+message+"|\n");
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
