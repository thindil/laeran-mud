#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/grammar.h>
#include <phantasmal/search_locations.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <status.h>
#include <limits.h>

inherit access API_ACCESS;



/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{
  if(clone) {

  } else {
    if(!find_object(LWO_PHRASE))
      compile_object(LWO_PHRASE);
  }
}

static void upgraded(varargs int clone) {

}

static void destructed(varargs int clone) {

}


/********** Object Functions ***************************************/

static object resolve_object_name(object user, string name)
{
  object obj;
  int    obnum;

  if(sscanf(name, "#%d", obnum)) {
    obj = MAPD->get_room_by_num(obnum);
    if(obj)
      return obj;
    obj = EXITD->get_exit_by_num(obnum);
    if(obj)
      return obj;
    
  }

  return nil;
}


/* Set object description.  This command is @set_brief,
   @set_look and @set_examine. */
static void cmd_set_obj_desc(object user, string cmd, string str) {
  object obj;
  string desc, objname;
  string look_type;
  int    must_set;

  if(!sscanf(cmd, "@set_%s", look_type))
    error("Nierozpoznana komenda do ustawiania opisu: " + cmd);

  desc = nil;
  if(!str || str == "") {
    obj = user->get_location();
    user->message("(Obecna lokacja)\n");
  } else if(sscanf(str, "%s %s", objname, desc) == 2
	    || sscanf(str, "%s", objname)) {
    obj = resolve_object_name(user, objname);
  }

  if(!obj) {
    user->message("Brak prawidłowego obiektu do ustawienia!\n");
    return;
  }

  if(!desc) {
    user->push_new_state(US_OBJ_DESC, obj, look_type, user->get_locale());
    user->push_new_state(US_ENTER_DATA);

    return;
  }

  if(desc) {
    object phr;

    user->message("Ustawianie opisu " + look_type + " na obiekcie #"
		  + obj->get_number() + "\n");

    if(!function_object("get_" + look_type, obj))
      error("Nie mogę znaleźć funkcji get dla " + look_type + " w "
	    + object_name(obj));

    must_set = 0;
    phr = call_other(obj, "get_" + look_type);
    if(!phr || (look_type == "examine" && phr == obj->get_look())) {
      phr = PHRASED->new_simple_english_phrase("ZMIEŃ MNIE!");
      must_set = 1;
    }
    phr->from_unq(desc);
    if(must_set) {
      call_other(obj, "set_" + look_type, phr);
    }
  }
}


private void priv_mob_stat(object user, object mob) {
  object mobbody, mobuser;
  mixed* tags;
  int    ctr;
  string tmp;

  mobbody = mob->get_body();
  mobuser = mob->get_user();

  tmp = "";

  tmp = "Ciało: ";
  if(mobbody) {
    tmp += "#" + mobbody->get_number() + " (";
    tmp += mobbody->get_brief()->to_string(user);
    tmp += ")\n";
  } else {
    tmp += "(brak)\n";
  }

  tmp += "Użytkownik: ";
  if(mobuser) {
    tmp += mobuser->get_name() + "\n";
  } else {
    tmp += "(NPC, nie gracz)\n";
  }

  tmp += "Typ: " + mob->get_type() + "\n";

    if (mob->get_parentbody() && mob->get_parentbody() > 0)
        tmp += "Ciało archetypu: " + mob->get_parentbody() + "\n";

    if (mob->get_spawnroom() && mob->get_spawnroom() > 0)
        tmp += "Pokój w którym się pojawia: " + mob->get_spawnroom() + "\n";

  tags = TAGD->mobile_all_tags(mob);
  if(!sizeof(tags)) {
    tmp += "\nBrak tagów.\n";
  } else {
    for(ctr = 0; ctr < sizeof(tags); ctr+=2) {
      tmp += "  " + tags[ctr] + ": " + STRINGD->mixed_sprint(tags[ctr + 1])
	+ "\n";
    }
  }

  user->message_scroll(tmp);
}


static void cmd_stat(object user, string cmd, string str) {
    int     objnum, ctr;
    object  obj, room, exit, location;
    string* words;
    string  tmp;
    mixed*  objs, *tags;
    object *details, *archetypes;
    mapping dmg_res;

    if(!str || STRINGD->is_whitespace(str)) {
        user->message("Użycie: " + cmd + " #<numer obiektu>\n");
        user->message("        " + cmd + " <opis obiektu>\n");
        return;
    }

    if(sscanf(str, "#%d", objnum) != 1) {
        str = STRINGD->trim_whitespace(str);

        if(!STRINGD->stricmp(str, "tutaj")) {
            if(!user->get_location()) {
                user->message("Jesteś w pustce!\n");
                return;
            }
            objnum = user->get_location()->get_number();
        } else {

            objs = user->find_first_objects(str, LOC_INVENTORY, LOC_CURRENT_ROOM,
                    LOC_BODY, LOC_CURRENT_EXITS);
            if(!objs) {
                user->message("Nie możesz znaleźć jakiegokolwiek '" + str + "' tutaj.\n");
                return;
            }

            if(sizeof(objs) > 1) {
                user->message("Więcej niż jeden obiekt pasuje. Wybierasz pierwszy.\n");
            }

            objnum = objs[0]->get_number();
        }
    }

    room = MAPD->get_room_by_num(objnum);
    exit = EXITD->get_exit_by_num(objnum);

    obj = room ? room : exit;

    if(!obj) {
        object mob;

        mob = MOBILED->get_mobile_by_num(objnum);

        if(!mob) {
            user->message("Nie znaleziono obiektu #" + objnum+ " zarejestrowanego z MAPD, EXITD lub MOBILED.\n");
            return;
        }
        priv_mob_stat(user, mob);
        return;
    }

    tmp  = "Numer: " + obj->get_number() + "\n";
    if(obj->get_detail_of()) {
        tmp += "Detal z: ";
    } else {
        tmp += "Lokacja: ";
    }
    location = obj->get_location();
    if(location) {
        if(typeof(location->get_number()) == T_INT
                && location->get_number() != -1) {
            tmp += "#" + location->get_number();

            if(location->get_brief()) {
                tmp += " (";
                tmp += location->get_brief()->to_string(user);
                tmp += ")\n";
            } else {
                tmp += "\n";
            }
        } else {
            tmp += " (niezarejestrowany)\n";
        }
    } else {
        tmp += " (żaden)\n";
    }

    tmp += "Opisy ("
        + PHRASED->locale_name_for_language(user->get_locale())
        + ")\n";

    tmp += "Krótki (Brief): ";
    if(obj->get_brief()) 
        tmp += "'" + obj->get_brief()->to_string(user) + "'\n";
    else 
        tmp += "(brak)\n";

    tmp += "Patrz (Look):\n  ";
    if(obj->get_look()) 
        tmp += "'" + obj->get_look()->to_string(user) + "'\n";
    else
        tmp += "(brak)\n";

    tmp += "Zbadaj (Examine):\n  ";
    if(obj->get_examine()) {
        if(obj->get_examine() == obj->get_look()) {
            tmp += "(tak sam jak opis Patrz)\n";
        } else {
            tmp += "'" + obj->get_examine()->to_string(user) + "'\n";
        }
    } else {
        tmp += "(brak)\n";
    }

    /* Show user the nouns and adjectives */
    tmp += "Rzeczowniki ("
        + PHRASED->locale_name_for_language(user->get_locale())
        + "): ";
    words = obj->get_nouns(user->get_locale());
    tmp += implode(words, ", ");
    tmp += "\n";

    tmp += "Przymiotniki ("
        + PHRASED->locale_name_for_language(user->get_locale())
        + "): ";
    words = obj->get_adjectives(user->get_locale());
    tmp += implode(words, ", ");
    tmp += "\n\n";

    if(function_object("get_weight", obj)) {
        tmp += "Jego waga to " + obj->get_weight() + " kilogramy.\n";
        tmp += "Jego objętość to " + obj->get_volume() + " litry.\n";
        tmp += "Jego długość to " + obj->get_length() + " centymetry.\n";
    }

    if (function_object("get_destination", obj)) 
        tmp += "Prowadzi do pokoju #" + obj->get_destination()->get_number() + "\n";

    if(function_object("is_locked", obj)) {
        if (obj->is_locked())
            tmp += "Obiekt jest zablokowany.\n";
        else
            tmp += "Obiekt jest odblokowany.\n";
        if (obj->is_lockable())
            tmp += "Obiekt można swobodnie odblokowywać i zablokowywać.\n";
        else
            tmp += "Obiekt nie można swobodnie odblokowywać i zablokowywać.\n";
    }

    if(function_object("is_container", obj)) {
        if(obj->is_container()) {
            if(obj->is_open())
                tmp += "Obiekt jest otwartym pojemnikiem.\n";
            else
                tmp += "Obiekt jest zamkniętym pojemnikiem.\n";
            if(obj->is_openable()) 
                tmp += "Obiekt może być swobodnie zamykany bądź otwierany.\n";
            else
                tmp += "Obiekt nie może być swobodnie zamykany bądź otwierany.\n";
            if(function_object("get_weight_capacity", obj)) {
                tmp += "Zawiera " + obj->get_current_weight()
                    + " kg z maksymalnie " + obj->get_weight_capacity()
                    + " kilogramów.\n";
                tmp += "Zawiera " + obj->get_current_volume()
                    + " l z maksymalnie " + obj->get_volume_capacity()
                    + " litrów.\n";
                tmp += "Jego maksymalna długość/szerokość zawartości to " + obj->get_length_capacity()
                    + " centymetrów.\n";
            }
        } else {
            tmp += "Obiekt nie jest pojemnikiem.\n";
        }
    }

    tmp += "\n";

    if(function_object("num_objects_in_container", obj)) {
        tmp += "Zawiera obiekty [" + obj->num_objects_in_container()
            + "]: ";
        objs = obj->objects_in_container();
        for(ctr = 0; ctr < sizeof(objs); ctr++) {
            if(typeof(objs[ctr]->get_number()) == T_INT
                    && objs[ctr]->get_number() != -1) {
                tmp += "#" + objs[ctr]->get_number() + " ";
            } else {
                tmp += "<nierejestrowany> ";
            }
        }
        tmp += "\nZawiera " + sizeof(obj->mobiles_in_container())
            + " mobków.\n\n";
    }

    if (function_object("num_exits", obj)) {
        if (obj->num_exits()) {
            tmp += "Posiada [" + obj->num_exits() + "] wyjść: ";
            for (ctr = 0; ctr < obj->num_exits(); ctr++)
                tmp += "#" + obj->get_exit_num(ctr)->get_number() + " ";
            tmp += "\n\n";
        }
    }

    if (function_object("get_damage", obj))
        tmp += "Zadaje " + (string)obj->get_damage() + " obrażeń.\n";
    if (function_object("get_damage_type", obj))
        tmp += "Rodzaj obrażeń: " + obj->get_damage_type() + ".\n";

    if(function_object("is_wearable", obj))
    {
        string* wlocations;
        int* wlocationsnum;
        if (obj->is_wearable())
        {
            tmp += "Obiekt można nosić.\n";
            tmp += "Lokacje ciała: ";
            wlocationsnum = obj->get_wearlocations();
            wlocations = ({"głowa", "tułów", "ręce", "dłonie", "nogi", "prawa dłoń", "lewa dłoń", "plecy", "prawy pas", "lewy pas"});
            for (ctr = 0; ctr < sizeof(wlocationsnum); ctr++)
            {
                tmp += wlocations[wlocationsnum[ctr]] + " ";
            }
            tmp += "\n";
        }
    }

    if(function_object("is_dressed", obj)) 
    {
        if (obj->is_dressed())
        {
            tmp += "Obiekt jest założony.\n";
        }
    }

    if (function_object("get_armor", obj))
        tmp += "Zbroja obiektu wynosi: " + (string)obj->get_armor() + "\n";
    if (function_object("get_damage_res", obj)) {
        dmg_res = obj->get_damage_res();
        words = map_indices(dmg_res);
        tmp += "Odporności obiektu:";
        if (!sizeof(words))
            tmp += " brak";
        for (ctr = 0; ctr < sizeof(words); ctr++) 
            tmp += " " + dmg_res[words[ctr]] + "% na " + words[ctr];
        tmp += "\n";
    }
    if (function_object("get_price", obj))
        tmp += "Cena za obiekt wynosi: " + (string)obj->get_price() + "\n";
    if (function_object("get_hp", obj))
        tmp += "Punkty życia obiektu: " + (string)obj->get_hp() + "\n";
    if (function_object("get_combat_rating", obj))
        tmp += "Poziom bojowy obiektu: " + (string)obj->get_combat_rating() + "\n";

    if (function_object("get_body_locations", obj) && sizeof(obj->get_body_locations()))
        tmp += "Lokacje ciała: " + implode(obj->get_body_locations(), ", ") + "\n";
    if (function_object("get_skill", obj) && strlen(obj->get_skill()))
        tmp += "Umiejętność: " + obj->get_skill() + "\n";
    if (function_object("get_craft_skill", obj) && strlen(obj->get_craft_skill()))
        tmp += "Umiejętność rzemieślnicza: " + obj->get_craft_skill() + "\n";
    if (function_object("get_quality", obj)) {
        if (!obj->get_quality())
            tmp += "Obiekt jest niezniszalny.\n";
        else {
            tmp += "Szansa na uszkodzenie obiektu wynosi: " + (string)obj->get_quality() + "%\n"
                + "Obecna wytrzymałość obiektu wynosi: " + (string)obj->get_cur_durability() + "\n"
                + "Maksymalna wytrzymałość obiektu wynosi: " + (string)obj->get_durability() + "\n";
        }
    }
    if (function_object("get_room_type", obj) && obj->get_room_type()) {
        tmp += "Typ pokoju: ";
        switch (obj->get_room_type()) {
            case 1:
                tmp += "zewnętrzny\n";
                break;
            case 2:
                tmp += "wewnętrzny\n";
                break;
            case 3:
                tmp += "podziemia\n";
                break;
            default:
                tmp += "nieznany\n";
                break;
        }
    }

    details = obj->get_immediate_details();
    if(details && sizeof(details)) {
        object detail;

        tmp += "Zawiera natychmiastowe detale [" + sizeof(details) + "]: ";
        for(ctr = 0; ctr < sizeof(details); ctr++) {
            detail = details[ctr];
            if(detail) {
                tmp += "#" + detail->get_number() + " ";
            } else {
                tmp += "<bez numeru> ";
            }
        }
        tmp += "\n";
    }
    details = obj->get_details();
    archetypes = obj->get_archetypes();
    if(sizeof(archetypes) && sizeof(details)) {
        object detail;

        tmp += "Zawiera kompletne detale [" + sizeof(details) + "]: ";
        for(ctr = 0; ctr < sizeof(details); ctr++) {
            detail = details[ctr];
            if(detail) {
                tmp += "#" + detail->get_number() + " ";
            } else {
                tmp += "<bez numeru> ";
            }
        }
        tmp += "\n";
    }

    tags = TAGD->object_all_tags(obj);
    if(!sizeof(tags)) {
        tmp += "\nNie ma ustawionych tagów.\n";
    } else {
        tmp += "\nNazwa tagu: Wartość\n";
        for(ctr = 0; ctr < sizeof(tags); ctr+=2) {
            tmp += "  " + tags[ctr] + ": " + STRINGD->mixed_sprint(tags[ctr + 1])
                + "\n";
        }
    }
    tmp += "\n";

    if(obj->get_mobile()) {
        tmp += "Obiekt jest istotą.\n";
        if(obj->get_mobile()->get_user()) {
            tmp += "Obiekt jest ciałem gracza "
                + obj->get_mobile()->get_user()->query_name()
                + ".\n";
        }
    }

    if(sizeof(archetypes)) {
        tmp += "Jest potomkiem obiektów: ";
        for(ctr = 0; ctr < sizeof(archetypes); ctr++) {
            tmp += "#" + archetypes[ctr]->get_number()
                + " (" + archetypes[ctr]->get_brief()->to_string(user)
                + ")";
        }
        tmp += ".\n";
    }
    if(room) {
        tmp += "Zarejestrowany z MAPD jako pokój bądź przenośny.\n";
    }
    if(exit) {
        tmp += "Zarejestrowany z EXITD jako wyjście.\n";
    }

    user->message_scroll(tmp);
}


static void cmd_add_nouns(object user, string cmd, string str) {
    int     ctr;
    object  obj;
    string* words;

    if(str)
        str = STRINGD->trim_whitespace(str);

    if(!str || str == "" || !sscanf(str, "%*s %*s")) {
        user->message("Użycie: " + cmd + " #<numer obiektu> [<rzeczownik> <rzeczownik>...]\n");
        return;
    }

    words = explode(str, " ");
    obj = resolve_object_name(user, words[0]);
    if(!obj) {
        user->message("Nie mogę znaleźć obiektu '" + words[0] + "'.\n");
        return;
    }

    user->message("Dodawanie rzeczowników...");
    obj->add_noun(PHRASED->new_simple_english_phrase(implode(words[1..], ",")));
    user->message("wykonane.\n");
}


static void cmd_clear_nouns(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_nouns();
    user->message("Usunięte.\n");
  } else {
    user->message("Nie mogę znaleźć obiektu '" + str + "'.\n");
  }
}


static void cmd_add_adjectives(object user, string cmd, string str) {
  int     ctr;
  object  obj, phr;
  string* words;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || str == "" || !sscanf(str, "%*s %*s")) {
    user->message("Użycie: " + cmd + " #<numer obiektu> [<przymiotnik> <przymiotnik>...]\n");
    return;
  }

  words = explode(str, " ");
  obj = resolve_object_name(user, words[0]);
  if(!obj) {
    user->message("Nie można znaleźć obiektu '" + words[0] + "'.\n");
    return;
  }

  for(ctr = 1; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  user->message("Dodawanie przymiotników ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ").\n");
  phr = new_object(LWO_PHRASE);
  phr->from_unq("~plPL{" + implode(words[1..],",") + "}");
  obj->add_adjective(phr);
  user->message("Wykonane.\n");
}


static void cmd_clear_adjectives(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_adjectives();
    user->message("Wyczyszczone.\n");
  } else {
    user->message("Nie można znaleźć obiektu '" + str + "'.\n");
  }
}


static void cmd_move_obj(object user, string cmd, string str) {
  object obj1, obj2;
  int    objnum1, objnum2;
  string second;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || ((sscanf(str, "#%d #%d", objnum1, objnum2) != 2)
	 && (sscanf(str, "#%d %s", objnum1, second) != 2))) {
    user->message("Użycie: " + cmd + " #<obiekt> #<lokacja>\n");
    user->message("    lub " + cmd + " #<obiekt> here\n");
    return;
  }

  if(second && STRINGD->stricmp(second, "here")) {
    user->message("Użycie: " + cmd + " #<obiekt> #<lokacja>\n");
    user->message("    lub " + cmd + " #<obiekt> tutaj\n");
  }

  if(second) {
    if(user->get_location()) {
      objnum2 = user->get_location()->get_number();
    } else {
      user->message("Nie możesz przenieść obiektu do swojej obecnej lokacji "
		    + "ponieważ jej nie masz!\n");
      return;
    }
  }

  obj2 = MAPD->get_room_by_num(objnum2);
  if(!obj2) {
    user->message("Drugi argument musi być pokojem. Obiekt #"
		  + objnum2 + " nie jest.\n");
    return;
  }

  obj1 = MAPD->get_room_by_num(objnum1);
  if(!obj1) {
    obj1 = EXITD->get_exit_by_num(objnum1);
  }

  if(!obj1) {
    user->message("Obiekt #" + objnum1 + " nie wygląda że jest zarejestrowany "
		  + "jako pokój bądź wyjście.\n");
    return;
  }

  if(obj1->get_location()) {
    obj1->get_location()->remove_from_container(obj1);
  }
  obj2->add_to_container(obj1);

  user->message("Teleportowałeś ");
  user->send_phrase(obj1->get_brief());
  user->message(" (#" + obj1->get_number() + ") w ");
  user->send_phrase(obj2->get_brief());
  user->message(" (#" + obj2->get_number() + ").\n");
}


static void cmd_set_obj_parent(object user, string cmd, string str) {
  object  obj, parent;
  string  parentstr, *par_entries;
  object *parents;
  int     objnum;
  mapping par_kw;

  if(!str
     || (sscanf(str, "#%d %s", objnum, parentstr) != 2)) {
    user->message("Użycie: " + cmd + " #<obiekt> #<rodzic> [#<rodzic2>...]\n");
    user->message("        " + cmd + " #<obiekt> none\n");
    return;
  }

  parentstr = STRINGD->trim_whitespace(parentstr);

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("Obiekt musi być pokojem bądź przenośny. Obiekt #"
		  + objnum + " nie jest.\n");
    return;
  }

  parentstr = STRINGD->trim_whitespace(STRINGD->to_lower(parentstr));

  if(!STRINGD->stricmp(parentstr, "none")) {
    obj->set_archetypes( ({ }) );
    user->message("Usunieto wszystkich rodziców.\n");
    return;
  }

  par_entries = explode(parentstr, " ");
  parents = ({ });

  par_kw = ([ "none" : 1,
	    "add" : 1,
	    "remove" : 1 ]);

  if(!par_kw[par_entries[0]]) {
    user->message("Argument operacji musi być jednym z 'none', 'add' lub "
		  + "remove.\n Twój argument, '" + par_entries[0]
		  + "', nie był.\n Sprawdź wpis pomocy.\n");
    return;
  }

  for(objnum = 1; objnum < sizeof(par_entries); objnum++) {
    int parentnum;

    par_entries[objnum] = STRINGD->trim_whitespace(par_entries[objnum]);
    if(!par_entries[objnum] || par_entries[objnum] == "")
      continue;

    if(!sscanf(par_entries[objnum], "#%d", parentnum)) {
      user->message("Argumenty rodziców muszą być numerami obiektów ("
		    + "#<numer>).\n"
		    + " Twój argument, '" + par_entries[objnum]
		    + "', nie był.\n");
      return;
    }

    parent = MAPD->get_room_by_num(parentnum);

    if(!parent) {
      user->message("Rodzice muszą być pokojami bądź przenośnymi. Obiekt #"
		    + parentnum
		    + " nie jest.\n");
      return;
    }

    parents += ({ parent });
  }

  switch(par_entries[0]) {
  case "none":
    /* Should never get this far */
    user->message("Nie dołączaj żadnych numerów obiektu do argumentu 'none'."
		  + "\n");
    return;

  case "add":
    parents = obj->get_archetypes() + parents;
    /* Fall through to next case */

  case "set":
    obj->set_archetypes(parents);
    break;
  default:
    user->message("Wewnętrzny błąd bazujący na słowie kluczowym.\n");
    error("Wewnętrzny błąd! Nie powinno Ciebie tutaj być!");
  }

  user->message("Zakończono ustawianie rodziców.\n");
}

static void cmd_set_obj_wearlocations(object user, string cmd, string str) 
{
    object obj;
    int objnum, i;
    string locations;
    string *value;
    int *newvalue;

    
    if(!str || sscanf(str, "#%d %s", objnum, locations) < 1) {
        user->message("Użycie: " + cmd + " #<obiekt> [wartość]\n");
        return;
    }
    if (!locations)
        locations = "";
    value = explode(locations, ", ");

    newvalue = ({ });
    for (i = 0; i < sizeof(value); i++) {
        switch (value[i]) {
        case "głowa":
            newvalue += ({0});
            break;
        case "tułów":
            newvalue += ({1});
            break;
        case "ręce":
            newvalue += ({2});
            break;
        case "dłonie":
            newvalue += ({3});
            break;
        case "nogi":
            newvalue += ({4});
            break;
        case "prawa dłoń":
            newvalue += ({5});
            break;
        case "lewa dłoń":
            newvalue += ({6});
            break;
        case "plecy":
            newvalue += ({7});
            break;
        case "prawy pas":
            newvalue += ({8});
            break;
        case "lewy pas":
            newvalue += ({9});
            break;
        default:
            break;
        }
    }

    obj = MAPD->get_room_by_num(objnum);
    if(!obj) {
        user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt #"
		      + objnum + " nie jest.\n");
        return;
    }

    call_other(obj, "set_wearlocations", newvalue);

    user->message("Wykonane.\n");
}

static void cmd_set_obj_room_type(object user, string cmd, string str)
{
    object  obj;
    int     objnum, value;

    if(!str || sscanf(str, "#%d %s", objnum, str) < 1) {
        user->message("Użycie: " + cmd + " #<obiekt> [wartość]\n");
        return;
    }

    if (!str)
        str = "";

    obj = MAPD->get_room_by_num(objnum);
    if(!obj) {
        user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt #"
                + objnum + " nie jest.\n");
        return;
    }
    switch (str) {
        case "zewnątrz":
            value = 1;
            break;
        case "wewnątrz":
            value = 2;
            break;
        case "podziemia":
            value = 3;
            break;
        default:
            value = 0;
            break;
    }
    call_other(obj, "set_room_type", value);
    user->message("Wykonane.\n");
}

static void cmd_set_obj_body_locations(object user, string cmd, string str) 
{
  object  obj;
  int     objnum;
  string  value;
  string  *newvalue;

  if(!str || sscanf(str, "#%d %s", objnum, value) < 1) {
    user->message("Użycie: " + cmd + " #<obiekt> [wartość]\n");
    return;
  }

  if (!value)
      value = "";

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt #"
		  + objnum + " nie jest.\n");
    return;
  }

  newvalue = explode(value, ", ");
  call_other(obj, "set_body_locations", newvalue);

  user->message("Wykonane.\n");
}

static void cmd_set_obj_damage_res(object user, string cmd, string str) 
{
    object obj;
    int objnum,i ;
    string value;
    string *pairs, *pair;
    mapping result;

    if(!str || sscanf(str, "#%d %s", objnum, value) < 1) {
        user->message("Użycie: " + cmd + " #<obiekt> [wartość]\n");
        return;
    }

    if (!value) 
        value = ""; 

    obj = MAPD->get_room_by_num(objnum);
    if(!obj) {
        user->message("Obiekt musi być pokojem lub przenośnym. Obiekt #"
                + objnum + " nie jest.\n");
        return;
    }
    pairs = explode(value, ", ");
    result = ([ ]);
    for (i = 0; i < sizeof(pairs); i++) {
        pair = explode(pairs[i], " ");
        result[pair[1]] = pair[0];
    }
    call_other(obj, "set_damage_res", result);

    user->message("Wykonane.\n");
}

/* Set object values for string based fields. */
static void cmd_set_obj_string_value(object user, string cmd, string str) 
{
  object  obj;
  int     objnum;
  string  cmd_value_name, newvalue;

  if(!str || sscanf(str, "#%d %s", objnum, newvalue) < 1) {
    user->message("Użycie: " + cmd + " #<obiekt> [wartość]\n");
    return;
  }

  if (!newvalue)
      newvalue = "";

  if (sscanf(cmd, "%*s_%*s_%s", cmd_value_name) != 3) {
    user->message("Wewnętrzny błąd w komendzie '" + cmd + "'.\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt numer #"
		  + objnum + " nie jest.\n");
    return;
  }

  call_other(obj, "set_" + cmd_value_name, newvalue);

  user->message("Wykonane.\n");
}
/* Set object values for int based fields. */
static void cmd_set_obj_int_value(object user, string cmd, string str) 
{
  object  obj;
  int     objnum, newvalue;
  string  cmd_value_name;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d %d", objnum, newvalue) != 2) {
    user->message("Użycie: " + cmd + " #<obiekt> <wartość>\n");
    return;
  }

  if (sscanf(cmd, "%*s_%*s_%s", cmd_value_name) != 3) {
    user->message("Wewnętrzny błąd w komendzie '" + cmd + "'.\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt #"
		  + objnum + " nie jest.\n");
    return;
  }

  if(newvalue < 0) {
    user->message("Wartości muszą być dodatnie, " + newvalue + " nie jest.\n");
    return;
  }

  call_other(obj, "set_" + cmd_value_name, newvalue);

  user->message("Wykonane.\n");
}


/* This function is called for set_obj_weight, set_obj_volume,
   and set_obj_height and their synonyms, as well as
   set_obj_weight_capacity, set_obj_height_capacity,
   set_obj_volume_capacity and so on. */
static void cmd_set_obj_value(object user, string cmd, string str) {
  object  obj;
  int     objnum, is_cap;
  float   newvalue;
  mapping val_names;
  string  cmd_value_name, cmd_norm_name;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d %f", objnum, newvalue) != 2) {
    user->message("Użycie: " + cmd + " #<obiekt> <wartość>\n");
    return;
  }

  val_names = ([
		"weight" : "weight",
		"volume" : "volume",
		"vol"    : "volume",
		"length" : "length",
		"len"    : "length",
		"height" : "length",
		]);

  /* For set_obj_weight_capacity and company */
  if(sscanf(cmd, "%*s_%*s_%s_%*s", cmd_value_name) == 4) {
    /* Normalize the name. */
    cmd_norm_name = val_names[cmd_value_name];
    is_cap = 1;
  } else if (sscanf(cmd, "%*s_%*s_%s", cmd_value_name) == 3) {
    /* Not a set_blah_capacity function */
    cmd_norm_name = val_names[cmd_value_name];
    is_cap = 0;
  } else {
    user->message("Wewnętrzny błąd parsowania komendy '"
		  + cmd + "'.\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("Obiekt musi być pokojem bądź przenośnym. Obiekt #"
		  + objnum + " nie jest.\n");
    return;
  }

  if(newvalue < 0.0) {
    user->message("Długość, waga i objętość muszą być dodatnie bądź zero."
		  + "\n.  " + newvalue + " is not.\n");
    return;
  }

  call_other(obj, "set_" + cmd_norm_name + (is_cap ? "_capacity" : ""),
	     newvalue);

  user->message("Done.\n");
}


static void cmd_set_obj_flag(object user, string cmd, string str) {
  object  obj, link_exit;
  int     objnum, flagval;
  string  flagname, flagstring;
  mapping valmap;

  if(str) str = STRINGD->trim_whitespace(str);
  if(str && !STRINGD->stricmp(str, "flagi")) {
    user->message("Nazwy flag:  cont container open openable locked lockable wearable\n");
    return;
  }

  if(!str || sscanf(str, "%*s %*s %*s %*s") == 4
     || (sscanf(str, "#%d %s %s", objnum, flagname, flagstring) != 3
	 && sscanf(str, "#%d %s", objnum, flagname) != 2)) {
    user->message("Użycie: " + cmd + " #<obiekt> flaga [wartość]\n");
    user->message("       " + cmd + " flagi\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    obj = EXITD->get_exit_by_num(objnum);
    if (!obj) {
      user->message("Nie mogę znaleźć obiektu #" + objnum + "!\n");
      return;
    }
  }

  valmap = ([
    "true": 1,
      "false": 0,
      "1": 1,
      "0": 0,
      "yes": 1,
      "no": 0 ]);

  if(flagstring) {
    if( ({ flagstring }) & map_indices(valmap) ) {
      flagval = valmap[flagstring];
    } else {
      user->message("Nie wiem czy wartość '" + flagstring + "' jest prawdą czy fałszem!\n");
      return;
    }
  } else {
    flagval = 1;
  }

  flagname = STRINGD->to_lower(flagname);
  switch (flagname) {
      case "cont":
      case "container":
          obj->set_container(flagval);
          break;
      case "open":
          obj->set_open(flagval);
          break;
      case "openable":
          obj->set_openable(flagval);
          break;
      case "locked":
          obj->set_locked(flagval);
          break;
      case "lockable":
          obj->set_lockable(flagval);
          break;
      case "wearable":
          obj->set_wearable(flagval);
          break;
      default:
          user->message("Nieznana flaga " + flagname + "\n");
          return;
  }

  user->message("Wykonano.\n");
}


static void cmd_make_obj(object user, string cmd, string str) {
  object state;
  string typename;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Użycie: " + cmd + "\n");
    return;
  }

  if(cmd == "@make_room") {
    typename = "room";
  } else if (cmd == "@make_port" || cmd == "@make_portable") {
    typename = "portable";
  } else if (cmd == "@make_det" || cmd == "@make_detail") {
    typename = "detail";
  } else {
    user->message("Nie rozpoznaję typu obiektu "
		  + "który próbujesz utworzyć.\n");
    return;
  }

  state = clone_object(US_MAKE_ROOM);

  user->push_new_state(US_MAKE_ROOM, typename);
}


static void cmd_set_obj_detail(object user, string cmd, string str) {
  int    objnum, detailnum;
  object obj, detail;

  if(!str
     || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d #%d", objnum, detailnum) != 2) {
    user->message("Użycie: " + cmd + " #<obiekt bazowy> #<numer detalu>\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  detail = MAPD->get_room_by_num(detailnum);

  if(objnum != -1 && !obj) {
    user->message("Bazowy obiekt (#" + objnum
		  + ") nie wygląda na pokój lub przenośny.\n");
  }
  if(!detail) {
    user->message("Obiekt detal (#" + detailnum
		  + ") nie wygląda na pokój lub przenośny.\n");
  }

  if(!obj || !detail) return;

  if(obj && detail->get_detail_of()) {
    user->message("Obiekt #" + detailnum + " jest już detalem obiektu #"
		  + detail->get_detail_of()->get_number() + "!\n");
    return;
  } else if(!obj && !detail->get_detail_of()) {
    user->message("Obiekt #" + detailnum
		  + " nie jest detalem czegokolwiek!\n");
    return;
  }

  if(obj) {
    user->message("Ustawianie obiektu #" + detailnum + " jako detalu obiektu #"
		  + objnum + ".\n");
    if(detail->get_location())
      detail->get_location()->remove_from_container(detail);
    obj->add_detail(detail);
  } else {
    obj = detail->get_detail_of();

    user->message("Usuwanie detalu #" + detailnum + " z obiektu #"
		  + obj->get_number() + ".\n");
    obj->remove_detail(detail);
  }

  user->message("Wykonane.\n");
}
