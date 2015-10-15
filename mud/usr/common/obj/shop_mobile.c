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
  object new_object;

  parts = explode(message, " ");
  objs = get_body()->objects_in_container();
  items = "Lista dostępnych na sprzedaż przedmiotów: \n";
  switch (parts[0])
    {
    case "lista":
      for (i = 0; i < sizeof(objs); i++)
	{
	  items += "- " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + " cena: " + (string)objs[i]->get_armor() + " miedziaków\n";
	}
      body->get_mobile()->get_user()->message_scroll(items);
      return;
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
	      return;
	    }
	}
      break;
    case "kupuje":
      if (sizeof(parts) < 2 || parts[1] == "")
	{
	  whisper(body, "Jaki przedmiot chcesz konkretnie kupić?");
	  return;
	}
      for (i = 0; i < sizeof(objs); i++)
	{
	  if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " "))
	    {
	      new_object = clone_object(SIMPLE_ROOM);
	      MAPD->add_room_to_zone(new_object, -1, ZONED->get_zone_for_room(body->get_location()));
	      new_object->add_archetype(objs[i]);
	      new_object->set_brief(objs[i]->get_brief());
	      new_object->set_look(objs[i]->get_look());
	      new_object->set_examine(objs[i]->get_examine());
	      if (objs[i]->is_container())
		{
		  new_object->set_container(1);
		}
	      if (objs[i]->is_open())
		{
		  new_object->set_open(1);
		}
	      if (objs[i]->is_openable())
		{
		  new_object->set_openable(1);
		}
	      if (objs[i]->is_weapon())
		{
		  new_object->set_weapon(1);
		}
	      if (objs[i]->is_wearable())
		{
		  new_object->set_wearable(1);
		}
	      body->add_to_container(new_object);
	      body->get_mobile()->get_user()->message("Kupiłeś " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + "\n");
	      return;
	    }
	}
      break;
    case "sprzedaje":
	if (sizeof(parts) < 2 || parts[1] == "")
	{
	  whisper(body, "Jaki przedmiot chcesz konkretnie sprzedać?");
	  return;
	}
	objs = body->objects_in_container();
	for (i = 0; i < sizeof(objs); i++)
	{
	  if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " "))
	    {
	      body->get_mobile()->get_user()->message("Sprzedałeś " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + "\n");
	      body->remove_from_container(objs[i]);
	      destruct_object(objs[i]);
	      return;
	    }
	}
	whisper(body, "Widzę, że nie masz takiego przedmiotu na sprzedaż.");
	return;
    default:
      whisper(body, "Witaj w moim sklepie. Jeżeli chcesz zobaczyć listę towarów, szepnij do mnie 'lista'. \n"
	  + " Jeżli chcesz obejrzeć przedmiot, szepnij do mnie 'zobacz <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś kupić, szepnij do mnie 'kupuje <nazwa przedmiotu>'.\n"
	  + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>'.\n");
      return;
    }
  whisper(body, "Nie mam takiego przedmiotu na sprzedaż");
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
