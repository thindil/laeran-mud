#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/map.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <type.h>

/* The Mapd keeps track of room objects, their groupings and their
   relationship to each other.  It also loads in rooms in
   fundamentally data-based formats rather than code-based formats and
   turns them into proper room objects. */

/* room_objects keeps track of rooms by object name and certain aliases */
private mapping room_objects;

/* which unq tags are mapped to which lpc code */
private mapping tag_code;
private object  default_binding_handler;

private object  room_dtd, bind_dtd;
private int     initialized;

/* Rooms that haven't yet fully resolved... */
private object* unresolved_rooms;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

#define ROOM_BIND_FILE "/usr/common/sys/room_binder.unq"

/* Prototypes */
object get_room_by_num(int num);
int* rooms_in_zone(int zone);
private int assign_room_to_zone(int num, object room, int zone);
void upgraded(varargs int clone);


static void create(varargs int clone) {
  room_objects = ([ ]);
  tag_code = ([ ]);
  default_binding_handler = nil;

  upgraded(clone);

  initialized = 0;
}

void upgraded(varargs int clone) {
  if(!SYSTEM() && previous_program() != MAPD)
    return;

  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);
  if(!find_object(UNQ_DTD))
    compile_object(UNQ_DTD);

  if(!unresolved_rooms)
    unresolved_rooms = ({ });
}

void init(string room_dtd_str, string bind_dtd_str) {
  int    ctr;
  mixed *unq_data;
  string bind_file, tag, file;

  if(!SYSTEM())
    return;

  if(!initialized) {
    room_dtd = clone_object(UNQ_DTD);
    room_dtd->load(room_dtd_str);

    /* read the binder file */
    bind_dtd = clone_object(UNQ_DTD);
    bind_dtd->load(bind_dtd_str);

    bind_file = read_file(ROOM_BIND_FILE);
    if (!bind_file)
      error("Nie mogę odczytać pliku " + ROOM_BIND_FILE + "!");

    unq_data = UNQ_PARSER->unq_parse_with_dtd(bind_file, bind_dtd);
    if(!unq_data)
      error("Nie mogę sparsować tekstu w MAPD::init()!");

    if (sizeof(unq_data) % 2)
      error("Nieparzysty rozmiar kawałka unq w MAPD::init()!");

    for (ctr = 0; ctr < sizeof(unq_data); ctr += 2) {
      if (STRINGD->stricmp(unq_data[ctr],"bind"))
	error("Nie kod/tag w MAPD::init()!");

      if (typeof(unq_data[ctr+1]) != T_ARRAY || sizeof(unq_data[ctr+1]) != 2) {
	/* Should never get here for proper DTD */
	error("Wewnętrzny błąd w MAPD->init()");
      }


      if (!STRINGD->stricmp(unq_data[ctr+1][0][0],"tag")) {
	tag = unq_data[ctr+1][0][1];
	file = unq_data[ctr+1][1][1];
      } else {
	tag = unq_data[ctr+1][1][1];
	file = unq_data[ctr+1][0][1];
      }

      if (tag_code[tag] != nil) {
	error("Tag " + tag + " jest już określony w MAPD::init()!");
      }

      /* Assign file to tag, and make sure it exists and is clonable */
      tag_code[tag] = file;
      if(!find_object(file))
	compile_object(file);
    }

    initialized = 1;
  } else error("MAPD już zainicjalizowany!");
}

void destructed(int clone) {
  mixed* rooms;
  int    numzones, riter, ziter;

  if(!SYSTEM())
    return;

  if(room_dtd)
    destruct_object(room_dtd);
  if(bind_dtd)
    destruct_object(bind_dtd);

  /* Now go through and destruct all rooms */
  numzones = ZONED->num_zones();
  for(ziter = 0; ziter < numzones; ziter++) {
    rooms = rooms_in_zone(ziter);
    for(riter = 0; riter < sizeof(rooms); riter++) {
      destruct_object(get_room_by_num(rooms[riter]));
    }
  }
}


void add_unq_binding(string tag_name, string tag_path) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Tylko uprzywilejowany kod może dodawać wiązania dla pokoi!");

  if(!tag_name)
    error("(Nil) nie jest prawidłowym tagiem w add_unq_binding!");

  if (tag_code[tag_name] != nil) {
    error("Tag o nazwie '" + tag_name
	  + "' jest już określony w MAPD::add_unq_binding()!");
  }

  /* Assign file to tag, and make sure it exists and is clonable */
  if(!find_object(tag_path))
    compile_object(tag_path);

  /* If we make it through all that without error, do the assignment. */
  tag_code[tag_name] = tag_path;
}

void set_binding_handler(object bhandler) {
  if(previous_program() != GAME_INITD)
    error("Tylko GAME_INITD może ustawiać room-binding handler!");

  if(!bhandler)
    error("(Nil) nie jest prawidłowym handler w set_binding_handler!");

  /* do the assignment */
  default_binding_handler = bhandler;
}

void add_room_object(object room) {
  string name;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  name = object_name(room);
  if(room_objects[name])
    error("Pokój jest już zarejestrowany w add_room_object!");

  room_objects[name] = room;
}

void add_room_to_zone(object room, int num, int req_zone) {
  int seg, allocated, zone;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  allocated = 0;

  if(!room_objects[object_name(room)])
    error("Dodawanie numeru do niezarejstrowanego obiektu " + object_name(room) + "!");

  num = assign_room_to_zone(num, room, req_zone);
  if(num < 0) {
    error("Błąd przy przypisywaniu numeru!");
  }

  seg = num / 100;
  zone = ZONED->get_segment_zone(seg);
  if(zone != req_zone && req_zone != -1)
    error("Pokój przypisany do dziwnego segmentu! Zła strefa!");

  if(zone == -1)
    zone = 0;  /* Fix offset */

  room->set_number(num);
}

void remove_room_object(object room) {
  string name;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  name = object_name(room);
  if(!room_objects[name]) {
    error("Usuwanie pokoju którego nie ma w room_objects!");
  }
  room_objects[name] = nil;

  room->set_number(-1);
}

object get_room_by_num(int num) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(num < 0) return nil;

  return OBJNUMD->get_object(num);
}


/* Find appropriate room number in the requested zone (if any) */
private int assign_room_to_zone(int num, object room, int req_zone) {
  int    segnum, ctr;
  string segown;

  if(num >= 0) {
    int zone;

    segnum = num / 100;

    segown = OBJNUMD->get_segment_owner(segnum);
    if(segown && strlen(segown) && segown != MAPD) {
      LOGD->write_syslog("Nie mogę przypisać numeru pokoju " + num
			 + " w segmencie nie należącym do MAPD!", LOG_WARN);
      return -1;
    }
    zone = ZONED->get_segment_zone(segnum);
    if(zone != req_zone && req_zone >= 0)
      error("Pokój o numerze (#" + num
	    + ", strefa #" + zone + ") nie jest w wybranej strefie (#" + req_zone
	    + ") w assign_room_to_zone!");

    OBJNUMD->allocate_in_segment(segnum, num, room);

    return num;
  } else {
    /* This section happens if there is no specific requested room
       number */
    int *zoneseg;

    /* If no specific room number or zone is requested, give us something
       unzoned */
    if(req_zone < 0)
      req_zone = 0;

    zoneseg = ZONED->get_segments_in_zone(req_zone);

    for(ctr = 0; ctr < sizeof(zoneseg); ctr++) {
      num = OBJNUMD->new_in_segment(zoneseg[ctr], room);
      if(num != -1)
	break;
    }
    if(num == -1) {
      segnum = OBJNUMD->allocate_new_segment();
      num = OBJNUMD->new_in_segment(segnum, room);

      if(req_zone)
	ZONED->set_segment_zone(segnum, req_zone);
    }

    return num;
  }
}


/* Take an UNQ description parsed with a DTD and add the appropriate room
   to mapd. */
private object add_struct_for_room(mixed* unq) {
  object room;
  int    num;
  string tag_name, room_program, err;

  /* no unq passed in, so no object passed out */
  if (sizeof(unq) == 0) {
    return nil;
  }

  if(STRINGD->stricmp(unq[0], "object")) {
    error("Etykieta '" + unq[0] + "' nie wygląda jak początek obiektu!");
  }

  if(!STRINGD->stricmp(unq[1][0][0], "obj_type")) {
    if(typeof(unq[1][0][1]) != T_STRING)
      error("UNQ obj_type dane nie są string!");

    tag_name = STRINGD->trim_whitespace(unq[1][0][1]);
  } else {
    /* Default object type */
    tag_name = "object";
  }

  if (tag_code[tag_name] == nil
      && default_binding_handler) {
    err = catch(room_program
		= default_binding_handler->type_for_tag(tag_name));
    if(err) {
      error("Błąd w wywołaniu type_for_tag na binding handler, typ " + tag_name
	    + ": " + err);
    }
  } else {
    room_program = tag_code[tag_name];
  }

  if(room_program == nil) {
    error("Tag " + tag_name
	  + " nie jest przypisany do jakiegokolwiek typu pokoju!");
  }

  if (!find_object(room_program)) {
    catch {
      compile_object(room_program);
    } : {
      error("Nie mogę skompilować obiektu '" + room_program
	    + "' dla tagu '" + tag_name + "'!");
    }
  }

  err = catch(room = clone_object(room_program));
  if(err) {
    error("Błąd przy klonowaniu programu " + room_program + " typu '" + tag_name
	  + "' kiedy tworzono nowy pokój: " + err);
  }
  room->from_dtd_unq(unq);

  /* Get the requested number from the room, or -1 for default.
     Attempt to assign this number. */
  num = room->get_number();
  num = assign_room_to_zone(num, room, -1); /* assign to any zone */
  if(num < 0) {
    error("Nie mogę przyznać numeru pokoju!");
  }
  room->set_number(num);

  if(!room_objects[object_name(room)])
    error("Zapomniałeś zarejestrować z MAPD wybranego pokoju?");

  return room;
}


private int resolve_parent(object room) {
  int*    pending_parents;
  int     ctr;
  object* parents, *pending_phr;
  object  parent;

  pending_parents = room->get_pending_parents();
  parents = ({ });
  if(pending_parents && sizeof(pending_parents)) {
    for(ctr = 0; ctr < sizeof(pending_parents); ctr++) {
      parent = MAPD->get_room_by_num(pending_parents[ctr]);
      if(!parent) {
	return 0;
      }
      parents += ({ parent });
    }

    room->set_archetypes(parents);

    /* Remove nouns in pending_removed_nouns */
    pending_phr = room->get_pending_removed_nouns();
    for(ctr = 0; ctr < sizeof(pending_phr); ctr++) {
      room->remove_noun(pending_phr[ctr]);
    }

    /* Removed adjectives in pending_removed_adjectives */
    pending_phr = room->get_pending_removed_adjectives();
    for(ctr = 0; ctr < sizeof(pending_phr); ctr++) {
      room->remove_adjective(pending_phr[ctr]);
    }
    return 1;
  }

  /* Don't need to set removed_nouns and removed_adjectives if there
     are no parents... */
  return 1;
}


private int resolve_removed_details(object room) {
  int    *rem_det;
  int     ctr;
  object  tmp;
  object *new_rem_det;

  rem_det = room->get_pending_removed_details();
  new_rem_det = ({ });
  for(ctr = 0; ctr < sizeof(rem_det); ctr++) {
    tmp = MAPD->get_room_by_num(rem_det[ctr]);
    if(!tmp)
      return 0;

    new_rem_det += ({ tmp });
  }

  room->set_removed_details(new_rem_det);
  return 1;
}

private int resolve_location(object room) {
  int    pending;
  object container;

  /* Resolve details (rather than regular containment) */
  pending = room->get_pending_detail_of();
  if(pending != -1) {
    container = get_room_by_num(pending);
    if(!container) {
      return 0;
    }

    container->add_detail(room);
    return 1;
  }

  /* Resolve regular containment */
  pending = room->get_pending_location();
  if(pending != -1) {
    container = get_room_by_num(pending);
    if(!container) {
      return 0;
    }

    container->add_to_container(room);
    return 1;
  } else {
    container = get_room_by_num(0);  /* Else, add to The Void */
    if(!container)
      error("Nie mogę znaleźć pokoju #0! Panika!");

    container->add_to_container(room);
    return 1;
  }
}


object *get_deferred_rooms(void) {
  if(SYSTEM())
    return unresolved_rooms[..];

  return nil;
}


void do_room_resolution(int fully) {
  int     iter, res_tmp, finished;
  mapping done_resolve;
  object* new_unres;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Tylko uprzywilejowany kod może żądać analizy pokoju!");

  done_resolve = ([ ]);
  finished = 0;

  while(finished < sizeof(unresolved_rooms)) {
    res_tmp = 0;

    /* Do one pass through the list, trying once each to
       resolve each unresolved room. */
    for(iter = 0; iter < sizeof(unresolved_rooms); iter++) {

      /* Skip things we've already resolved. */
      if(done_resolve[unresolved_rooms[iter]])
	continue;

      if(resolve_location(unresolved_rooms[iter])) {
	res_tmp = 1;
	done_resolve[unresolved_rooms[iter]] = 1;
	finished++;
      }
    }

    if(!res_tmp) {
      /* We're not resolving any more rooms... */
      break;
    }
  }

  /* Now resolve parents and removed details. */
  new_unres = ({ });
  for(iter = 0; iter < sizeof(unresolved_rooms); iter++) {
    if(!done_resolve[unresolved_rooms[iter]]
       || !resolve_parent(unresolved_rooms[iter])
       || !resolve_removed_details(unresolved_rooms[iter])) {
      new_unres += ({ unresolved_rooms[iter] });
      continue;
    }

    /* Clear all pending data */
    unresolved_rooms[iter]->clear_pending();
  }

  unresolved_rooms = new_unres;

  /* See if we were asked to resolve 100% of remaining rooms */
  if(fully && sizeof(unresolved_rooms)) {
    string tmp;
    int    ctr;

    tmp = "Nie mogę przeanalizować następujących pokoi: ";
    for(ctr = 0; ctr < sizeof(unresolved_rooms) - 1; ctr++) {
      tmp += unresolved_rooms[ctr]->get_number() + ", ";
    }

    /* This makes sure there's no final comma */
    tmp += unresolved_rooms[sizeof(unresolved_rooms) - 1]->get_number();

    LOGD->write_syslog("Nie mogę sprawdzić pokoi: " + tmp, LOG_FATAL);
    error("Nie mogę sprawdzić wszystkich pokoi! Edytuj pliki z pokojami aby to naprawić!");
  }
}


void add_dtd_unq_rooms(mixed* unq, string filename) {
  int    iter;
  mixed* resolve_rooms;
  object room;

  if(!initialized)
    error("Can't add rooms to uninitialized mapd!");

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  /* TODO: we'll need the filename for objectd notify dependencies
     -- we'll need to keep track of the fact that if that file
     changes, these room objects change. */

  iter = 0;
  resolve_rooms = ({ });
  while(iter < sizeof(unq)) {
    room = add_struct_for_room( ({ unq[iter], unq[iter + 1] }) );
    resolve_rooms += ({ room });
    iter += 2;
  }

  unresolved_rooms += resolve_rooms;
  do_room_resolution(0);
}

/* Take a chunk of text to parse as UNQ and add rooms appropriately... */
void add_unq_text_rooms(string text, string filename) {
  mixed* unq_data;

  if(!initialized)
    error("Can't add rooms to uninitialized mapd!");

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  unq_data = UNQ_PARSER->unq_parse_with_dtd(text, room_dtd, filename);
  if(!unq_data)
    error("Cannot parse text in add_unq_text_rooms!");

  add_dtd_unq_rooms(unq_data, filename);
}

int* segments_in_zone(int zone) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return ZONED->get_segments_in_zone(zone);
}

int* rooms_in_segment(int segment) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(OBJNUMD->get_segment_owner(segment) != MAPD)
    return nil;

  return OBJNUMD->objects_in_segment(segment);
}

int* rooms_in_zone(int zone) {
  int *segs, *rooms, *tmp;
  int  iter;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  segs = segments_in_zone(zone);
  if(!segs) return nil;
  rooms = ({ });

  for(iter = 0; iter < sizeof(segs); iter++) {
    tmp = rooms_in_segment(segs[iter]);
    if(tmp)
      rooms += tmp;
  }

  return rooms;
}
