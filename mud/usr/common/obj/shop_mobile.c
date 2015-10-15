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
	  + " Jeżli chcesz obejrzeć przedmiot, szepnij do mnie 'zobacz <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś kupić, szepnij do mnie 'kupuje <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>'.\n");
}

/* Trader iteraction */
void hook_whisper(object body, string message)
{
  string* parts;
  mixed* objs;
  int i;
  string items;

  parts = explode(message, " ");
  objs = get_body()->objects_in_container();
  items = "Lista dostępnych na sprzedaż przedmiotów: \n";
  switch (parts[0])
    {
    case "lista":
      for (i = 0; i < sizeof(objs); i++)
	{
	  items += "- " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + "\n";
	}
      body->get_mobile()->get_user()->message_scroll(items);
      break;
    case "zobacz":
      if (sizeof(parts) < 2 || parts[1] == "")
	{
	  whisper(body, "Jaki przedmiot chcesz konkretnie zobaczyć?");
	  return;
	}
      for (i = 0; i < sizeof(objs); i++)
	{
	  if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " "))
	    {
	      body->get_mobile()->get_user()->message(objs[i]->get_look()->to_string(body->get_mobile()->get_user()) + "\n");
	      break;
	    }
	}
      break;
    default:
      whisper(body, "Witaj w moim sklepie. Jeżeli chcesz zobaczyć listę towarów, szepnij do mnie 'lista'. \n"
	  + " Jeżli chcesz obejrzeć przedmiot, szepnij do mnie 'zobacz <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś kupić, szepnij do mnie 'kupuje <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>'.\n");
    }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
