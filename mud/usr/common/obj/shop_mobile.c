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

string get_type(void) 
{
  return "shop";
}

/* Initiate communication */
void hook_social(object body, object target, string verb)
{
    if (target && get_number() == target->get_mobile()->get_number() && verb == "pozdrow") {
        whisper(body, "Witaj w moim sklepie. Jeżeli chcesz zobaczyć listę towarów, szepnij do mnie 'lista'. \n"
                + " Jeżeli chcesz obejrzeć przedmiot, szepnij do mnie 'zobacz <nazwa przedmiotu>'.\n"
                + " Jeżeli chcesz coś kupić, szepnij do mnie 'kupuje <nazwa przedmiotu>'.\n"
                + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>'.\n");
    }
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
    switch (parts[0]) {
        case "lista":
            for (i = 0; i < sizeof(objs); i++)
                items += "- " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + " cena: " + (string)objs[i]->get_price() + " miedziaków\n";
            body->get_mobile()->get_user()->message_scroll(items);
            return;
        case "zobacz":
            if (sizeof(parts) < 2 || parts[1] == "") {
                whisper(body, "Jaki przedmiot chcesz konkretnie zobaczyć?");
                return;
            }
            for (i = 0; i < sizeof(objs); i++) {
                if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " ")) {
                    body->get_mobile()->get_user()->message(objs[i]->get_look()->to_string(body->get_mobile()->get_user()) + "\n");
                    return;
                }
            }
            break;
        case "kupuje":
            if (sizeof(parts) < 2 || parts[1] == "") {
                whisper(body, "Jaki przedmiot chcesz konkretnie kupić?");
                return;
            }
            for (i = 0; i < sizeof(objs); i++) {
                if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " ")) {
                    items = body->can_put(body->get_mobile()->get_user(), body, objs[i], nil);
                    if (items) {
                        body->get_mobile()->get_user()->message(items + "\n");
                        return;
                    }
                    if (body->get_price() < objs[i]->get_price()) {
                        body->get_mobile()->get_user()->message("Nie masz tylu pieniędzy aby kupić ten przedmiot.\n");
                        return;
                    }
                    new_object = clone_object(SIMPLE_ROOM);
                    MAPD->add_room_to_zone(new_object, -1, ZONED->get_zone_for_room(body->get_location()));
                    new_object->add_archetype(objs[i]);
                    new_object->set_brief(nil);
                    new_object->set_look(nil);
                    body->get_location()->add_to_container(new_object);
                    if (objs[i]->is_container())
                        new_object->set_container(1);
                    if (objs[i]->is_open())
                        new_object->set_open(1);
                    if (objs[i]->is_openable())
                        new_object->set_openable(1);
                    if (objs[i]->is_weapon())
                        new_object->set_weapon(1);
                    if (objs[i]->is_wearable())
                        new_object->set_wearable(1);
                    items = body->get_mobile()->place(new_object, body);
                    if (!items) {
                        body->set_price(body->get_price() - objs[i]->get_price());
                        body->get_mobile()->get_user()->message("Kupiłeś " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + " i zapłaciłeś za to " + (string)objs[i]->get_price() + " miedziaków. \n");
                    } else {
                        body->get_mobile()->get_user()->message(items + "\n");
                        new_object->get_location()->remove_from_container(new_object);
                        destruct_object(new_object);
                    }
                    return;
                }
            }
            break;
        case "sprzedaje":
            if (sizeof(parts) < 2 || parts[1] == "") {
                whisper(body, "Jaki przedmiot chcesz konkretnie sprzedać?");
                return;
            }
            objs = body->objects_in_container();
            for (i = 0; i < sizeof(objs); i++) {
                if (objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) == implode(parts[1..], " ")) {
                    items = body->get_mobile()->place(objs[i], body->get_location());
                    if (!items) {
                        body->set_price(body->get_price() + objs[i]->get_price());
                        body->get_mobile()->get_user()->message("Sprzedałeś " + objs[i]->get_brief()->to_string(body->get_mobile()->get_user()) + " i zarobiłeś na tym " + (string)objs[i]->get_price() + " miedziaków.\n");
                        objs[i]->get_location()->remove_from_container(objs[i]);
                        destruct_object(objs[i]);
                    } else 
                        body->get_mobile()->get_user()->message(items + "\n");
                    return;
                }
            }
            whisper(body, "Widzę, że nie masz takiego przedmiotu na sprzedaż.");
            return;
        default:
            whisper(body, "Witaj w moim sklepie. Jeżeli chcesz zobaczyć listę towarów, szepnij do mnie 'lista'. \n"
                    + " Jeżeli chcesz obejrzeć przedmiot, szepnij do mnie 'zobacz <nazwa przedmiotu>'.\n"
                    + " Jeżeli chcesz coś kupić, szepnij do mnie 'kupuje <nazwa przedmiotu>'.\n"
                    + " Jeżeli chcesz coś sprzedać, szepnij do mnie 'sprzedaje <nazwa przedmiotu>'.\n");
            return;
    }
    whisper(body, "Nie mam takiego przedmiotu na sprzedaż");
}

/* Open/close shop based on time */
void hook_time(int hour)
{
    int i;
    object exit;

    for (i = 0; i < location->num_exits(); i++) {
        exit = location->get_exit_num(i);
        if (hour > 21 || hour < 6) {
            if (exit->is_open() && exit->is_openable()) {
                if (sizeof(location->mobiles_in_container()) > 1) {
                    say("Zamykamy, proszę wyjść.");
                    emote("wypycha wszystkich za drzwi i zamyka je.");
                    location->enum_room_mobiles("move", ({ this_object() }), exit->get_direction() );
                }
                close(exit);
                exit->set_locked(1);
            }
        } else {
            if (exit->is_locked()) {
                exit->set_locked(0);
            }
            if (!exit->is_open() && exit->is_openable()) {
                if (sizeof(location->mobiles_in_container()) > 1) 
                    emote("otwiera drzwi.");
                open(exit);
            }
        }
    }
}

void from_dtd_unq(mixed* unq) 
{
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
